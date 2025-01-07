// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <mediacapture/qgstreamermediarecorder_p.h>
#include <qgstreamerformatinfo_p.h>
#include <common/qgstpipeline_p.h>
#include <common/qgstreamermessage_p.h>
#include <common/qgst_debug_p.h>
#include <qgstreamerintegration_p.h>

#include <QtMultimedia/private/qmediastoragelocation_p.h>
#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/qaudiodevice.h>

#include <QtCore/qdebug.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qloggingcategory.h>

#include <gst/gsttagsetter.h>
#include <gst/gstversion.h>
#include <gst/video/video.h>
#include <gst/pbutils/encoding-profile.h>

Q_STATIC_LOGGING_CATEGORY(qLcMediaRecorder, "qt.multimedia.recorder");

QT_BEGIN_NAMESPACE

QGstreamerMediaRecorder::QGstreamerMediaRecorder(QMediaRecorder *parent)
    : QPlatformMediaRecorder(parent), audioPauseControl(*this), videoPauseControl(*this)
{
    signalDurationChangedTimer.setInterval(100);
    signalDurationChangedTimer.callOnTimeout(&signalDurationChangedTimer, [this]() {
        durationChanged(duration());
    });
}

QGstreamerMediaRecorder::~QGstreamerMediaRecorder()
{
    if (m_session)
        finalize();
}

bool QGstreamerMediaRecorder::isLocationWritable(const QUrl &) const
{
    return true;
}

void QGstreamerMediaRecorder::handleSessionError(QMediaRecorder::Error code,
                                                 const QString &description)
{
    updateError(code, description);
    stop();
}

void QGstreamerMediaRecorder::processBusMessage(const QGstreamerMessage &msg)
{
    constexpr bool traceStateChange = false;
    constexpr bool traceAllEvents = false;

    if constexpr (traceAllEvents)
        qCDebug(qLcMediaRecorder) << "received event:" << msg;

    switch (msg.type()) {
    case GST_MESSAGE_ELEMENT: {
        QGstStructureView s = msg.structure();
        if (s.name() == "GstBinForwarded")
            return processBusMessage(s.getMessage());

        qCDebug(qLcMediaRecorder) << "received element message from" << msg.source().name()
                                  << s.name();
        return;
    }

    case GST_MESSAGE_EOS: {
        qCDebug(qLcMediaRecorder) << "received EOS from" << msg.source().name();
        finalize();
        return;
    }

    case GST_MESSAGE_ERROR: {
        qCDebug(qLcMediaRecorder) << "received error:" << msg.source().name()
                                  << QCompactGstMessageAdaptor(msg);

        QUniqueGErrorHandle err;
        QGString debug;
        gst_message_parse_error(msg.message(), &err, &debug);
        updateError(QMediaRecorder::ResourceError, QString::fromUtf8(err.get()->message));
        if (!m_finalizing)
            stop();
        finalize();
        return;
    }

    case GST_MESSAGE_STATE_CHANGED: {
        if constexpr (traceStateChange)
            qCDebug(qLcMediaRecorder) << "received state change" << QCompactGstMessageAdaptor(msg);

        return;
    }

    default:
        return;
    };
}

qint64 QGstreamerMediaRecorder::duration() const
{
    return std::max(audioPauseControl.duration, videoPauseControl.duration);
}


static GstEncodingContainerProfile *createContainerProfile(const QMediaEncoderSettings &settings)
{
    auto *formatInfo = QGstreamerIntegration::instance()->gstFormatsInfo();

    auto caps = formatInfo->formatCaps(settings.fileFormat());

    GstEncodingContainerProfile *profile =
            (GstEncodingContainerProfile *)gst_encoding_container_profile_new(
                    "container_profile", (gchar *)"custom container profile",
                    const_cast<GstCaps *>(caps.caps()),
                    nullptr); // preset
    return profile;
}

static GstEncodingProfile *createVideoProfile(const QMediaEncoderSettings &settings)
{
    auto *formatInfo = QGstreamerIntegration::instance()->gstFormatsInfo();

    QGstCaps caps = formatInfo->videoCaps(settings.mediaFormat());
    if (!caps)
        return nullptr;

    QSize videoResolution = settings.videoResolution();
    if (videoResolution.isValid())
        caps.setResolution(videoResolution);

    GstEncodingVideoProfile *profile =
            gst_encoding_video_profile_new(const_cast<GstCaps *>(caps.caps()), nullptr,
                                           nullptr, // restriction
                                           0); // presence

    gst_encoding_video_profile_set_pass(profile, 0);
    gst_encoding_video_profile_set_variableframerate(profile, TRUE);

    return (GstEncodingProfile *)profile;
}

