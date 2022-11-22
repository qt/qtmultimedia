// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGDECODER_P_H
#define QFFMPEGDECODER_P_H

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
#include "qffmpegmediaplayer_p.h"
#include "qffmpeghwaccel_p.h"
#include "qffmpegclock_p.h"
#include "qaudiobuffer.h"
#include "qffmpegresampler_p.h"

#include <private/qmultimediautils_p.h>
#include <qshareddata.h>
#include <qtimer.h>
#include <qqueue.h>

QT_BEGIN_NAMESPACE

class QAudioSink;
class QFFmpegAudioDecoder;
class QFFmpegMediaPlayer;

namespace QFFmpeg
{

class Resampler;

// queue up max 16M of encoded data, that should always be enough
// (it's around 2 secs of 4K HDR video, longer for almost all other formats)
enum { MaxQueueSize = 16*1024*1024 };

struct Packet
{
    struct Data {
        Data(AVPacket *p)
            : packet(p)
        {}
        ~Data() {
            if (packet)
                av_packet_free(&packet);
        }
        QAtomicInt ref;
        AVPacket *packet = nullptr;
    };
    Packet() = default;
    Packet(AVPacket *p)
        : d(new Data(p))
    {}

    bool isValid() const { return !!d; }
    AVPacket *avPacket() const { return d->packet; }
private:
    QExplicitlySharedDataPointer<Data> d;
};

struct Codec
{
    struct AVCodecFreeContext { void operator()(AVCodecContext *ctx) { avcodec_free_context(&ctx); } };
    using UniqueAVCodecContext = std::unique_ptr<AVCodecContext, AVCodecFreeContext>;
    struct Data {
        Data(UniqueAVCodecContext &&context, AVStream *stream, std::unique_ptr<QFFmpeg::HWAccel> &&hwAccel);
        ~Data();
        QAtomicInt ref;
        UniqueAVCodecContext context;
        AVStream *stream = nullptr;
        std::unique_ptr<QFFmpeg::HWAccel> hwAccel;
    };

    static QMaybe<Codec> create(AVStream *);

    AVCodecContext *context() const { return d->context.get(); }
    AVStream *stream() const { return d->stream; }
    uint streamIndex() const { return d->stream->index; }
    HWAccel *hwAccel() const { return d->hwAccel.get(); }
    qint64 toMs(qint64 ts) const { return timeStampMs(ts, d->stream->time_base).value_or(0); }
    qint64 toUs(qint64 ts) const { return timeStampUs(ts, d->stream->time_base).value_or(0); }

private:
    Codec(Data *data) : d(data) {}
    QExplicitlySharedDataPointer<Data> d;
};


struct Frame
{
    struct Data {
        Data(AVFrameUPtr f, const Codec &codec, qint64 pts)
            : codec(codec)
            , frame(std::move(f))
            , pts(pts)
        {}
        Data(const QString &text, qint64 pts, qint64 duration)
            : text(text), pts(pts), duration(duration)
        {}

        QAtomicInt ref;
        std::optional<Codec> codec;
        AVFrameUPtr frame;
        QString text;
        qint64 pts = -1;
        qint64 duration = -1;
    };
    Frame() = default;
    Frame(AVFrameUPtr f, const Codec &codec, qint64 pts)
        : d(new Data(std::move(f), codec, pts))
    {}
    Frame(const QString &text, qint64 pts, qint64 duration)
        : d(new Data(text, pts, duration))
    {}
    bool isValid() const { return !!d; }

    AVFrame *avFrame() const { return d->frame.get(); }
    AVFrameUPtr takeAVFrame() { return std::move(d->frame); }
    const Codec *codec() const { return d->codec ? &d->codec.value() : nullptr; }
    qint64 pts() const { return d->pts; }
    qint64 duration() const { return d->duration; }
    qint64 end() const { return d->pts + d->duration; }
    QString text() const { return d->text; }
private:
    QExplicitlySharedDataPointer<Data> d;
};

class Demuxer;
class StreamDecoder;
class Renderer;
class AudioRenderer;
class VideoRenderer;

class Decoder : public QObject
{
    Q_OBJECT
public:
    Decoder();
    ~Decoder();

    void setMedia(const QUrl &media, QIODevice *stream);

