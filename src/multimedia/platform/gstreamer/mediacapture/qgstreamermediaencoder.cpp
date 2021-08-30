/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstreamermediaencoder_p.h"
#include "private/qgstreamerintegration_p.h"
#include "private/qgstreamerformatinfo_p.h"
#include "private/qgstpipeline_p.h"
#include "private/qgstreamermessage_p.h"
#include "qaudiodevice.h"
#include "qmediastoragelocation_p.h"

#include <qdebug.h>
#include <qstandardpaths.h>
#include <qmimetype.h>
#include <qloggingcategory.h>

#include <gst/gsttagsetter.h>
#include <gst/gstversion.h>
#include <gst/video/video.h>
#include <gst/pbutils/encoding-profile.h>

Q_LOGGING_CATEGORY(qLcMediaEncoder, "qt.multimedia.encoder")

QGstreamerMediaEncoder::QGstreamerMediaEncoder(QMediaRecorder *parent)
  : QPlatformMediaEncoder(parent),
    audioPauseControl(*this),
    videoPauseControl(*this)
{
    // used to update duration every 100 msecond
    heartbeat.setInterval(100);
    QObject::connect(&heartbeat, &QTimer::timeout, [this]() { updateDuration(); });
}

QGstreamerMediaEncoder::~QGstreamerMediaEncoder()
{
    if (!gstPipeline.isNull()) {
        finalize();
        gstPipeline.removeMessageFilter(this);
        gstPipeline.setStateSync(GST_STATE_NULL);
    }
}

bool QGstreamerMediaEncoder::isLocationWritable(const QUrl &) const
{
    return true;
}

void QGstreamerMediaEncoder::handleSessionError(QMediaRecorder::Error code, const QString &description)
{
    error(code, description);
    stop();
}

bool QGstreamerMediaEncoder::processBusMessage(const QGstreamerMessage &message)
{
    GstMessage *gm = message.rawMessage();
    if (!gm)
        return false;

//    qCDebug(qLcMediaEncoder) << "received event from" << message.source().name() << Qt::hex << message.type();
//    if (message.type() == GST_MESSAGE_STATE_CHANGED) {
//        GstState    oldState;
//        GstState    newState;
//        GstState    pending;
//        gst_message_parse_state_changed(gm, &oldState, &newState, &pending);
//        qCDebug(qLcMediaEncoder) << "received state change from" << message.source().name() << oldState << newState << pending;
//    }
    if (message.type() == GST_MESSAGE_ELEMENT) {
        QGstStructure s = gst_message_get_structure(gm);
        qCDebug(qLcMediaEncoder) << "received element message from" << message.source().name() << s.name();
        if (s.name() == "GstBinForwarded")
            gm = s.getMessage();
        if (!gm)
            return false;
    }

    if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_EOS) {
        qCDebug(qLcMediaEncoder) << "received EOS from" << QGstObject(GST_MESSAGE_SRC(gm)).name();
        finalize();
        return false;
    }

    if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ERROR) {
        GError *err;
        gchar *debug;
        gst_message_parse_error(gm, &err, &debug);
        error(QMediaRecorder::ResourceError, QString::fromUtf8(err->message));
        g_error_free(err);
        g_free(debug);
    }

    if (GST_MESSAGE_SRC(gm) == gstEncoder.object()) {
        switch (GST_MESSAGE_TYPE(gm))  {
        case GST_MESSAGE_STATE_CHANGED: {
            GstState    oldState;
            GstState    newState;
            GstState    pending;
            gst_message_parse_state_changed(gm, &oldState, &newState, &pending);

            if (newState == GST_STATE_PAUSED && !m_metaData.isEmpty())
                setMetaData(m_metaData);
            break;
        }
        default:
            break;
        }
    }
    return false;
}

void QGstreamerMediaEncoder::updateDuration()
{
    durationChanged(m_duration.elapsed());
}

qint64 QGstreamerMediaEncoder::duration() const
{
    return m_duration.elapsed();
}


static GstEncodingContainerProfile *createContainerProfile(const QMediaEncoderSettings &settings)
{
    auto *formatInfo = QGstreamerIntegration::instance()->m_formatsInfo;

    QGstMutableCaps caps = formatInfo->formatCaps(settings.fileFormat());

    GstEncodingContainerProfile *profile = (GstEncodingContainerProfile *)gst_encoding_container_profile_new(
        "container_profile",
        (gchar *)"custom container profile",
        const_cast<GstCaps *>(caps.get()),
        nullptr); //preset
    return profile;
}