static GstEncodingProfile *createAudioProfile(const QMediaEncoderSettings &settings)
{
    auto *formatInfo = QGstreamerIntegration::instance()->gstFormatsInfo();

    auto caps = formatInfo->audioCaps(settings.mediaFormat());
    if (!caps)
        return nullptr;

    GstEncodingProfile *profile =
            (GstEncodingProfile *)gst_encoding_audio_profile_new(const_cast<GstCaps *>(caps.caps()),
                                                                 nullptr, // preset
                                                                 nullptr, // restriction
                                                                 0); // presence

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

void QGstreamerMediaRecorder::PauseControl::reset()
{
    pauseOffsetPts = 0;
    pauseStartPts.reset();
    duration = 0;
    firstBufferPts.reset();
}

void QGstreamerMediaRecorder::PauseControl::installOn(QGstPad pad)
{
    pad.addProbe<&QGstreamerMediaRecorder::PauseControl::processBuffer>(this,
                                                                        GST_PAD_PROBE_TYPE_BUFFER);
}

GstPadProbeReturn QGstreamerMediaRecorder::PauseControl::processBuffer(QGstPad,
                                                                       GstPadProbeInfo *info)
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

void QGstreamerMediaRecorder::record(QMediaEncoderSettings &settings)
{
    if (!m_session ||m_finalizing || state() != QMediaRecorder::StoppedState)
        return;

    const auto hasVideo = m_session->camera() && m_session->camera()->isActive();
    const auto hasAudio = m_session->audioInput() != nullptr;

    if (!hasVideo && !hasAudio) {
        updateError(QMediaRecorder::ResourceError, QMediaRecorder::tr("No camera or audio input"));
        return;
    }

    const auto audioOnly = settings.videoCodec() == QMediaFormat::VideoCodec::Unspecified;

    auto primaryLocation = audioOnly ? QStandardPaths::MusicLocation : QStandardPaths::MoviesLocation;
    auto container = settings.preferredSuffix();
    auto location = QMediaStorageLocation::generateFileName(outputLocation().toLocalFile(), primaryLocation, container);

    QUrl actualSink = QUrl::fromLocalFile(QDir::currentPath()).resolved(location);
    qCDebug(qLcMediaRecorder) << "recording new video to" << actualSink;

    Q_ASSERT(!actualSink.isEmpty());

    QGstBin gstEncodebin = QGstBin::createFromFactory("encodebin", "encodebin");
    Q_ASSERT(gstEncodebin);
    auto *encodingProfile = createEncodingProfile(settings);
    g_object_set(gstEncodebin.object(), "profile", encodingProfile, nullptr);
    gst_encoding_profile_unref(encodingProfile);

    QGstElement gstFileSink = QGstElement::createFromFactory("filesink", "filesink");
    Q_ASSERT(gstFileSink);
    gstFileSink.set("location", QFile::encodeName(actualSink.toLocalFile()).constData());

    QGstPad audioSink = {};
    QGstPad videoSink = {};

    audioPauseControl.reset();
    videoPauseControl.reset();

    if (hasAudio) {
        audioSink = gstEncodebin.getRequestPad("audio_%u");
        if (!audioSink)
            qWarning() << "Unsupported audio codec";
        else
            audioPauseControl.installOn(audioSink);
    }

    if (hasVideo) {
        videoSink = gstEncodebin.getRequestPad("video_%u");
        if (!videoSink)
            qWarning() << "Unsupported video codec";
        else
            videoPauseControl.installOn(videoSink);
    }

    QGstreamerMediaCaptureSession::RecorderElements recorder{
        std::move(gstEncodebin),
        std::move(gstFileSink),
        std::move(audioSink),
        std::move(videoSink),
    };

    m_session->linkAndStartEncoder(std::move(recorder), m_metaData);

    signalDurationChangedTimer.start();

    m_session->pipeline().dumpGraph("recording");

    durationChanged(0);
    actualLocationChanged(QUrl::fromLocalFile(location));
    stateChanged(QMediaRecorder::RecordingState);
}

void QGstreamerMediaRecorder::pause()
{
    if (!m_session || m_finalizing || state() != QMediaRecorder::RecordingState)
        return;
    signalDurationChangedTimer.stop();
    durationChanged(duration());
    m_session->pipeline().dumpGraph("before-pause");
    stateChanged(QMediaRecorder::PausedState);
}

void QGstreamerMediaRecorder::resume()
{
    m_session->pipeline().dumpGraph("before-resume");
    if (!m_session || m_finalizing || state() != QMediaRecorder::PausedState)
        return;
    signalDurationChangedTimer.start();
    stateChanged(QMediaRecorder::RecordingState);
}

void QGstreamerMediaRecorder::stop()
{
    if (!m_session || m_finalizing || state() == QMediaRecorder::StoppedState)
        return;
    durationChanged(duration());
    qCDebug(qLcMediaRecorder) << "stop";
    m_finalizing = true;
    m_session->unlinkRecorder();
    signalDurationChangedTimer.stop();
}

void QGstreamerMediaRecorder::finalize()
{
    if (!m_session || !m_finalizing)
        return;

    qCDebug(qLcMediaRecorder) << "finalize";

    m_session->finalizeRecorder();
    m_finalizing = false;
    stateChanged(QMediaRecorder::StoppedState);
}

void QGstreamerMediaRecorder::setMetaData(const QMediaMetaData &metaData)
{
    if (!m_session)
        return;
    m_metaData = metaData;
}

QMediaMetaData QGstreamerMediaRecorder::metaData() const
{
    return m_metaData;
}

void QGstreamerMediaRecorder::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QGstreamerMediaCaptureSession *captureSession =
            static_cast<QGstreamerMediaCaptureSession *>(session);
    if (m_session == captureSession)
        return;

    if (m_session) {
        stop();
        if (m_finalizing) {
            QEventLoop loop;
            QObject::connect(mediaRecorder(), &QMediaRecorder::recorderStateChanged, &loop,
                             &QEventLoop::quit);
            loop.exec();
        }
    }

    m_session = captureSession;
}

QT_END_NAMESPACE
