// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "recordingrunner.h"

/// Base

RecordingRunner::RecordingRunner(const RecorderSettings &recorderSettings)
{
    if (recorderSettings.frameRate)
        m_recorder.setVideoFrameRate(recorderSettings.frameRate);
    if (recorderSettings.resolution.isValid())
        m_recorder.setVideoResolution(recorderSettings.resolution);
    if (recorderSettings.quality)
        m_recorder.setQuality(*recorderSettings.quality);
    if (!recorderSettings.outputLocation.isEmpty())
        m_recorder.setOutputLocation(recorderSettings.outputLocation);
    if (recorderSettings.fileFormat || recorderSettings.audioCodec || recorderSettings.videoCodec) {
        QMediaFormat format;
        if (recorderSettings.fileFormat)
            format.setFileFormat(*recorderSettings.fileFormat);
        if (recorderSettings.videoCodec)
            format.setVideoCodec(*recorderSettings.videoCodec);
        if (recorderSettings.audioCodec)
            format.setAudioCodec(*recorderSettings.audioCodec);
        m_recorder.setMediaFormat(format);
    }

    m_recorder.setAutoStop(true);

    connect(&m_recorder, &QMediaRecorder::recorderStateChanged, this,
            &RecordingRunner::handleRecorderStateChanged);
    connect(&m_recorder, &QMediaRecorder::errorOccurred, this, &RecordingRunner::handleError);

    m_session.setRecorder(&m_recorder);
}

RecordingRunner::~RecordingRunner() = default;

void RecordingRunner::run()
{
    m_recorder.record();
}

void RecordingRunner::handleRecorderStateChanged(QMediaRecorder::RecorderState state)
{
    if (state == QMediaRecorder::RecordingState) {
        qInfo() << "Starting recording to" << m_recorder.actualLocation();
    } else if (state == QMediaRecorder::StoppedState) {
        const bool noError = m_recorder.error() == QMediaRecorder::NoError;
        qInfo() << "The recording to" << m_recorder.actualLocation() << "has been finished"
                << (noError ? "" : "with an error");

        emit finished();
    }
}

void RecordingRunner::handleError(QMediaRecorder::Error error, QString description)
{
    qWarning() << "Recoding error occurred:" << error << description;

    if (m_recorder.recorderState() == QMediaRecorder::StoppedState)
        emit finished();
}

/// Pull mode

PullModeRecordingRunner::PullModeRecordingRunner(
        const RecorderSettings &recorderSettings,
        const AudioGeneratorSettingsOpt &audioGeneratorSettings,
        const VideoGeneratorSettingsOpt &videoGeneratorSettings)
    : RecordingRunner(recorderSettings)
{
    Q_ASSERT(audioGeneratorSettings || videoGeneratorSettings);

    if (audioGeneratorSettings) {
        m_audioGenerator = std::make_unique<AudioGenerator>(*audioGeneratorSettings);
        m_audioInput = std::make_unique<QAudioBufferInput>();
        connect(m_audioInput.get(), &QAudioBufferInput::readyToSendAudioBuffer, this,
                &PullModeRecordingRunner::sendNextAudioBuffer);
        session().setAudioBufferInput(m_audioInput.get());
    }

    if (videoGeneratorSettings) {
        m_videoGenerator = std::make_unique<VideoGenerator>(*videoGeneratorSettings);
        m_videoInput = std::make_unique<QVideoFrameInput>();
        connect(m_videoInput.get(), &QVideoFrameInput::readyToSendVideoFrame, this,
                &PullModeRecordingRunner::sendNextVideoFrame);
        session().setVideoFrameInput(m_videoInput.get());
    }
}

void PullModeRecordingRunner::sendNextAudioBuffer()
{
    const bool result = m_audioInput->sendAudioBuffer(m_audioGenerator->generate());
    Q_ASSERT(result);
}

void PullModeRecordingRunner::sendNextVideoFrame()
{
    const bool result = m_videoInput->sendVideoFrame(m_videoGenerator->generate());
    Q_ASSERT(result);
}

/// Push mode

PushModeRecordingRunner::PushModeRecordingRunner(
        const RecorderSettings &recorderSettings,
        const AudioGeneratorSettingsOpt &audioGeneratorSettings,
        const VideoGeneratorSettingsOpt &videoGeneratorSettings,
        const PushModeSettings &pushModeSettings)
    : RecordingRunner(recorderSettings)
{
    if (audioGeneratorSettings) {
        m_audioBufferInputQueue =
                std::make_unique<AudioBufferInputQueue>(pushModeSettings.maxQueueSize);
        session().setAudioBufferInput(m_audioBufferInputQueue->mediaFrameInput());
        m_audioBufferSource = std::make_unique<AudioBufferSource>(*audioGeneratorSettings,
                                                                  pushModeSettings.producingRate);
        m_audioBufferSource->addFrameReceivedCallback(&AudioBufferInputQueue::pushMediaFrame,
                                                      m_audioBufferInputQueue.get());
    }

    if (videoGeneratorSettings) {
        m_videoFrameInputQueue =
                std::make_unique<VideoFrameInputQueue>(pushModeSettings.maxQueueSize);
        session().setVideoFrameInput(m_videoFrameInputQueue->mediaFrameInput());
        m_videoFrameSource = std::make_unique<VideoFrameSource>(*videoGeneratorSettings,
                                                                pushModeSettings.producingRate);
        m_videoFrameSource->addFrameReceivedCallback(&VideoFrameInputQueue::pushMediaFrame,
                                                     m_videoFrameInputQueue.get());
    }

    connect(this, &RecordingRunner::finished, this, &PushModeRecordingRunner::onFinished);
}

PushModeRecordingRunner::~PushModeRecordingRunner() = default;

void PushModeRecordingRunner::onFinished()
{
    m_audioBufferSource.reset();
    m_videoFrameSource.reset();
}

void PushModeRecordingRunner::run()
{
    if (m_audioBufferSource)
        m_audioBufferSource->run();
    if (m_videoFrameSource)
        m_videoFrameSource->run();
    RecordingRunner::run();
}

#include "moc_recordingrunner.cpp"
