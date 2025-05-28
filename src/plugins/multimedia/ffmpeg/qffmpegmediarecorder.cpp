// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediarecorder_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtMultimedia/qaudiobuffer.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudiosource.h>
#include <QtMultimedia/private/qmediastoragelocation_p.h>
#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/private/qplatformsurfacecapture_p.h>

#include "recordingengine/qffmpegrecordingengine_p.h"
#include "qffmpegmediacapturesession_p.h"

Q_STATIC_LOGGING_CATEGORY(qLcMediaEncoder, "qt.multimedia.ffmpeg.encoder");

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QFFmpegMediaRecorder::QFFmpegMediaRecorder(QMediaRecorder *parent) : QPlatformMediaRecorder(parent)
{
}

QFFmpegMediaRecorder::~QFFmpegMediaRecorder() = default;

bool QFFmpegMediaRecorder::isLocationWritable(const QUrl &) const
{
    return true;
}

void QFFmpegMediaRecorder::handleSessionError(QMediaRecorder::Error code, const QString &description)
{
    updateError(code, description);
    stop();
}

void QFFmpegMediaRecorder::record(QMediaEncoderSettings &settings)
{
    if (!m_session || state() != QMediaRecorder::StoppedState)
        return;

    auto videoSources = m_session->activeVideoSources();
    auto audioInputs = m_session->activeAudioInputs();
    const auto hasVideo = !videoSources.empty();
    const auto hasAudio = !audioInputs.empty();

    if (!hasVideo && !hasAudio) {
        updateError(QMediaRecorder::ResourceError, QMediaRecorder::tr("No video or audio input"));
        return;
    }

    if (outputDevice() && !outputLocation().isEmpty())
        qCWarning(qLcMediaEncoder)
                << "Both outputDevice and outputLocation has been set to QMediaRecorder";

    if (outputDevice() && !outputDevice()->isWritable())
        qCWarning(qLcMediaEncoder) << "Output device has been set but not it's not writable";

    QString actualLocation;
    auto formatContext = std::make_unique<QFFmpeg::EncodingFormatContext>(settings.fileFormat());

    if (outputDevice() && outputDevice()->isWritable()) {
        formatContext->openAVIO(outputDevice());
    } else {
        actualLocation = findActualLocation(settings);
        formatContext->openAVIO(actualLocation);
    }

    qCInfo(qLcMediaEncoder).nospace()
            << "Recording new media with muxer "
            << formatContext->avFormatContext()->oformat->long_name << " to "
            << (actualLocation.isNull() ? u"IO device"_s : actualLocation)
            << " with format: " << settings.fileFormat() << ", " << settings.audioCodec() << ", "
            << settings.videoCodec();

    if (!formatContext->isAVIOOpen()) {
        updateError(QMediaRecorder::LocationNotWritable,
                    QMediaRecorder::tr("Cannot open the output location for writing"));
        return;
    }

    m_recordingEngine.reset(new RecordingEngine(settings, std::move(formatContext)));
    m_recordingEngine->setMetaData(m_metaData);

    connect(m_recordingEngine.get(), &QFFmpeg::RecordingEngine::durationChanged, this,
            &QFFmpegMediaRecorder::newDuration);
    connect(m_recordingEngine.get(), &QFFmpeg::RecordingEngine::finalizationDone, this,
            &QFFmpegMediaRecorder::finalizationDone);
    connect(m_recordingEngine.get(), &QFFmpeg::RecordingEngine::sessionError, this,
            &QFFmpegMediaRecorder::handleSessionError);

    updateAutoStop();

    auto handleStreamInitializationError = [this](QMediaRecorder::Error code,
                                                  const QString &description) {
        qCWarning(qLcMediaEncoder) << "Stream initialization error:" << description;
        updateError(code, description);
    };

    connect(m_recordingEngine.get(), &QFFmpeg::RecordingEngine::streamInitializationError, this,
            handleStreamInitializationError);

    durationChanged(0);
    actualLocationChanged(QUrl::fromLocalFile(actualLocation));

    qCDebug(qLcMediaEncoder) << "Starting recording engine";
    if (m_recordingEngine->initialize(audioInputs, videoSources)) {
        stateChanged(QMediaRecorder::RecordingState);
        qCDebug(qLcMediaEncoder) << "Recording engine started";
    } else {
        // else an error has been already emitted
        qCWarning(qLcMediaEncoder) << "Failed to start recording engine";
    }
}

void QFFmpegMediaRecorder::pause()
{
    if (!m_session || state() != QMediaRecorder::RecordingState)
        return;

    Q_ASSERT(m_recordingEngine);
    m_recordingEngine->setPaused(true);

    stateChanged(QMediaRecorder::PausedState);
}

void QFFmpegMediaRecorder::resume()
{
    if (!m_session || state() != QMediaRecorder::PausedState)
        return;

    Q_ASSERT(m_recordingEngine);
    m_recordingEngine->setPaused(false);

    stateChanged(QMediaRecorder::RecordingState);
}

void QFFmpegMediaRecorder::stop()
{
    if (!m_session || state() == QMediaRecorder::StoppedState)
        return;

    qCDebug(qLcMediaEncoder) << "Stopping media recorder";

    m_recordingEngine.reset();
}

void QFFmpegMediaRecorder::finalizationDone()
{
    stateChanged(QMediaRecorder::StoppedState);
}

void QFFmpegMediaRecorder::setMetaData(const QMediaMetaData &metaData)
{
    if (!m_session)
        return;
    m_metaData = metaData;
}

QMediaMetaData QFFmpegMediaRecorder::metaData() const
{
    return m_metaData;
}

void QFFmpegMediaRecorder::setCaptureSession(QFFmpegMediaCaptureSession *session)
{
    auto *captureSession = session;
    if (m_session == captureSession)
        return;

    if (m_session)
        stop();

    m_session = captureSession;
    if (!m_session)
        return;
}

void QFFmpegMediaRecorder::updateAutoStop()
{
    const bool autoStop = mediaRecorder()->autoStop();
    if (!m_recordingEngine || m_recordingEngine->autoStop() == autoStop)
        return;

    if (autoStop)
        connect(m_recordingEngine.get(), &QFFmpeg::RecordingEngine::autoStopped, this,
                &QFFmpegMediaRecorder::stop);
    else
        disconnect(m_recordingEngine.get(), &QFFmpeg::RecordingEngine::autoStopped, this,
                   &QFFmpegMediaRecorder::stop);

    m_recordingEngine->setAutoStop(autoStop);
}

void QFFmpegMediaRecorder::RecordingEngineDeleter::operator()(
        RecordingEngine *recordingEngine) const
{
    // ### all of the below should be done asynchronous. finalize() should do it's work in a thread
    // to avoid blocking the UI in case of slow codecs
    recordingEngine->finalize();
}

QT_END_NAMESPACE

#include "moc_qffmpegmediarecorder_p.cpp"
