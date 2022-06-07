// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGENCODER_P_H
#define QFFMPEGENCODER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qffmpegthread_p.h"
#include "qffmpeg_p.h"
#include "qffmpeghwaccel_p.h"

#include <private/qplatformmediarecorder_p.h>
#include <qaudioformat.h>
#include <qaudiobuffer.h>

#include <qqueue.h>

QT_BEGIN_NAMESPACE

class QFFmpegAudioInput;
class QVideoFrame;
class QPlatformCamera;

namespace QFFmpeg
{

class Encoder;
class Muxer;
class AudioEncoder;
class VideoEncoder;
class VideoFrameEncoder;

class EncodingFinalizer : public QThread
{
public:
    EncodingFinalizer(Encoder *e)
        : encoder(e)
    {}
    void run() override;

    Encoder *encoder = nullptr;
};

class Encoder : public QObject
{
    Q_OBJECT
public:
    Encoder(const QMediaEncoderSettings &settings, const QUrl &url);
    ~Encoder();

    void addAudioInput(QFFmpegAudioInput *input);
    void addVideoSource(QPlatformCamera *source);

    void start();
    void finalize();

    void setPaused(bool p);

    void setMetaData(const QMediaMetaData &metaData);

public Q_SLOTS:
    void newAudioBuffer(const QAudioBuffer &buffer);
    void newVideoFrame(const QVideoFrame &frame);
    void newTimeStamp(qint64 time);

Q_SIGNALS:
    void durationChanged(qint64 duration);
    void error(QMediaRecorder::Error code, const QString &description);
    void finalizationDone();

public:

    QMediaEncoderSettings settings;
    QMediaMetaData metaData;
    AVFormatContext *formatContext = nullptr;
    Muxer *muxer = nullptr;
    bool isRecording = false;

    AudioEncoder *audioEncode = nullptr;
    VideoEncoder *videoEncode = nullptr;

    QMutex timeMutex;
    qint64 timeRecorded = 0;
};


class Muxer : public Thread
{
    mutable QMutex queueMutex;
    QQueue<AVPacket *> packetQueue;
public:
    Muxer(Encoder *encoder);

    void addPacket(AVPacket *);

private:
    AVPacket *takePacket();

    void init() override;
    void cleanup() override;
    bool shouldWait() const override;
    void loop() override;

    Encoder *encoder;
};

class EncoderThread : public Thread
{
public:
    virtual void setPaused(bool b)
    {
        paused.storeRelease(b);
    }

protected:
    QAtomicInteger<bool> paused = false;
    Encoder *encoder = nullptr;
};

class AudioEncoder : public EncoderThread
{
    mutable QMutex queueMutex;
    QQueue<QAudioBuffer> audioBufferQueue;
public:
    AudioEncoder(Encoder *encoder, QFFmpegAudioInput *input, const QMediaEncoderSettings &settings);

    void addBuffer(const QAudioBuffer &buffer);

    QFFmpegAudioInput *audioInput() const { return input; }

private:
    QAudioBuffer takeBuffer();
    void retrievePackets();

    void init() override;
    void cleanup() override;
    bool shouldWait() const override;
    void loop() override;

    AVStream *stream = nullptr;
    AVCodecContext *codec = nullptr;
    QFFmpegAudioInput *input;
    QAudioFormat format;

    SwrContext *resampler = nullptr;
    qint64 samplesWritten = 0;
};


class VideoEncoder : public EncoderThread
{
    mutable QMutex queueMutex;
    QQueue<QVideoFrame> videoFrameQueue;
public:
    VideoEncoder(Encoder *encoder, QPlatformCamera *camera, const QMediaEncoderSettings &settings);
    ~VideoEncoder();

    void addFrame(const QVideoFrame &frame);

    void setPaused(bool b) override
    {
        EncoderThread::setPaused(b);
        if (b)
            baseTime.storeRelease(-1);
    }

private:
    QVideoFrame takeFrame();
    void retrievePackets();

    void init() override;
    void cleanup() override;
    bool shouldWait() const override;
    void loop() override;

    QMediaEncoderSettings m_encoderSettings;
    QPlatformCamera *m_camera = nullptr;
    VideoFrameEncoder *frameEncoder = nullptr;

    QAtomicInteger<qint64> baseTime = -1;
    qint64 lastFrameTime = 0;
};

}

QT_END_NAMESPACE

#endif
