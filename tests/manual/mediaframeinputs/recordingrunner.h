// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef RECORDINGRUNNER_H
#define RECORDINGRUNNER_H

#include <QMediaRecorder>
#include <QMediaCaptureSession>
#include <QUrl>

#include "mediagenerator.h"
#include "pushmodemediasource.h"
#include "mediaframeinputqueue.h"

#include <optional>

class RecordingRunner : public QObject
{
    Q_OBJECT
public:
    ~RecordingRunner() override;

    virtual void run(const QUrl &outputLocation = {});

    const QMediaRecorder &recorder() const { return m_recorder; }

protected:
    RecordingRunner();

    QMediaCaptureSession &session() { return m_session; }

private slots:
    void handleRecorderStateChanged(QMediaRecorder::RecorderState state);

    void handleError(QMediaRecorder::Error error, QString description);

signals:
    void finished();

private:
    QMediaCaptureSession m_session;
    QMediaRecorder m_recorder;
};

class PullModeRecordingRunner : public RecordingRunner
{
public:
    PullModeRecordingRunner(const AudioGeneratorSettingsOpt &audioGenerationSettings,
                            const VideoGeneratorSettingsOpt &videoGenerationSettings);

private:
    void sendNextAudioBuffer();

    void sendNextVideoFrame();

private:
    std::unique_ptr<AudioGenerator> m_audioGenerator;
    std::unique_ptr<VideoGenerator> m_videoGenerator;

    std::unique_ptr<QAudioBufferInput> m_audioInput;
    std::unique_ptr<QVideoFrameInput> m_videoInput;
};

struct PushModeSettings
{
    qreal producingRate = 1.;
    std::uint32_t maxQueueSize = 5;
};

using PushModeSettingsOpt = std::optional<PushModeSettings>;

class PushModeRecordingRunner : public RecordingRunner
{
public:
    PushModeRecordingRunner(const AudioGeneratorSettingsOpt &audioGenerationSettings,
                            const VideoGeneratorSettingsOpt &videoGenerationSettings,
                            const PushModeSettings &pushModeSettings);

    ~PushModeRecordingRunner();

    void run(const QUrl &outputLocation = {}) override;

private:
    void onFinished();

private:
    using AudioBufferSource = PushModeFrameSource<AudioGenerator>;
    using VideoFrameSource = PushModeFrameSource<VideoGenerator>;
    using VideoFrameInputQueue = MediaFrameInputQueue<VideoFrameInputQueueTraits>;
    using AudioBufferInputQueue = MediaFrameInputQueue<AudioBufferInputQueueTraits>;

    std::unique_ptr<AudioBufferSource> m_audioBufferSource;
    std::unique_ptr<VideoFrameSource> m_videoFrameSource;

    std::unique_ptr<VideoFrameInputQueue> m_videoFrameInputQueue;
    std::unique_ptr<AudioBufferInputQueue> m_audioBufferInputQueue;
};

#endif // RECORDINGRUNNER_H