    void init();
    void setState(QMediaPlayer::PlaybackState state);
    void play() {
        setState(QMediaPlayer::PlayingState);
    }
    void pause() {
        setState(QMediaPlayer::PausedState);
    }
    void stop() {
        setState(QMediaPlayer::StoppedState);
    }

    void triggerStep();

    void setVideoSink(QVideoSink *sink);
    void setAudioSink(QPlatformAudioOutput *output);

    void changeAVTrack(QPlatformMediaPlayer::TrackType type);

    void seek(qint64 pos);
    void setPlaybackRate(float rate);

    int activeTrack(QPlatformMediaPlayer::TrackType type);
    void setActiveTrack(QPlatformMediaPlayer::TrackType type, int streamNumber);

    bool isSeekable() const
    {
        return m_isSeekable;
    }

signals:
    void endOfStream();
    void errorOccured(int error, const QString &errorString);
    void positionChanged(qint64 time);

public slots:
    void streamAtEnd();

public:
    struct StreamInfo {
        int avStreamIndex = -1;
        bool isDefault = false;
        QMediaMetaData metaData;
    };

    // Accessed from multiple threads, but API is threadsafe
    ClockController clockController;

private:
    void setPaused(bool b);

protected:
    friend QFFmpegMediaPlayer;

    QMediaPlayer::PlaybackState m_state = QMediaPlayer::StoppedState;
    bool m_isSeekable = false;

    Demuxer *demuxer = nullptr;
    QVideoSink *videoSink = nullptr;
    Renderer *videoRenderer = nullptr;
    QPlatformAudioOutput *audioOutput = nullptr;
    Renderer *audioRenderer = nullptr;

    QList<StreamInfo> m_streamMap[QPlatformMediaPlayer::NTrackTypes];
    int m_requestedStreams[QPlatformMediaPlayer::NTrackTypes] = { -1, -1, -1 };
    qint64 m_duration = 0;
    QMediaMetaData m_metaData;

    int avStreamIndex(QPlatformMediaPlayer::TrackType type)
    {
        int i = m_requestedStreams[type];
        return i < 0 || i >= m_streamMap[type].size() ? -1 : m_streamMap[type][i].avStreamIndex;
    }
};

class Demuxer : public Thread
{
    Q_OBJECT
public:
    Demuxer(Decoder *decoder, AVFormatContext *context);
    ~Demuxer();

    StreamDecoder *addStream(int streamIndex);
    void removeStream(int streamIndex);

    bool isStopped() const
    {
        return m_isStopped.loadRelaxed();
    }
    void startDecoding()
    {
        m_isStopped.storeRelaxed(false);
        updateEnabledStreams();
        wake();
    }
    void stopDecoding();

    int seek(qint64 pos);

private:
    void updateEnabledStreams();
    void sendFinalPacketToStreams();

    void init() override;
    void cleanup() override;
    bool shouldWait() const override;
    void loop() override;

    Decoder *decoder;
    AVFormatContext *context = nullptr;
    QList<StreamDecoder *> streamDecoders;

    QAtomicInteger<bool> m_isStopped = true;
    qint64 last_pts = -1;
};


class StreamDecoder : public Thread
{
    Q_OBJECT
protected:
    Demuxer *demuxer = nullptr;
    Renderer *m_renderer = nullptr;

    struct PacketQueue {
        mutable QMutex mutex;
        QQueue<Packet> queue;
        qint64 size = 0;
        qint64 duration = 0;
    };
    PacketQueue packetQueue;

    struct FrameQueue {
        mutable QMutex mutex;
        QQueue<Frame> queue;
        int maxSize = 3;
    };
    FrameQueue frameQueue;
    QAtomicInteger<bool> eos = false;
    bool decoderHasNoFrames = false;

public:
    StreamDecoder(Demuxer *demuxer, const Codec &codec);

    void addPacket(AVPacket *packet);

    qint64 queuedPacketSize() const {
        QMutexLocker locker(&packetQueue.mutex);
        return packetQueue.size;
    }
    qint64 queuedDuration() const {
        QMutexLocker locker(&packetQueue.mutex);
        return packetQueue.duration;
    }

