/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
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

#include <private/qtmultimediaglobal_p.h>
#include "qffmpeg_p.h"
#include "qffmpegmediaplayer_p.h"

#include <qmutex.h>
#include <qwaitcondition.h>
#include <qthread.h>
#include <qtimer.h>
#include <qqueue.h>

QT_BEGIN_NAMESPACE

class QAudioSink;

namespace QFFmpeg
{

// queue up max 16M of encoded data, that should always be enough
// (it's around 3.5 secs of 4K h264, longer for almost all other formats)
enum { MaxQueueSize = 16*1024*1024 };

inline qint64 timeStamp(qint64 ts, AVRational base)
{
    return (1000*ts*base.num + 500)/base.den;
}


struct Subtitle {
    QString text;
    qint64 start;
    qint64 end;
};

struct SubtitleQueue
{
    QMutex mutex;
    QQueue<Subtitle *> queue;

    void enqueue(Subtitle *s) {
        Q_ASSERT(s);
        QMutexLocker locker(&mutex);
        queue.enqueue(s);
    }
    Subtitle *peek() {
        QMutexLocker locker(&mutex);
        return queue.isEmpty() ? nullptr : queue.first();
    }
    Subtitle *dequeue() {
        QMutexLocker locker(&mutex);
        return queue.isEmpty() ? nullptr : queue.dequeue();
    }
    void clear() {
        QMutexLocker locker(&mutex);
        queue.clear();
    }
};

class DemuxerThread;
class DecoderThread;
class RendererThread;
class DecoderThread;

struct Pipeline {
    QPlatformMediaPlayer::TrackType type = QPlatformMediaPlayer::NTrackTypes; // Audio or Video
    //    int streamIndex = -1; // FFmpeg stream index
    DecoderThread *decoder = nullptr;
    RendererThread *renderer = nullptr;

    void createVideoPipeline(QFFmpegDecoder *d, QVideoSink *sink);
    void createAudioPipeline(QFFmpegDecoder *d, QAudioOutput *sink);
    void destroy();
};

class QFFmpegDecoder : public QObject
{
public:
    QFFmpegDecoder(QFFmpegMediaPlayer *p);

    void init();
    void play() {
        init();
        setPaused(false);
    }
    void pause() {
        setPaused(true);
    }
    void setPaused(bool b);
    void triggerStep();
    void syncClocks();

    void pausePipeline(bool paused);

    AVStream *stream(QPlatformMediaPlayer::TrackType type) {
        int index = m_currentStream[type];
        return index < 0 ? nullptr : context->streams[index];
    }

    void setVideoSink(QVideoSink *sink);
    void updateVideo();

    void setAudioSink(QPlatformAudioOutput *output);
    void updateAudio();

    void changeTrack(QPlatformMediaPlayer::TrackType type, int index);

    void startDemuxer();
    bool openCodec(QPlatformMediaPlayer::TrackType type, int index = -1);
    void closeCodec(QPlatformMediaPlayer::TrackType type);

    int getDefaultStream(QPlatformMediaPlayer::TrackType type);

    void seek(qint64 pos);

public:
    QFFmpegMediaPlayer *player = nullptr;

    QMutex mutex;
    QWaitCondition condition;

    bool paused = true;

    DemuxerThread *demuxer = nullptr;
    QAtomicInteger<qint64> seek_pos = -1;

    static constexpr int NPipelines = 2;
    Pipeline pipelines[NPipelines];

    AVFormatContext *context = nullptr;
    int m_currentStream[QPlatformMediaPlayer::NTrackTypes] = { -1, -1, -1 };
    AVCodecContext *codecContext[QPlatformMediaPlayer::NTrackTypes] = {};

    QVideoSink *videoSink = nullptr;
    QPlatformAudioOutput *audioOutput = nullptr;

    SubtitleQueue subtitleQueue;

    QElapsedTimer baseTimer;
    qint64 currentTime = 0;

    bool playing = false;
};

class Thread : public QThread
{
public:
    QMutex mutex;
    QWaitCondition condition;
private:
    bool pauseRequested = false;
    bool step = false;

    QAtomicInteger<bool> paused = false;

protected:
    QFFmpegDecoder *data;
    QAtomicInteger<bool> exit = false;
    bool eos = false;

public:
    Thread(QFFmpegDecoder *parent)
        : QThread(parent)
        , data(parent)
    {}

    // public API is thread-safe

    void kill() {
        exit.storeRelaxed(true);
        deleteLater();
    }

    bool atEnd() const { return eos; }

