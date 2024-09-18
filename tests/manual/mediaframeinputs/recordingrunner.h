// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef RECORDINGRUNNER_H
#define RECORDINGRUNNER_H

#include <QMediaRecorder>
#include <QMediaCaptureSession>
#include <QUrl>

#include <optional>

#include "mediagenerator.h"

QT_BEGIN_NAMESPACE
class QVideoFrameInput;
class QAudioBufferInput;
QT_END_NAMESPACE

class RecordingRunner : public QObject
{
    Q_OBJECT
public:
    RecordingRunner(const std::optional<AudioGenerator::Settings> &audioGenerationSettings = {},
                    const std::optional<VideoGenerator::Settings> &videoGenerationSettings = {});

    ~RecordingRunner() override;

    void run(const QUrl &outputLocation = {});

    const QMediaRecorder &recorder() const { return m_recorder; }

private slots:
    void handleRecorderStateChanged(QMediaRecorder::RecorderState state);

    void handleError(QMediaRecorder::Error error, QString description);

    void sendNextAudioData();

    void sendNextVideoData();

signals:
    void finished();

private:
    QMediaCaptureSession m_session;
    QMediaRecorder m_recorder;

    std::unique_ptr<QAudioBufferInput> m_audioInput;
    std::unique_ptr<QVideoFrameInput> m_videoInput;

    std::unique_ptr<AudioGenerator> m_audioGenerator;
    std::unique_ptr<VideoGenerator> m_videoGenerator;
};

#endif // RECORDINGRUNNER_H
