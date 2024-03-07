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
#include "qffmpegencodingformatcontext_p.h"

#include "private/qmultimediautils_p.h"

#include <private/qplatformmediarecorder_p.h>
#include <qaudioformat.h>
#include <qaudiobuffer.h>
#include <qmediarecorder.h>

#include <queue>

QT_BEGIN_NAMESPACE

class QFFmpegAudioInput;
class QVideoFrame;
class QPlatformVideoSource;

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
    EncodingFinalizer(Encoder *e);

    void run() override;

private:
    Encoder *m_encoder = nullptr;
};

class Encoder : public QObject
{
    Q_OBJECT
public:
    Encoder(const QMediaEncoderSettings &settings, const QString &filePath);
    ~Encoder();

    void addAudioInput(QFFmpegAudioInput *input);
    void addVideoSource(QPlatformVideoSource *source);

    void start();
    void finalize();

    void setPaused(bool p);

    void setMetaData(const QMediaMetaData &metaData);

public Q_SLOTS:
    void newTimeStamp(qint64 time);

Q_SIGNALS:
    void durationChanged(qint64 duration);
    void error(QMediaRecorder::Error code, const QString &description);
    void finalizationDone();

private:
    template<typename... Args>
    void addMediaFrameHandler(Args &&...args);

private:
    // TODO: improve the encasulation
    friend class EncodingFinalizer;
    friend class AudioEncoder;
    friend class VideoEncoder;
    friend class Muxer;

    QMediaEncoderSettings m_settings;
    QMediaMetaData m_metaData;
    EncodingFormatContext m_formatContext;
    Muxer *m_muxer = nullptr;

    AudioEncoder *m_audioEncoder = nullptr;
    QList<VideoEncoder *> m_videoEncoders;
    QList<QMetaObject::Connection> m_connections;

    QMutex m_timeMutex;
    qint64 m_timeRecorded = 0;

    bool m_isHeaderWritten = false;
};


class Muxer : public ConsumerThread
{
public:
    Muxer(Encoder *encoder);

    void addPacket(AVPacketUPtr packet);

private:
    AVPacketUPtr takePacket();

    void init() override;
    void cleanup() override;
    bool hasData() const override;
    void processOne() override;

private:
    mutable QMutex m_queueMutex;
    std::queue<AVPacketUPtr> m_packetQueue;

    Encoder *m_encoder;
};

class EncoderThread : public ConsumerThread
{
public:
    EncoderThread(Encoder *encoder) : m_encoder(encoder) { }
    virtual void setPaused(bool b) { m_paused.storeRelease(b); }

protected:
    QAtomicInteger<bool> m_paused = false;
    Encoder *m_encoder = nullptr;
};

class AudioEncoder : public EncoderThread
{
public:
    AudioEncoder(Encoder *encoder, QFFmpegAudioInput *input, const QMediaEncoderSettings &settings);

    void open();
    void addBuffer(const QAudioBuffer &buffer);

    QFFmpegAudioInput *audioInput() const { return m_input; }

private:
    QAudioBuffer takeBuffer();
    void retrievePackets();

    void init() override;
    void cleanup() override;
    bool hasData() const override;
    void processOne() override;

private:
    mutable QMutex m_queueMutex;
    std::queue<QAudioBuffer> m_audioBufferQueue;

    AVStream *m_stream = nullptr;
    AVCodecContextUPtr m_codecContext;
    QFFmpegAudioInput *m_input = nullptr;
    QAudioFormat m_format;

    SwrContextUPtr m_resampler;
    qint64 m_samplesWritten = 0;
    const AVCodec *m_avCodec = nullptr;
    QMediaEncoderSettings m_settings;
};

class VideoEncoder : public EncoderThread
{
public:
    VideoEncoder(Encoder *encoder, const QMediaEncoderSettings &settings,
                 const QVideoFrameFormat &format, std::optional<AVPixelFormat> hwFormat);
    ~VideoEncoder() override;

    bool isValid() const;

    void addFrame(const QVideoFrame &frame);

    void setPaused(bool b) override
    {
        EncoderThread::setPaused(b);
        if (b)
            m_baseTime.storeRelease(-1);
    }

private:
    QVideoFrame takeFrame();
    void retrievePackets();

    void init() override;
    void cleanup() override;
    bool hasData() const override;
    void processOne() override;

private:
    mutable QMutex m_queueMutex;
    std::queue<QVideoFrame> m_videoFrameQueue;
    const size_t m_maxQueueSize = 10; // Arbitrarily chosen to limit memory usage (332 MB @ 4K)

    std::unique_ptr<VideoFrameEncoder> m_frameEncoder;
    QAtomicInteger<qint64> m_baseTime = std::numeric_limits<qint64>::min();
    qint64 m_lastFrameTime = 0;
};
}

QT_END_NAMESPACE

#endif
