// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamermediaencoder_p.h"
#include "qgstreamerintegration_p.h"
#include "qgstreamerformatinfo_p.h"
#include "qgstpipeline_p.h"
#include "qgstreamermessage_p.h"
#include <private/qplatformcamera_p.h>
#include "qaudiodevice.h"
#include <private/qmediastoragelocation_p.h>

#include <qdebug.h>
#include <qeventloop.h>
#include <qstandardpaths.h>
#include <qmimetype.h>
#include <qloggingcategory.h>

#include <gst/gsttagsetter.h>
#include <gst/gstversion.h>
#include <gst/video/video.h>
#include <gst/pbutils/encoding-profile.h>

static Q_LOGGING_CATEGORY(qLcMediaEncoderGst, "qt.multimedia.encoder")

QT_BEGIN_NAMESPACE

QGstreamerMediaEncoder::QGstreamerMediaEncoder(QMediaRecorder *parent)
  : QPlatformMediaRecorder(parent),
    audioPauseControl(*this),
    videoPauseControl(*this)
{
    signalDurationChangedTimer.setInterval(100);
    signalDurationChangedTimer.callOnTimeout(&signalDurationChangedTimer,
                                             [this](){ durationChanged(duration()); });
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
    if (message.isNull())
        return false;
    auto msg = message;

//    qCDebug(qLcMediaEncoderGst) << "received event from" << message.source().name() << Qt::hex << message.type();
//    if (message.type() == GST_MESSAGE_STATE_CHANGED) {
//        GstState    oldState;
//        GstState    newState;
//        GstState    pending;
//        gst_message_parse_state_changed(gm, &oldState, &newState, &pending);
//        qCDebug(qLcMediaEncoderGst) << "received state change from" << message.source().name() << oldState << newState << pending;
//    }
    if (msg.type() == GST_MESSAGE_ELEMENT) {
        QGstStructure s = msg.structure();
        qCDebug(qLcMediaEncoderGst) << "received element message from" << msg.source().name() << s.name();
        if (s.name() == "GstBinForwarded")
            msg = QGstreamerMessage(s);
        if (msg.isNull())
            return false;
    }

    if (msg.type() == GST_MESSAGE_EOS) {
        qCDebug(qLcMediaEncoderGst) << "received EOS from" << msg.source().name();
        finalize();
        return false;
    }

    if (msg.type() == GST_MESSAGE_ERROR) {
        GError *err;
        gchar *debug;
        gst_message_parse_error(msg.rawMessage(), &err, &debug);
        error(QMediaRecorder::ResourceError, QString::fromUtf8(err->message));
        g_error_free(err);
        g_free(debug);
        if (!m_finalizing)
            stop();
        finalize();
    }

    return false;
}

qint64 QGstreamerMediaEncoder::duration() const
{
    return std::max(audioPauseControl.duration, videoPauseControl.duration);
}


static GstEncodingContainerProfile *createContainerProfile(const QMediaEncoderSettings &settings)
{
    auto *formatInfo = QGstreamerIntegration::instance()->gstFormatsInfo();

    auto caps = formatInfo->formatCaps(settings.fileFormat());

    GstEncodingContainerProfile *profile = (GstEncodingContainerProfile *)gst_encoding_container_profile_new(
        "container_profile",
        (gchar *)"custom container profile",
        const_cast<GstCaps *>(caps.get()),
        nullptr); //preset
    return profile;
}

