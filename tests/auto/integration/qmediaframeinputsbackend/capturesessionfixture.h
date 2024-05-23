// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CAPTURESESSIONFIXTURE_H
#define CAPTURESESSIONFIXTURE_H

#include "framegenerator.h"
#include <QtMultimedia/qvideoframeinput.h>
#include <QtMultimedia/qaudioinput.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qaudiobufferinput.h>
#include <QtCore/qtemporaryfile.h>

#include <../shared/testvideosink.h>
#include <QtTest/qsignalspy.h>

QT_BEGIN_NAMESPACE

enum class StreamType { Audio, Video, AudioAndVideo };
enum class AutoStop { EmitEmpty, No };

struct CaptureSessionFixture
{
    explicit CaptureSessionFixture(StreamType streamType, AutoStop autoStop);
    ~CaptureSessionFixture();

    void connectPullMode();
    bool waitForRecorderStopped(milliseconds duration);
    bool hasAudio() const;
    bool hasVideo() const;

    VideoGenerator m_videoGenerator;
    AudioGenerator m_audioGenerator;
    QVideoFrameInput m_videoInput;
    QAudioBufferInput m_audioInput;
    QMediaCaptureSession m_session;
    QMediaRecorder m_recorder;
    QTemporaryFile m_tempFile;
    StreamType m_streamType = StreamType::Video;

    QSignalSpy readyToSendVideoFrame{ &m_videoInput, &QVideoFrameInput::readyToSendVideoFrame };
    QSignalSpy readyToSendAudioBuffer{ &m_audioInput, &QAudioBufferInput::readyToSendAudioBuffer };
    QSignalSpy recorderStateChanged{ &m_recorder, &QMediaRecorder::recorderStateChanged };
};

QT_END_NAMESPACE

#endif