    const Frame *lockAndPeekFrame()
    {
        frameQueue.mutex.lock();
        return frameQueue.queue.isEmpty() ? nullptr : &frameQueue.queue.first();
    }
    void removePeekedFrame()
    {
        frameQueue.queue.takeFirst();
        wake();
    }
    void unlockAndReleaseFrame()
    {
        frameQueue.mutex.unlock();
    }
    Frame takeFrame();

    void flush();

    Codec codec;

    void setRenderer(Renderer *r);
    Renderer *renderer() const { return m_renderer; }

    bool isAtEnd() const { return eos.loadAcquire(); }

    void killHelper() override;

private:
    Packet takePacket();
    Packet peekPacket();

    void addFrame(const Frame &f);

    bool hasEnoughFrames() const
    {
        QMutexLocker locker(&frameQueue.mutex);
        return frameQueue.queue.size() >= frameQueue.maxSize;
    }
    bool hasNoPackets() const
    {
        QMutexLocker locker(&packetQueue.mutex);
        return packetQueue.queue.isEmpty();
    }

    void init() override;
    bool shouldWait() const override;
    void loop() override;

    void decode();
    void decodeSubtitle();

    QPlatformMediaPlayer::TrackType type() const;
};

class Renderer : public Thread
{
    Q_OBJECT
protected:
    QPlatformMediaPlayer::TrackType type;

    bool step = false;
    bool paused = true;
    StreamDecoder *streamDecoder = nullptr;
    QAtomicInteger<bool> eos = false;

public:
    Renderer(QPlatformMediaPlayer::TrackType type);

    void setPaused(bool p) {
        QMutexLocker locker(&mutex);
        paused = p;
        if (!p)
            wake();
    }
    void singleStep() {
        QMutexLocker locker(&mutex);
        if (!paused)
            return;
        step = true;
        wake();
    }
    void doneStep() {
        step = false;
    }
    bool isAtEnd() { return !streamDecoder || eos.loadAcquire(); }

    void setStream(StreamDecoder *stream);
    virtual void setSubtitleStream(StreamDecoder *) {}

    void killHelper() override;

    virtual void streamChanged() {}

Q_SIGNALS:
    void atEnd();

protected:
    bool shouldWait() const override;

public:
};

class ClockedRenderer : public Renderer, public Clock
{
public:
    ClockedRenderer(Decoder *decoder, QPlatformMediaPlayer::TrackType type)
        : Renderer(type)
        , Clock(&decoder->clockController)
    {
    }
    ~ClockedRenderer()
    {
    }
    void setPaused(bool paused) override;
};

class VideoRenderer : public ClockedRenderer
{
    Q_OBJECT

    StreamDecoder *subtitleStreamDecoder = nullptr;
public:
    VideoRenderer(Decoder *decoder, QVideoSink *sink);

    void killHelper() override;

    void setSubtitleStream(StreamDecoder *stream) override;
private:

    void init() override;
    void loop() override;

    QVideoSink *sink;
};

class AudioRenderer : public ClockedRenderer
{
    Q_OBJECT
public:
    AudioRenderer(Decoder *decoder, QAudioOutput *output);
    ~AudioRenderer() = default;

    // Clock interface
    void syncTo(qint64 usecs) override;
    void setPlaybackRate(float rate, qint64 currentTime) override;

private slots:
    void updateAudio();
    void setSoundVolume(float volume);

private:
    void updateOutput(const Codec *codec);
    void initResempler(const Codec *codec);
    void freeOutput();

    void init() override;
    void cleanup() override;
    void loop() override;
    void streamChanged() override;
    Type type() const override { return AudioClock; }

    int outputSamples(int inputSamples) {
        return qRound(inputSamples/playbackRate());
    }

    // Used for timing update calculations based on processed data
    qint64 audioBaseTime = 0;
    qint64 processedBase = 0;
    qint64 processedUSecs = 0;

    bool deviceChanged = false;
    QAudioOutput *output = nullptr;
    qint64 writtenUSecs = 0;
    qint64 latencyUSecs = 0;

    QAudioFormat format;
    QAudioSink *audioSink = nullptr;
    QIODevice *audioDevice = nullptr;
    std::unique_ptr<Resampler> resampler;
    QAudioBuffer bufferedData;
    qsizetype bufferWritten = 0;
};

}

QT_END_NAMESPACE

#endif

