// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CUSTOMMEDIAINPUTSNIPPETS_H
#define CUSTOMMEDIAINPUTSNIPPETS_H

#include <QAudioBufferInput>
#include <QVideoFrameInput>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QVideoFrame>
#include <QAudioBuffer>
#include <QTimer>
#include <QCoreApplication>

class MediaGenerator : public QObject
{
    Q_OBJECT

public slots:
    void nextVideoFrame();
    void nextAudioBuffer();

signals:
    void videoFrameReady(const QVideoFrame &frame);
    void audioBufferReady(const QAudioBuffer &buffer);

private:
    QVideoFrame nextFrame();
    QAudioBuffer nextBuffer();
};

class CustomMediaInputSnippets : public QObject
{
    Q_OBJECT

public:
    void setupAndRecordVideo();
    void setupAndRecordAudio();
};

#endif // CUSTOMMEDIAINPUTSNIPPETS_H
