// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef RECORDINGRUNNER_H
#define RECORDINGRUNNER_H

#include <QMediaRecorder>
#include <QMediaCaptureSession>

#include "mediagenerator.h"
#include "pushmodemediasource.h"
#include "mediaframeinputqueue.h"
#include "settings.h"

class RecordingRunner : public QObject
{
    Q_OBJECT
public:
    ~RecordingRunner() override;

    virtual void run();

    const QMediaRecorder &recorder() const { return m_recorder; }

protected:
    RecordingRunner(const RecorderSettings &recorderSettings);

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
    PullModeRecordingRunner(const RecorderSettings &recorderSettings,
                            const AudioGeneratorSettingsOpt &audioGeneratorSettings,
                            const VideoGeneratorSettingsOpt &videoGeneratorSettings);

private:
    void sendNextAudioBuffer();

    void sendNextVideoFrame();

private:
    std::unique_ptr<AudioGenerator> m_audioGenerator;
    std::unique_ptr<VideoGenerator> m_videoGenerator;

    std::unique_ptr<QAudioBufferInput> m_audioInput;
    std::unique_ptr<QVideoFrameInput> m_videoInput;
};

class PushModeRecordingRunner : public RecordingRunner
{
public:
    PushModeRecordingRunner(const RecorderSettings &recorderSettings,
                            const AudioGeneratorSettingsOpt &audioGeneratorSettings,
                            const VideoGeneratorSettingsOpt &videoGeneratorSettings,
                            const PushModeSettings &pushModeSettings);

    ~PushModeRecordingRunner();

    void run() override;

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