    void requestPause() {
        QMutexLocker locker(&mutex);
        qDebug() << "XXX" << this << "request pause";
        pauseRequested = true;
    }
    void requestUnPause() {
        {
            QMutexLocker locker(&mutex);
            pauseRequested = false;
        }
        condition.wakeAll();
    }
    void requestSingleStep() {
        {
            QMutexLocker locker(&mutex);
            if (!pauseRequested)
                return;
            step = true;
        }
        condition.wakeAll();
    }

    void wake() {
        condition.wakeAll();
    }

    // use from main thread, while data->mutex is locked
    bool isPaused() {
        return paused;
    }

protected:
    virtual void init() {}
    virtual void cleanup() {}
    // loop() should never block, all blocking has to happen in shouldWait()
    virtual void loop() = 0;
    virtual bool shouldWait() { return false; }

    bool isPauseRequested() { return pauseRequested; }

private:
    bool checkPaused();
    void maybePause();

    void run() override;
};


class DemuxerThread : public Thread
{
public:
    DemuxerThread(QFFmpegDecoder *decoder)
        : Thread(decoder)
    {
    }

private:
    void init() override;
    void cleanup() override;
    bool shouldWait() override;
    void loop() override;

    void doSeek(qint64 pos, qint64 offset);
    void queuePacket();

    void decodeSubtitle(AVStream *stream, AVCodecContext *codec, AVPacket *packet);

    AVPacket packet;
    qint64 seekPos = -1;
    qint64 seekOffset = 0;
};


class DecoderThread : public Thread
{
    QQueue<AVPacket *> queue;
    qint64 seekTo = -1;
public:
    qint64 queuedPacketSize = 0;
    qint64 queuedDuration = 0;

public:
    DecoderThread(QFFmpegDecoder *decoder, QPlatformMediaPlayer::TrackType t);

    void enqueue(AVPacket *packet);

    void flushAndSeek(qint64 seek);

private:
    void waitForPacket() {
        mutex.lock();
        condition.wait(&mutex);
        mutex.unlock();
    }
    AVPacket *peek() {
        QMutexLocker locker(&mutex);
        return queue.isEmpty() ? nullptr : queue.first();
    }
    AVPacket *dequeue(qint64 *seek);

private:
    void init() override;
    bool shouldWait() override;
    void loop() override;

    QPlatformMediaPlayer::TrackType type;
    qint64 seek = -1;
    const QLoggingCategory &cat;
};


class RendererThread : public Thread
{
    QQueue<AVFrame *> queue;
    int maxSize = 0;
    qint64 seekTo = -1;
    QPlatformMediaPlayer::TrackType type;
public:

    void enqueue(AVFrame *f) {
        Q_ASSERT(f);
        QMutexLocker locker(&mutex);
        queue.enqueue(f);
        condition.wakeAll();
    }
    bool hasEnoughFrames() {
        QMutexLocker locker(&mutex);
        return queue.size() >= maxSize;
    }
    void waitForSpace() {
        condition.wakeAll();
        mutex.lock();
        while (queue.size() >= maxSize)
            condition.wait(&mutex);
        mutex.unlock();
    }
    void flushAndSeek(qint64 seek) {
        QMutexLocker locker(&mutex);
        queue.clear();
        seekTo = seek;
    }
protected:
    AVFrame *peek(qint64 *seek) {
        QMutexLocker locker(&mutex);
        *seek = seekTo;
        return queue.isEmpty() ? nullptr : queue.first();
    }
    AVFrame *dequeue(qint64 *seek);
    void waitForFrame() {
        condition.wakeAll();
        mutex.lock();
        while (queue.size() == 0)
            condition.wait(&mutex);
        mutex.unlock();
    }
protected:
    qint64 pts_base = 0;
public:
    // Queue size: 3 frames for video, 9 for audio
    RendererThread(QFFmpegDecoder *decoder, QPlatformMediaPlayer::TrackType type)
        : Thread(decoder)
        , maxSize(type == QPlatformMediaPlayer::AudioStream ? 9 : 3)
        , type(type)
    {
    }

    void syncClock(qint64 base)
    {
        pts_base = base;
    }
};

class VideoRendererThread : public RendererThread
{
public:
    VideoRendererThread(QFFmpegDecoder *decoder, QVideoSink *sink);

private:
    void init() override;
    void loop() override;

    QVideoSink *sink;
};

class AudioRendererThread : public RendererThread
{
public:
    AudioRendererThread(QFFmpegDecoder *decoder, QAudioOutput *output);
    ~AudioRendererThread() = default;

private slots:
    void outputDeviceChanged();

private:
    void updateOutput();
    void freeOutput();

    void init() override;
    void cleanup() override;
    void loop() override;

    bool deviceChanged = false;
    QAudioOutput *output = nullptr;

    QAudioSink *audioSink = nullptr;
    QIODevice *audioDevice = nullptr;
    SwrContext *resampler = nullptr;
};

}

QT_END_NAMESPACE

#endif