static GstEncodingProfile *createVideoProfile(const QMediaEncoderSettings &settings)
{
    auto *formatInfo = QGstreamerIntegration::instance()->m_formatsInfo;

    QGstMutableCaps caps = formatInfo->videoCaps(settings.mediaFormat());
    if (caps.isNull())
        return nullptr;

    GstEncodingVideoProfile *profile = gst_encoding_video_profile_new(
        const_cast<GstCaps *>(caps.get()),
        nullptr,
        nullptr, //restriction
        0); //presence

    gst_encoding_video_profile_set_pass(profile, 0);
    gst_encoding_video_profile_set_variableframerate(profile, TRUE);

    return (GstEncodingProfile *)profile;
}

static GstEncodingProfile *createAudioProfile(const QMediaEncoderSettings &settings)
{
    auto *formatInfo = QGstreamerIntegration::instance()->m_formatsInfo;

    auto caps = formatInfo->audioCaps(settings.mediaFormat());
    if (caps.isNull())
        return nullptr;

    GstEncodingProfile *profile = (GstEncodingProfile *)gst_encoding_audio_profile_new(
        const_cast<GstCaps *>(caps.get()),
        nullptr, //preset
        nullptr,   //restriction
        0);     //presence

    return profile;
}


static GstEncodingContainerProfile *createEncodingProfile(const QMediaEncoderSettings &settings)
{
    auto *containerProfile = createContainerProfile(settings);
    if (!containerProfile) {
        qWarning() << "QGstreamerMediaEncoder: failed to create container profile!";
        return nullptr;
    }

    GstEncodingProfile *audioProfile = createAudioProfile(settings);
    GstEncodingProfile *videoProfile = nullptr;
    if (settings.videoCodec() != QMediaFormat::VideoCodec::Unspecified)
        videoProfile = createVideoProfile(settings);
//    qDebug() << "audio profile" << (audioProfile ? gst_caps_to_string(gst_encoding_profile_get_format(audioProfile)) : "(null)");
//    qDebug() << "video profile" << (videoProfile ? gst_caps_to_string(gst_encoding_profile_get_format(videoProfile)) : "(null)");
//    qDebug() << "conta profile" << gst_caps_to_string(gst_encoding_profile_get_format((GstEncodingProfile *)containerProfile));

    if (videoProfile) {
        if (!gst_encoding_container_profile_add_profile(containerProfile, videoProfile)) {
            qWarning() << "QGstreamerMediaEncoder: failed to add video profile!";
            gst_encoding_profile_unref(videoProfile);
        }
    }
    if (audioProfile) {
        if (!gst_encoding_container_profile_add_profile(containerProfile, audioProfile)) {
            qWarning() << "QGstreamerMediaEncoder: failed to add audio profile!";
            gst_encoding_profile_unref(audioProfile);
        }
    }

    return containerProfile;
}

void QGstreamerMediaEncoder::PauseControl::installOn(QGstPad pad)
{
    pauseOffsetPts = 0;
    pauseStartPts.reset();
    pad.addProbe<&QGstreamerMediaEncoder::PauseControl::processBuffer>(this, GST_PAD_PROBE_TYPE_BUFFER);
}

GstPadProbeReturn QGstreamerMediaEncoder::PauseControl::processBuffer(QGstPad, GstPadProbeInfo *info)
{
    auto buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!buffer)
        return GST_PAD_PROBE_OK;

    buffer = gst_buffer_make_writable(buffer);

    if (!buffer)
        return GST_PAD_PROBE_OK;

    GST_PAD_PROBE_INFO_DATA(info) = buffer;

    if (!GST_BUFFER_PTS_IS_VALID(buffer))
        return GST_PAD_PROBE_OK;

    if (encoder.state() == QMediaRecorder::PausedState) {
        if (!pauseStartPts)
            pauseStartPts = GST_BUFFER_PTS(buffer);

        return GST_PAD_PROBE_DROP;
    }

    if (pauseStartPts) {
        pauseOffsetPts += GST_BUFFER_PTS(buffer) - *pauseStartPts;
        pauseStartPts.reset();
    }
    GST_BUFFER_PTS(buffer) -= pauseOffsetPts;

    return GST_PAD_PROBE_OK;
}