static GstEncodingProfile *createVideoProfile(const QMediaEncoderSettings &settings)
{
    auto *formatInfo = QGstreamerIntegration::instance()->gstFormatsInfo();

    auto caps = formatInfo->videoCaps(settings.mediaFormat());
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
    auto *formatInfo = QGstreamerIntegration::instance()->gstFormatsInfo();

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

void QGstreamerMediaEncoder::PauseControl::reset()
{
    pauseOffsetPts = 0;
    pauseStartPts.reset();
    duration = 0;
    firstBufferPts.reset();
}

void QGstreamerMediaEncoder::PauseControl::installOn(QGstPad pad)
{
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

    if (!firstBufferPts)
        firstBufferPts = GST_BUFFER_PTS(buffer);

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

    duration = (GST_BUFFER_PTS(buffer) - *firstBufferPts) / GST_MSECOND;

    return GST_PAD_PROBE_OK;
}

void QGstreamerMediaEncoder::record(QMediaEncoderSettings &settings)
{
    if (!m_session ||m_finalizing || state() != QMediaRecorder::StoppedState)
        return;

    const auto hasVideo = m_session->camera() && m_session->camera()->isActive();
    const auto hasAudio = m_session->audioInput() != nullptr;

    if (!hasVideo && !hasAudio) {
        error(QMediaRecorder::ResourceError, QMediaRecorder::tr("No camera or audio input"));
        return;
    }

    const auto audioOnly = settings.videoCodec() == QMediaFormat::VideoCodec::Unspecified;

    auto primaryLocation = audioOnly ? QStandardPaths::MusicLocation : QStandardPaths::MoviesLocation;
    auto container = settings.mimeType().preferredSuffix();
    auto location = QMediaStorageLocation::generateFileName(outputLocation().toLocalFile(), primaryLocation, container);

    QUrl actualSink = QUrl::fromLocalFile(QDir::currentPath()).resolved(location);
    qCDebug(qLcMediaEncoderGst) << "recording new video to" << actualSink;

    Q_ASSERT(!actualSink.isEmpty());

    gstEncoder = QGstElement("encodebin", "encodebin");
    Q_ASSERT(gstEncoder);
    auto *encodingProfile = createEncodingProfile(settings);
    g_object_set (gstEncoder.object(), "profile", encodingProfile, nullptr);
    gst_encoding_profile_unref(encodingProfile);

    gstFileSink = QGstElement("filesink", "filesink");
    Q_ASSERT(gstFileSink);
    gstFileSink.set("location", QFile::encodeName(actualSink.toLocalFile()).constData());
    gstFileSink.set("async", false);

    QGstPad audioSink = {};
    QGstPad videoSink = {};

    audioPauseControl.reset();
    videoPauseControl.reset();

    if (hasAudio) {
        audioSink = gstEncoder.getRequestPad("audio_%u");
        if (audioSink.isNull())
            qWarning() << "Unsupported audio codec";
        else
            audioPauseControl.installOn(audioSink);
    }

    if (hasVideo) {
        videoSink = gstEncoder.getRequestPad("video_%u");
        if (videoSink.isNull())
            qWarning() << "Unsupported video codec";
        else
            videoPauseControl.installOn(videoSink);
    }

    gstPipeline.add(gstEncoder, gstFileSink);
    gstEncoder.link(gstFileSink);
    m_metaData.setMetaData(gstEncoder.bin());

    m_session->linkEncoder(audioSink, videoSink);

    gstEncoder.syncStateWithParent();
    gstFileSink.syncStateWithParent();

    signalDurationChangedTimer.start();
    gstPipeline.dumpGraph("recording");

    durationChanged(0);
    stateChanged(QMediaRecorder::RecordingState);
    actualLocationChanged(QUrl::fromLocalFile(location));
}

void QGstreamerMediaEncoder::pause()
{
    if (!m_session || m_finalizing || state() != QMediaRecorder::RecordingState)
        return;
    signalDurationChangedTimer.stop();
    gstPipeline.dumpGraph("before-pause");
    stateChanged(QMediaRecorder::PausedState);
}

void QGstreamerMediaEncoder::resume()
{
    gstPipeline.dumpGraph("before-resume");
    if (!m_session || m_finalizing || state() != QMediaRecorder::PausedState)
        return;
    signalDurationChangedTimer.start();
    stateChanged(QMediaRecorder::RecordingState);
}

void QGstreamerMediaEncoder::stop()
{
    if (!m_session || m_finalizing || state() == QMediaRecorder::StoppedState)
        return;
    qCDebug(qLcMediaEncoderGst) << "stop";
    m_finalizing = true;
    m_session->unlinkEncoder();
    signalDurationChangedTimer.stop();

    qCDebug(qLcMediaEncoderGst) << ">>>>>>>>>>>>> sending EOS";
    gstEncoder.sendEos();
}

void QGstreamerMediaEncoder::finalize()
{
    if (!m_session || gstEncoder.isNull())
        return;

    qCDebug(qLcMediaEncoderGst) << "finalize";

    gstPipeline.remove(gstEncoder);
    gstPipeline.remove(gstFileSink);
    gstEncoder.setStateSync(GST_STATE_NULL);
    gstFileSink.setStateSync(GST_STATE_NULL);
    gstFileSink = {};
    gstEncoder = {};
    m_finalizing = false;
    stateChanged(QMediaRecorder::StoppedState);
}

void QGstreamerMediaEncoder::setMetaData(const QMediaMetaData &metaData)
{
    if (!m_session)
        return;
    m_metaData = static_cast<const QGstreamerMetaData &>(metaData);
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
        stop();
        if (m_finalizing) {
            QEventLoop loop;
            loop.connect(mediaRecorder(), SIGNAL(recorderStateChanged(RecorderState)), SLOT(quit()));
            loop.exec();
        }

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

QT_END_NAMESPACE
