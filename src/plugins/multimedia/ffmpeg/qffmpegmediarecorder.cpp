// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediarecorder_p.h"
#include "qaudiodevice.h"
#include <private/qmediastoragelocation_p.h>
#include <private/qplatformcamera_p.h>
#include <private/qplatformsurfacecapture_p.h>
#include "qaudiosource.h"
#include "qffmpegaudioinput_p.h"
#include "qaudiobuffer.h"
#include "recordingengine/qffmpegrecordingengine_p.h"
#include "qffmpegmediacapturesession_p.h"

#include <qdebug.h>
#include <qloggingcategory.h>

static Q_LOGGING_CATEGORY(qLcMediaEncoder, "qt.multimedia.ffmpeg.encoder");

QT_BEGIN_NAMESPACE

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
    const auto hasVideo = !videoSources.empty();
    const auto hasAudio = m_session->audioInput() != nullptr;

    if (!hasVideo && !hasAudio) {
        updateError(QMediaRecorder::ResourceError, QMediaRecorder::tr("No video or audio input"));
        return;
    }

    auto actualLocation = findActualLocation(settings);

    qCDebug(qLcMediaEncoder) << "recording new media to" << actualLocation;
    qCDebug(qLcMediaEncoder) << "requested format:" << settings.fileFormat()
                             << settings.audioCodec();

    auto formatContext = std::make_unique<QFFmpeg::EncodingFormatContext>(settings.fileFormat());

    formatContext->openAVIO(actualLocation);

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

    auto handleStreamInitializationError = [this](QMediaRecorder::Error code,
                                                  const QString &description) {
        qCWarning(qLcMediaEncoder) << "Stream initialization error:" << description;
        updateError(code, description);
    };

    connect(m_recordingEngine.get(), &QFFmpeg::RecordingEngine::streamInitializationError, this,
            handleStreamInitializationError);

    durationChanged(0);
    stateChanged(QMediaRecorder::RecordingState);
    actualLocationChanged(QUrl::fromLocalFile(actualLocation));

    m_recordingEngine->initialize(static_cast<QFFmpegAudioInput *>(m_session->audioInput()),
                                  videoSources);
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
    auto * input = m_session ? m_session->audioInput() : nullptr;
    if (input)
        static_cast<QFFmpegAudioInput *>(input)->setRunning(false);
    qCDebug(qLcMediaEncoder) << "stop";

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

void QFFmpegMediaRecorder::RecordingEngineDeleter::operator()(
        RecordingEngine *recordingEngine) const
{
    // ### all of the below should be done asynchronous. finalize() should do it's work in a thread
    // to avoid blocking the UI in case of slow codecs
    recordingEngine->finalize();
}

QT_END_NAMESPACE

#include "moc_qffmpegmediarecorder_p.cpp"