void QGstreamerMediaEncoder::record(QMediaEncoderSettings &settings)
{
    if (!m_session || state() != QMediaRecorder::StoppedState)
        return;

    const auto audioOnly = settings.videoCodec() == QMediaFormat::VideoCodec::Unspecified;

    auto primaryLocation = audioOnly ? QStandardPaths::MusicLocation : QStandardPaths::MoviesLocation;
    auto container = settings.mimeType().preferredSuffix();
    auto location = QMediaStorageLocation::generateFileName(outputLocation().toLocalFile(), primaryLocation, container);

    QUrl actualSink = QUrl::fromLocalFile(QDir::currentPath()).resolved(location);
    qCDebug(qLcMediaEncoder) << "recording new video to" << actualSink;

    Q_ASSERT(!actualSink.isEmpty());

    gstPipeline.dumpGraph("before-recording");

    gstPipeline.beginConfig();

    gstEncoder = QGstElement("encodebin", "encodebin");
    auto *encodingProfile = createEncodingProfile(settings);
    g_object_set (gstEncoder.object(), "profile", encodingProfile, nullptr);
    gst_encoding_profile_unref(encodingProfile);

    gstFileSink = QGstElement("filesink", "filesink");
    gstFileSink.set("location", QFile::encodeName(actualSink.toLocalFile()).constData());

    gstPipeline.add(gstEncoder, gstFileSink);
    gstEncoder.link(gstFileSink);

    gstEncoder.setState(GST_STATE_PAUSED);
    gstFileSink.setState(GST_STATE_PAUSED);

    audioSrcPad = m_session->getAudioPad();
    if (!audioSrcPad.isNull()) {
        QGstPad audioPad = gstEncoder.getRequestPad("audio_%u");
        audioPauseControl.installOn(audioPad);
        audioSrcPad.link(audioPad);
    }

    if (settings.videoCodec() != QMediaFormat::VideoCodec::Unspecified) {
        videoSrcPad = m_session->getVideoPad();
        if (!videoSrcPad.isNull()) {
            QGstPad videoPad = gstEncoder.getRequestPad("video_%u");
            videoPauseControl.installOn(videoPad);
            videoSrcPad.link(videoPad);
        }
    }

    gstPipeline.endConfig();

    m_duration.start();
    heartbeat.start();
    gstPipeline.dumpGraph("recording");

    stateChanged(QMediaRecorder::RecordingState);
    actualLocationChanged(QUrl::fromLocalFile(location));
}

void QGstreamerMediaEncoder::pause()
{
    if (!m_session || state() != QMediaRecorder::RecordingState)
        return;
    heartbeat.stop();
    gstPipeline.dumpGraph("before-pause");
    gstEncoder.setState(GST_STATE_PAUSED);

    stateChanged(QMediaRecorder::PausedState);
}

void QGstreamerMediaEncoder::resume()
{
    gstPipeline.dumpGraph("before-resume");
    if (!m_session || state() != QMediaRecorder::PausedState)
        return;
    heartbeat.start();
    gstEncoder.setState(GST_STATE_PLAYING);
    stateChanged(QMediaRecorder::RecordingState);
}

void QGstreamerMediaEncoder::stop()
{
    if (!m_session || state() == QMediaRecorder::StoppedState)
        return;
    qCDebug(qLcMediaEncoder) << "stop";

    gstPipeline.beginConfig();

    if (!audioSrcPad.isNull()) {
        audioSrcPad.unlinkPeer();
        m_session->releaseAudioPad(audioSrcPad);
        audioSrcPad = {};
    }
    if (!videoSrcPad.isNull()) {
        videoSrcPad.unlinkPeer();
        m_session->releaseVideoPad(videoSrcPad);
        videoSrcPad = {};
    }

    gstPipeline.endConfig();

    //with live sources it's necessary to send EOS even to pipeline
    //before going to STOPPED state
    qCDebug(qLcMediaEncoder) << ">>>>>>>>>>>>> sending EOS";

    gstEncoder.sendEos();
    stateChanged(QMediaRecorder::StoppedState);
}

void QGstreamerMediaEncoder::finalize()
{
    if (!m_session || gstEncoder.isNull())
        return;
    qCDebug(qLcMediaEncoder) << "finalize";

    heartbeat.stop();

    // The filesink can only be used once, replace it with a new one
    gstPipeline.beginConfig();
    gstEncoder.setState(GST_STATE_NULL);
    gstFileSink.setState(GST_STATE_NULL);
    gstPipeline.remove(gstEncoder);
    gstPipeline.remove(gstFileSink);
    gstFileSink = {};
    gstEncoder = {};
    gstPipeline.endConfig();
}

void QGstreamerMediaEncoder::setMetaData(const QMediaMetaData &metaData)
{
    if (!m_session)
        return;
    m_metaData = static_cast<const QGstreamerMetaData &>(metaData);
    m_metaData.setMetaData(gstEncoder.bin());

}

QMediaMetaData QGstreamerMediaEncoder::metaData() const
{
    return m_metaData;
}

void QGstreamerMediaEncoder::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QGstreamerMediaCapture *captureSession = static_cast<QGstreamerMediaCapture *>(session);
    if (m_session == captureSession)
        return;

    if (m_session) {
        finalize();
        gstPipeline.removeMessageFilter(this);
        gstPipeline = {};
    }

    m_session = captureSession;
    if (!m_session)
        return;

    gstPipeline = captureSession->gstPipeline;
    gstPipeline.set("message-forward", true);
    gstPipeline.installMessageFilter(this);
}
