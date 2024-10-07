// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "custommediainputsnippets.h"

void CustomMediaInputSnippets::setupAndRecordVideo()
{
    //! [QVideoFrameInput setup]
    QMediaCaptureSession session;
    QMediaRecorder recorder;
    QVideoFrameInput videoInput;

    session.setRecorder(&recorder);
    session.setVideoFrameInput(&videoInput);

    MediaGenerator generator; // Custom class providing video frames

    connect(&videoInput, &QVideoFrameInput::readyToSendVideoFrame,
            &generator, &MediaGenerator::nextVideoFrame);
    connect(&generator, &MediaGenerator::videoFrameReady,
            &videoInput, &QVideoFrameInput::sendVideoFrame);

    recorder.record();
    //! [QVideoFrameInput setup]

    // Start event loop here to keep objects alive and make the snippets runnable without crashing
    QTimer::singleShot(1000, qApp, &QCoreApplication::quit); // Close the app after 1 second
    qApp->exec();
}

void CustomMediaInputSnippets::setupAndRecordAudio()
{
    //! [QAudioBufferInput setup]
    QMediaCaptureSession session;
    QMediaRecorder recorder;
    QAudioBufferInput audioInput;

    session.setRecorder(&recorder);
    session.setAudioBufferInput(&audioInput);

    MediaGenerator generator; // Custom class providing audio buffers

    connect(&audioInput, &QAudioBufferInput::readyToSendAudioBuffer,
            &generator, &MediaGenerator::nextAudioBuffer);
    connect(&generator, &MediaGenerator::audioBufferReady,
            &audioInput, &QAudioBufferInput::sendAudioBuffer);

    recorder.record();
    //! [QAudioBufferInput setup]

    // Start event loop here to keep objects alive and make the snippets runnable without crashing
    QTimer::singleShot(1000, qApp, &QCoreApplication::quit); // Close the app after 1 second
    qApp->exec();
}

//! [nextVideoFrame()]
void MediaGenerator::nextVideoFrame()
{
    QVideoFrame frame = nextFrame();
    emit videoFrameReady(frame);
}
//! [nextVideoFrame()]

//! [nextAudioBuffer()]
void MediaGenerator::nextAudioBuffer()
{
    QAudioBuffer buffer = nextBuffer();
    emit audioBufferReady(buffer);
}
//! [nextAudioBuffer()]

QVideoFrame MediaGenerator::nextFrame()
{
    // Create mock video frame which is not interpretet as empty by the recorder
    QVideoFrameFormat format(QSize(1080, 720), QVideoFrameFormat::Format_NV12);
    QVideoFrame frame(format);
    return frame;
}

QAudioBuffer MediaGenerator::nextBuffer()
{
    // Create mock audio buffer which is not interpretet as empty by the recorder
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Float);
    int bufferSize = format.bytesPerSample() * 128;
    QByteArray byteArray(bufferSize, 0);
    return QAudioBuffer(byteArray, format);
}
