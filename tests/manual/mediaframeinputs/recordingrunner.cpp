// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "recordingrunner.h"

#include <QUrl>
#include <QVideoFrame>
#include <QAudioBuffer>
#include <QAudioBufferInput>
#include <QVideoFrameInput>

RecordingRunner::RecordingRunner(
        const std::optional<AudioGenerator::Settings> &audioGenerationSettings,
        const std::optional<VideoGenerator::Settings> &videoGenerationSettings)
{
    Q_ASSERT(audioGenerationSettings || videoGenerationSettings);

    if (audioGenerationSettings) {
        m_audioGenerator = std::make_unique<AudioGenerator>(*audioGenerationSettings);
        m_audioInput = std::make_unique<QAudioBufferInput>();
        connect(m_audioInput.get(), &QAudioBufferInput::readyToSendAudioBuffer, this,
                &RecordingRunner::sendNextAudioData);
        m_session.setAudioBufferInput(m_audioInput.get());
    }

    if (videoGenerationSettings) {
        m_videoGenerator = std::make_unique<VideoGenerator>(*videoGenerationSettings);
        m_videoInput = std::make_unique<QVideoFrameInput>();
        connect(m_videoInput.get(), &QVideoFrameInput::readyToSendVideoFrame, this,
                &RecordingRunner::sendNextVideoData);
        m_session.setVideoFrameInput(m_videoInput.get());
    }

    m_session.setRecorder(&m_recorder);
    m_recorder.setAutoStop(true);

    connect(&m_recorder, &QMediaRecorder::recorderStateChanged, this,
            &RecordingRunner::handleRecorderStateChanged);
    connect(&m_recorder, &QMediaRecorder::errorOccurred, this, &RecordingRunner::handleError);
}

RecordingRunner::~RecordingRunner() = default;

void RecordingRunner::run(const QUrl &outputLocation)
{
    if (!outputLocation.isEmpty())
        m_recorder.setOutputLocation(outputLocation);
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

void RecordingRunner::sendNextAudioData()
{
    m_audioInput->sendAudioBuffer(m_audioGenerator->generate());
}

void RecordingRunner::sendNextVideoData()
{
    m_videoInput->sendVideoFrame(m_videoGenerator->generate());
}

#include "moc_recordingrunner.cpp"
