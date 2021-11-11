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

#include "qffmpegmediaplayer_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegmediadevices_p.h"
#include "private/qiso639_2_p.h"
#include "qffmpeg_p.h"
#include "qffmpegmediametadata_p.h"
#include "qffmpegvideobuffer_p.h"
#include "private/qplatformaudiooutput_p.h"
#include "qvideosink.h"
#include "qaudiosink.h"

#include <qlocale.h>
#include <qthread.h>
#include <qatomic.h>
#include <qwaitcondition.h>
#include <qmutex.h>
#include <qtimer.h>
#include <qqueue.h>

#include <qloggingcategory.h>

Q_LOGGING_CATEGORY(qLcDemuxer, "qt.multimedia.ffmpeg.demuxer")
Q_LOGGING_CATEGORY(qLcVideoDecoder, "qt.multimedia.ffmpeg.videoDecoder")
Q_LOGGING_CATEGORY(qLcVideoRenderer, "qt.multimedia.ffmpeg.videoRenderer")
Q_LOGGING_CATEGORY(qLcAudioDecoder, "qt.multimedia.ffmpeg.audioDecoder")
Q_LOGGING_CATEGORY(qLcAudioRenderer, "qt.multimedia.ffmpeg.audioRenderer")


static qint64 timeStamp(qint64 ts, AVRational base)
{
    return (1000*ts*base.num + 500)/base.den;
}

namespace {

struct PacketQueue
{
    QMutex mutex;
    QWaitCondition condition;
    QQueue<AVPacket *> queue;
    qint64 size = 0;
    qint64 duration = 0;
    qint64 seekTo = -1;

    void waitForPacket() {
        mutex.lock();
        condition.wait(&mutex);
        mutex.unlock();
    }
    void enqueue(AVPacket *packet) {
        Q_ASSERT(packet);
        AVPacket *p = av_packet_alloc();
        av_packet_move_ref(p, packet);
        {
            QMutexLocker locker(&mutex);
            queue.enqueue(p);
            size += p->size;
            duration += p->duration;
        }
        condition.wakeAll();
    }
    AVPacket *peek() {
        QMutexLocker locker(&mutex);
        return queue.isEmpty() ? nullptr : queue.first();
    }
    AVPacket *dequeue(qint64 *seek) {
        QMutexLocker locker(&mutex);
        if (queue.isEmpty())
            return nullptr;
        auto *p = queue.dequeue();
        size -= p->size;
        duration -= p->duration;
        *seek = seekTo;
        seekTo = -1;
        return p;
    }
    void flushAndSeek(qint64 seek) {
        QMutexLocker locker(&mutex);
        queue.clear();
        size = 0;
        duration = 0;
        seekTo = seek;
    }
};

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

struct FrameQueue
{
    FrameQueue(int size) : maxSize(size) {}
    QMutex mutex;
    QWaitCondition condition;
    QQueue<AVFrame *> queue;
    int maxSize = 0;
    bool flushed = false;

    void enqueue(AVFrame *f) {
        Q_ASSERT(f);
        QMutexLocker locker(&mutex);
        queue.enqueue(f);
    }
    AVFrame *peek() {
        QMutexLocker locker(&mutex);
        return queue.isEmpty() ? nullptr : queue.first();
    }
    AVFrame *dequeue(bool *wasFlushed) {
        QMutexLocker locker(&mutex);
        if (queue.isEmpty())
            return nullptr;
        *wasFlushed = flushed;
        flushed = false;
        return queue.dequeue();
    }
    void waitForFrame() {
        condition.wakeAll();
        mutex.lock();
        while (queue.size() == 0)
            condition.wait(&mutex);
        mutex.unlock();
    }
    void waitForSpace() {
        condition.wakeAll();
        mutex.lock();
        while (queue.size() >= maxSize)
            condition.wait(&mutex);
        mutex.unlock();
    }
    void wake() {
        condition.wakeAll();
    }
    void flush() {
        QMutexLocker locker(&mutex);
        queue.clear();
        flushed = true;
    }
};

}

class DemuxerThread;
class VideoDecoderThread;
class VideoRendererThread;
class AudioDecoderThread;
class AudioRendererThread;

class QFFmpegDecoder : public QObject
{
public:
    QFFmpegDecoder(QFFmpegMediaPlayer *p);

    QMutex mutex;
    QWaitCondition condition;

    void init()
    {
        if (demuxer)
            return;
        openCodec(QPlatformMediaPlayer::VideoStream);
        if (audioOutput) {
            openCodec(QPlatformMediaPlayer::AudioStream);
            changeAudioSink(audioOutput);
        }

        if (player->m_streamMap[QPlatformMediaPlayer::SubtitleStream].count())
            openCodec(QPlatformMediaPlayer::SubtitleStream);
        startDemuxer();
    }

    void play() {
        init();
        setPaused(false);
    }
    void pause() {
        setPaused(true);
    }
    void setPaused(bool b) {
        QMutexLocker locker(&rendererMutex);
        if (paused == b)
            return;
        paused = b;
        if (!b)
            pts_base = -1;
        locker.unlock();
        if (!b) {
            rendererCondition.wakeAll();
        }
    }
    void triggerSingleStep() {
        QMutexLocker locker(&rendererMutex);
        if (!paused)
            return;
        singleStep = true;
        locker.unlock();
        rendererCondition.wakeAll();
    }
    void blockOnPaused() {
        QMutexLocker locker(&rendererMutex);
        while (paused)
            rendererCondition.wait(&rendererMutex);
    }
    void blockOnPausedWithStep() {
        QMutexLocker locker(&rendererMutex);
        while (paused && !singleStep)
            rendererCondition.wait(&rendererMutex);
        singleStep = false;
    }

    AVStream *stream(QPlatformMediaPlayer::TrackType type) {
        int index = m_currentStream[type];
        return index < 0 ? nullptr : context->streams[index];
    }

    void changeAudioSink(QPlatformAudioOutput *output);

    void startDemuxer();
    void openCodec(QPlatformMediaPlayer::TrackType type, int index = -1);
    void closeCodec(QPlatformMediaPlayer::TrackType type);

    int getDefaultStream(QPlatformMediaPlayer::TrackType type)
    {
        const auto &map = player->m_streamMap[type];
        for (int i = 0; i < map.size(); ++i) {
            auto *s = context->streams[map.at(i).avStreamIndex];
            if (s->disposition & AV_DISPOSITION_DEFAULT)
                return i;
        }
        return 0;
    }

    void seek(qint64 pos)
    {
        seek_pos.storeRelaxed(pos);
        qDebug() << ">>>>>> seeking to pos" << pos;
        triggerSingleStep();
    }

    QFFmpegMediaPlayer *player = nullptr;

    bool paused = true;
    bool singleStep = false;
    QMutex rendererMutex;
    QWaitCondition rendererCondition;

    DemuxerThread *demuxer = nullptr;
    QAtomicInteger<qint64> seek_pos = -1;

    VideoDecoderThread *videoDecoder = nullptr;
    VideoRendererThread *videoRenderer = nullptr;
    AudioDecoderThread *audioDecoder = nullptr;
    AudioRendererThread *audioRenderer = nullptr;

    AVFormatContext *context = nullptr;
    int m_currentStream[QPlatformMediaPlayer::NTrackTypes] = { -1, -1, -1 };
    AVCodecContext *codecContext[QPlatformMediaPlayer::NTrackTypes] = {};
    QVideoSink *videoSink = nullptr;

    QMutex audioMutex;
    QPlatformAudioOutput *audioOutput = nullptr;
    QAudioSink *audioSink = nullptr;
    QIODevice *audioDevice = nullptr;
    SwrContext *resampler = nullptr;

    // queue up max 16M of encoded data, that should always be enough
    // (it's around 3.5 secs of 4K h264, longer for almost all other formats)
    enum { MaxQueueSize = 16*1024*1024 };

    PacketQueue videoPacketQueue;
    PacketQueue audioPacketQueue;

    SubtitleQueue subtitleQueue;
    FrameQueue videoFrameQueue;
    FrameQueue audioFrameQueue;

    qint64 pts_base = -1;
    QElapsedTimer baseTimer;
    qint64 currentTime = 0;
    qint64 nextFrameTime = 0;

    bool playing = false;
};

class DemuxerThread : public QThread
{
public:
    DemuxerThread(QFFmpegDecoder *decoder)
        : QThread(decoder)
        , data(decoder)
    {
        av_init_packet(&packet);
    }

    ~DemuxerThread()
    {
        av_packet_unref(&packet);
    }

    void run() override
    {
        qCDebug(qLcDemuxer) << "Demuxer started";
        qint64 seekPos = -1;
        qint64 seekOffset = 0;
        while (1) {
            int seek = data->seek_pos.loadRelaxed();
            if (seek >= 0) {
                seekPos = seek;
                data->seek_pos.storeRelaxed(-1);
            }

            if (seekPos >= 0)
                doSeek(seekPos, seekOffset);

            if (av_read_frame(data->context, &packet) < 0)
                break;
            if (seekPos >= 0 && packet.pts != AV_NOPTS_VALUE) {
                auto *stream = data->context->streams[packet.stream_index];
                qint64 pts = timeStamp(packet.pts, stream->time_base);
//                qDebug() << ">>> after seek, got pts:" << pts;
                if (pts > seekPos && seekPos + seekOffset >= 0) {
                    seekOffset -= 50;
//                    qDebug() << "    retrying seek:" << seekPos << seekOffset;
                    continue;
                }
                // Found a decent pos
                seekPos = -1;
                seekOffset = 0;
                if (data->paused)
                    data->singleStep = true;
            }
            queuePacket();

            auto haveEnoughPackets = [&]() -> bool {
                if (data->videoPacketQueue.size + data->audioPacketQueue.size > QFFmpegDecoder::MaxQueueSize)
                    return true;
                auto *videoStream = data->stream(QPlatformMediaPlayer::VideoStream);
                if (videoStream && timeStamp(data->videoPacketQueue.duration, videoStream->time_base) < 2000)
                    return false;
                auto *audioStream = data->stream(QPlatformMediaPlayer::AudioStream);
                if (audioStream && timeStamp(data->audioPacketQueue.duration, audioStream->time_base) < 2000)
                    return false;
                return true;
            };

            while (haveEnoughPackets() && data->seek_pos.loadRelaxed() < 0)
                msleep(10);
        }
    }

    void doSeek(qint64 pos, qint64 offset)
    {
        auto *stream = data->stream(QPlatformMediaPlayer::VideoStream);
        if (!stream)
            stream = data->stream(QPlatformMediaPlayer::AudioStream);
        qint64 seekPos = (pos + offset)*AV_TIME_BASE/1000;
        int streamIndex = -1;
        if (stream && stream->time_base.num != 0) {
            streamIndex = stream->index;
            seekPos = (pos + offset)*stream->time_base.den/(stream->time_base.num*1000);
        }
//        qDebug() << ">>>> executing seek" << pos << streamIndex << seekPos;// << timeStamp(seekPos, stream->time_base);
        av_seek_frame(data->context, streamIndex, seekPos, AVSEEK_FLAG_BACKWARD);
        data->audioPacketQueue.flushAndSeek(pos);
        data->videoPacketQueue.flushAndSeek(pos);
        data->subtitleQueue.clear();
        qDebug() << "all queues flushed";
    }

    void queuePacket()
    {
        auto *stream = data->context->streams[packet.stream_index];
        int trackType = -1;
        for (int i = 0; i < QPlatformMediaPlayer::NTrackTypes; ++i)
            if (packet.stream_index == data->m_currentStream[i])
                trackType = i;

        auto *codec = data->codecContext[trackType];
        if (!codec || trackType < 0)
            return;

        auto base = stream->time_base;
        qCDebug(qLcDemuxer) << "read a packet: track" << trackType << packet.stream_index
                            << timeStamp(packet.dts, base) << timeStamp(packet.pts, base) << timeStamp(packet.duration, base);

        if (trackType == QPlatformMediaPlayer::VideoStream)
            data->videoPacketQueue.enqueue(&packet);
        else if (trackType == QPlatformMediaPlayer::AudioStream)
            data->audioPacketQueue.enqueue(&packet);
        else if (trackType == QPlatformMediaPlayer::SubtitleStream)
            decodeSubtitle(stream, codec, &packet);

    }

    void decodeSubtitle(AVStream *stream, AVCodecContext *codec, AVPacket *packet)
    {
        auto base = stream->time_base;
        //                qDebug() << "    decoding subtitle" << "has delay:" << (codec->codec->capabilities & AV_CODEC_CAP_DELAY);
        AVSubtitle subtitle;
        memset(&subtitle, 0, sizeof(subtitle));
        int gotSubtitle = 0;
        int res = avcodec_decode_subtitle2(codec, &subtitle, &gotSubtitle, packet);
        //                qDebug() << "       subtitle got:" << res << gotSubtitle << subtitle.format << Qt::hex << (quint64)subtitle.pts;
        if (res >= 0 && gotSubtitle) {
            // apparently the timestamps in the AVSubtitle structure are not always filled in
            // if they are missing, use the packets pts and duration values instead
            qint64 start, end;
            if (subtitle.pts == AV_NOPTS_VALUE) {
                start = timeStamp(packet->pts, base);
                end = start + timeStamp(packet->duration, base);
            } else {
                qint64 pts = timeStamp(subtitle.pts, AV_TIME_BASE_Q);
                start = pts + subtitle.start_display_time;
                end = pts + subtitle.end_display_time;
            }
            //                    qDebug() << "    got subtitle (" << start << "--" << end << "):";
            QString text;
            for (uint i = 0; i < subtitle.num_rects; ++i) {
                const auto *r = subtitle.rects[i];
                //                        qDebug() << "    subtitletext:" << r->text << "/" << r->ass;
                if (i)
                    text += QLatin1Char('\n');
                if (r->text)
                    text += QString::fromUtf8(r->text);
                else {
                    const char *ass = r->ass;
                    int nCommas = 0;
                    while (*ass) {
                        if (nCommas == 9)
                            break;
                        if (*ass == ',')
                            ++nCommas;
                        ++ass;
                    }
                    text += QString::fromUtf8(ass);
                }
            }
            text.replace(QLatin1String("\\N"), QLatin1String("\n"));
            text.replace(QLatin1String("\\n"), QLatin1String("\n"));
            text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
            if (text.endsWith(QLatin1Char('\n')))
                text.chop(1);

            //                    qDebug() << "    >>> subtitle adding" << text << start << end;
            Subtitle *sub = new Subtitle{text, start, end};
            data->subtitleQueue.enqueue(sub);
        }
    }

    QFFmpegDecoder *data;
    AVPacket packet;
};

class VideoDecoderThread : public QThread
{
public:
    VideoDecoderThread(QFFmpegDecoder *decoder)
        : QThread(decoder)
        , data(decoder)
    {}

    void run()
    {
        qCDebug(qLcVideoDecoder) << "Starting video decoder";
        int seekTo = -1;
        for (;;) {
            data->videoFrameQueue.waitForSpace();

            auto *codec = data->codecContext[QPlatformMediaPlayer::VideoStream];
            Q_ASSERT(codec);
            AVFrame *frame = av_frame_alloc();
            int res = avcodec_receive_frame(codec, frame);
            if (res >= 0) {
                qCDebug(qLcVideoDecoder) << "received video frame";
                if (seekTo >= 0) {
                    auto *stream = data->stream(QPlatformMediaPlayer::VideoStream);
                    if (timeStamp(frame->pts, stream->time_base) < seekTo) {
                        // too early, discard
                        av_frame_free(&frame);
                        continue;
                    }
                    seekTo = -1;
                }
                data->videoFrameQueue.enqueue(frame);
            } else if (res == AVERROR(EOF)) {
                break;
            } else if (res != AVERROR(EAGAIN)) {
                qWarning() << "error in video decoder" << res;
            }

            qint64 seek = -1;
            AVPacket *packet = data->videoPacketQueue.dequeue(&seek);
            while (!packet) {
                qCDebug(qLcVideoDecoder) << "no packets in packet queue, waiting";
                data->videoPacketQueue.waitForPacket();
                packet = data->videoPacketQueue.dequeue(&seek);
            }
            if (seek >= 0) {
                qDebug() << ">>> flushing video codec";
                avcodec_flush_buffers(codec);
                data->videoFrameQueue.flush();
                seekTo = seek;
            }

            // send the frame to the data
            avcodec_send_packet(codec, packet);
            av_packet_unref(packet);
            qCDebug(qLcVideoDecoder) << "video packet sent to decoder";
        }
    }

    QFFmpegDecoder *data;
};

class AudioDecoderThread : public QThread
{
public:
    AudioDecoderThread(QFFmpegDecoder *decoder)
        : QThread(decoder)
        , data(decoder)
    {}

    void run()
    {
        qCDebug(qLcAudioDecoder) << "Starting audio decoder";
        int seekTo = -1;
        for (;;) {
            data->audioFrameQueue.waitForSpace();

            auto *codec = data->codecContext[QPlatformMediaPlayer::AudioStream];
            Q_ASSERT(codec);
            AVFrame *frame = av_frame_alloc();
            int res = avcodec_receive_frame(codec, frame);
            if (res >= 0) {
                qCDebug(qLcAudioDecoder) << "received audio frame";
                if (seekTo >= 0) {
                    auto *stream = data->stream(QPlatformMediaPlayer::VideoStream);
                    if (timeStamp(frame->pts, stream->time_base) < seekTo) {
                        // too early, discard
                        av_frame_free(&frame);
                        continue;
                    }
                    seekTo = -1;
                }
                data->audioFrameQueue.enqueue(frame);
            } else if (res == AVERROR(EOF)) {
                break;
            } else if (res != AVERROR(EAGAIN)) {
                qWarning() << "error in audio decoder" << res;
            }

            qint64 seek = -1;
            AVPacket *packet = data->audioPacketQueue.dequeue(&seek);
            while (!packet) {
                data->audioPacketQueue.waitForPacket();
                packet = data->audioPacketQueue.dequeue(&seek);
            }
            if (seek >= 0) {
                qDebug() << ">>> flushing audio codec";
                avcodec_flush_buffers(codec);
                data->audioFrameQueue.flush();
                seekTo = seek;
            }

            // send the frame to the data
            avcodec_send_packet(codec, packet);
            av_packet_unref(packet);
            qCDebug(qLcAudioDecoder) << "audio packet sent to decoder";
        }
    }

    QFFmpegDecoder *data;
};

class VideoRendererThread : public QThread
{
public:
    VideoRendererThread(QFFmpegDecoder *decoder)
        : QThread(decoder)
        , data(decoder)
    {}

    void run()
    {
        qCDebug(qLcVideoRenderer) << "starting video renderer";
        for (;;) {
            data->blockOnPausedWithStep();

            auto *stream = data->context->streams[data->m_currentStream[QPlatformMediaPlayer::VideoStream]];
            auto base = stream->time_base;

            QVideoSink *sink = data->videoSink;
            qCDebug(qLcVideoRenderer) << "waiting for video frame" << sink;
            data->videoFrameQueue.waitForFrame();
            bool flushed = false;
            AVFrame *avFrame = data->videoFrameQueue.dequeue(&flushed);
            if (flushed) {
                qCDebug(qLcVideoRenderer) << "video frame queue flushed, resetting pts_base";
                data->pts_base = -1;
            }
            qCDebug(qLcVideoRenderer) << "received video frame";
            Q_ASSERT(avFrame);

            qint64 startTime = timeStamp(avFrame->pts, base);
            qint64 duration = (1000*stream->avg_frame_rate.den + (stream->avg_frame_rate.num>>1))
                              /stream->avg_frame_rate.num;
            if (sink) {
                QFFmpegVideoBuffer *buffer = new QFFmpegVideoBuffer(avFrame);
                QVideoFrameFormat format(buffer->size(), buffer->pixelFormat());
                QVideoFrame frame(buffer, format);
                qint64 startTime = timeStamp(avFrame->pts, base);
                frame.setStartTime(startTime);
                frame.setEndTime(startTime + duration);
                qCDebug(qLcVideoRenderer) << ">>>> creating video frame" << startTime << (startTime + duration);

                // add in subtitles
                Subtitle *sub = data->subtitleQueue.peek();
//                qCDebug(qLcVideoRenderer) << "frame: subtitle" << sub;
                if (sub) {
                    qCDebug(qLcVideoRenderer) << "    " << sub->start << sub->end << sub->text;
                    if (sub->start <= startTime && sub->end > startTime) {
//                        qCDebug(qLcVideoRenderer) << "        setting text";
                        sink->setSubtitleText(sub->text);
                    }
                    if (sub->end < startTime) {
//                        qCDebug(qLcVideoRenderer) << "        removing subtitle item";
                        delete data->subtitleQueue.dequeue();
                        sink->setSubtitleText({});
                    }
                }

//                qCDebug(qLcVideoRenderer) << "    sending a video frame" << startTime << duration;
                sink->setVideoFrame(frame);
            }
            AVFrame *nextFrame = data->videoFrameQueue.peek();
            if (data->pts_base < 0) {
                qCDebug(qLcVideoRenderer) << "videorenderer: setting new time base" << startTime;
                data->pts_base = startTime;
                data->baseTimer.start();
            }
            if (nextFrame)
                data->nextFrameTime = timeStamp(nextFrame->pts, base);
            else
                data->nextFrameTime = startTime + duration;
            int msToWait = data->nextFrameTime - (data->baseTimer.elapsed() + data->pts_base);
            qCDebug(qLcVideoRenderer) << "next video frame in" << msToWait;
            data->currentTime = startTime;
            data->player->positionChanged(startTime);
            if (msToWait > 0)
                msleep(msToWait);
        }
    }

    QFFmpegDecoder *data;
};

class AudioRendererThread : public QThread
{
public:
    AudioRendererThread(QFFmpegDecoder *decoder)
        : QThread(decoder)
        , data(decoder)
    {}

    void run()
    {
        qDebug() << "Starting audio renderer";
        for (;;) {
            // ### suspend the audiosink on pause
            data->blockOnPaused();

            auto *stream = data->context->streams[data->m_currentStream[QPlatformMediaPlayer::AudioStream]];
            auto base = stream->time_base;

            data->audioFrameQueue.waitForFrame();
            QMutexLocker locker(&data->audioMutex);
            bool flushed = false;
            AVFrame *avFrame = data->audioFrameQueue.dequeue(&flushed);
            if (flushed) {
                qDebug() << "audio frame queue flushed, resetting pts_base";
                data->pts_base = -1;
            }
            Q_ASSERT(avFrame);

            if (!data->audioSink) {
                qDebug() << "no sink, skipping frame";
                av_frame_free(&avFrame);
                continue;
            }

            QAudioFormat format = data->audioSink->format();
            qint64 startTime = timeStamp(avFrame->pts, base);
            qint64 duration = format.durationForBytes(avFrame->linesize[0]);
            qDebug() << "sending" << avFrame->linesize[0] << "bytes to audio sink, startTime/duration=" << startTime << duration;
            if (!data->resampler) {
                data->audioDevice->write((char *)avFrame->data[0], avFrame->linesize[0]);
            } else {
                 uint8_t *output;
                 av_samples_alloc(&output, nullptr, 2, avFrame->nb_samples,
                                  QFFmpegMediaDevices::avSampleFormat(format.sampleFormat()), 0);
                 const uint8_t **in = (const uint8_t **)avFrame->extended_data;
                 int out_samples = swr_convert(data->resampler, &output, avFrame->nb_samples,
                                               in, avFrame->nb_samples);
                 int size = av_samples_get_buffer_size(nullptr, 2, out_samples,
                                                       QFFmpegMediaDevices::avSampleFormat(format.sampleFormat()), 0);
                 data->audioDevice->write((char *)output, size);
                 av_freep(&output);
            }
            av_frame_free(&avFrame);

            AVFrame *nextFrame = data->audioFrameQueue.peek();
            if (data->pts_base < 0) {
                qDebug() << "audiorenderer: setting new time base" << startTime;
                data->pts_base = startTime;
                data->baseTimer.start();
            }
            qint64 nextAudioFrameTime = startTime + duration/1000;
            if (nextFrame)
                nextAudioFrameTime = qMin(timeStamp(nextFrame->pts, base), nextAudioFrameTime);
            locker.unlock();
            // always write 40ms ahead
            int msToWait = nextAudioFrameTime - 40 - (data->baseTimer.elapsed() + data->pts_base);
            qDebug() << "next audio frame at" << nextAudioFrameTime << "sleeping" << msToWait << "ms.";
            if (msToWait > 0)
                msleep(msToWait);
        }
    }

    QFFmpegDecoder *data;
};


QFFmpegDecoder::QFFmpegDecoder(QFFmpegMediaPlayer *p)
    : player(p)
    , videoFrameQueue(3)
    , audioFrameQueue(9)
{}

void QFFmpegDecoder::changeAudioSink(QPlatformAudioOutput *output)
{
    // ### optimize and avoid reallocating if current and new would be the same
    audioOutput = output;
    QMutexLocker locker(&audioMutex);
    qDebug() << "changeAudioSink" << audioOutput;

    if (audioSink) {
        audioSink->stop();
        audioDevice = nullptr;
        delete audioSink;
        audioSink = nullptr;
        if (resampler)
            swr_free(&resampler);
        resampler = nullptr;
    }
    if (!output)
        return;

    AVStream *audioStream = stream(QPlatformMediaPlayer::AudioStream);
    if (!audioStream) {
        qDebug() << "   -> XXX no stream!";
        return;
    }

    QAudioFormat format;
    format.setSampleFormat(QFFmpegMediaDevices::sampleFormat(AVSampleFormat(audioStream->codecpar->format)));
    format.setSampleRate(audioStream->codecpar->sample_rate);
    format.setChannelCount(2); // #### FIXME
    // ### add channel layout
    qDebug() << "creating new audio sink with format" << format;
    audioSink = new QAudioSink(audioOutput->device, format, this);
    audioDevice = audioSink->start();
    qDebug() << "   -> have an audio sink" << audioDevice;

    // init resampling if needed
    AVSampleFormat requiredFormat = QFFmpegMediaDevices::avSampleFormat(format.sampleFormat());
    if (requiredFormat == audioStream->codecpar->format &&
        audioStream->codecpar->channels == 2)
        return;
    resampler = swr_alloc_set_opts(nullptr,  // we're allocating a new context
                                    AV_CH_LAYOUT_STEREO,  // out_ch_layout
                                    requiredFormat,    // out_sample_fmt
                                    audioStream->codecpar->sample_rate,                // out_sample_rate
                                    audioStream->codecpar->channel_layout, // in_ch_layout
                                    AVSampleFormat(audioStream->codecpar->format),   // in_sample_fmt
                                    audioStream->codecpar->sample_rate,                // in_sample_rate
                                    0,                    // log_offset
                                    nullptr);
    swr_init(resampler);
}
void QFFmpegDecoder::startDemuxer()
{
    if (!demuxer) {
        demuxer = new DemuxerThread(this);
        demuxer->start();
    }
}

void QFFmpegDecoder::openCodec(QPlatformMediaPlayer::TrackType type, int index)
{
    if (index < 0)
        index = getDefaultStream(type);
    qDebug() << "openCodec" << type << index;
    int streamIdx = player->m_streamMap[type].value(index).avStreamIndex;
    if (streamIdx < 0)
        return;
    qDebug() << "    using stream index" << streamIdx;
    // ### return if same track as before

    if (codecContext[type])
        closeCodec(type);

    auto *stream = context->streams[streamIdx];
    AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        player->error(QMediaPlayer::FormatError, QMediaPlayer::tr("Can't decode track."));
        return;
    }

    codecContext[type] = avcodec_alloc_context3(decoder);
    if (!codecContext[type]) {
        player->error(QMediaPlayer::FormatError, QMediaPlayer::tr("Failed to allocate a FFmpeg codec context"));
        return;
    }
    int ret = avcodec_parameters_to_context(codecContext[type], stream->codecpar);
    if (ret < 0) {
        player->error(QMediaPlayer::FormatError, QMediaPlayer::tr("Can't decode track."));
        return;
    }
    /* Init the decoders, with or without reference counting */
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    av_dict_set(&opts, "threads", "auto", 0);
    ret = avcodec_open2(codecContext[type], decoder, &opts);
    if (ret < 0) {
        player->error(QMediaPlayer::FormatError, QMediaPlayer::tr("Can't decode track."));
        return;
    }
    m_currentStream[type] = streamIdx;
    if (type == QPlatformMediaPlayer::VideoStream) {
        videoDecoder = new VideoDecoderThread(this);
        videoRenderer = new VideoRendererThread(this);
        videoDecoder->start();
        videoRenderer->start();
    } else if (type == QPlatformMediaPlayer::AudioStream) {
        audioDecoder = new AudioDecoderThread(this);
        audioRenderer = new AudioRendererThread(this);
        audioDecoder->start();
        audioRenderer->start();
    }
}

void QFFmpegDecoder::closeCodec(QPlatformMediaPlayer::TrackType type)
{
    if (codecContext[type])
        avcodec_close(codecContext[type]);
    codecContext[type] = nullptr;
    m_currentStream[type] = -1;
}


QFFmpegMediaPlayer::QFFmpegMediaPlayer(QMediaPlayer *player)
    : QPlatformMediaPlayer(player)
{
    decoder = new QFFmpegDecoder(this);
}

QFFmpegMediaPlayer::~QFFmpegMediaPlayer()
{
    delete decoder;
}

qint64 QFFmpegMediaPlayer::duration() const
{
    return m_duration;
}

qint64 QFFmpegMediaPlayer::position() const
{
    return decoder->currentTime;
}

void QFFmpegMediaPlayer::setPosition(qint64 position)
{
    decoder->seek(position);
}

float QFFmpegMediaPlayer::bufferProgress() const
{
    return 0;
}

QMediaTimeRange QFFmpegMediaPlayer::availablePlaybackRanges() const
{
    return {};
}

qreal QFFmpegMediaPlayer::playbackRate() const
{
    return 1;
}

void QFFmpegMediaPlayer::setPlaybackRate(qreal rate)
{
    Q_UNUSED(rate);
}

QUrl QFFmpegMediaPlayer::media() const
{
    return m_url;
}

const QIODevice *QFFmpegMediaPlayer::mediaStream() const
{
    return m_device;
}

void QFFmpegMediaPlayer::setMedia(const QUrl &media, QIODevice *stream)
{
    m_url = media;
    m_device = stream;
    m_streamMap[0] = m_streamMap[1] = m_streamMap[2] = {};
    m_metaData = {};

    // ### use io device when provided

    QByteArray url = media.toEncoded();
    decoder->context = nullptr;
    int ret = avformat_open_input(&decoder->context, url.constData(), nullptr, nullptr);
    if (ret < 0) {
        error(QMediaPlayer::AccessDeniedError, QMediaPlayer::tr("Could not open file"));
        return;
    }

    ret = avformat_find_stream_info(decoder->context, nullptr);
    if (ret < 0) {
        error(QMediaPlayer::FormatError, QMediaPlayer::tr("Could not find stream information for media file"));
        return;
    }
    av_dump_format(decoder->context, 0, url.constData(), 0);

    m_metaData = QFFmpegMetaData::fromAVMetaData(decoder->context->metadata);
    metaDataChanged();

    // check streams and seekable
    if (decoder->context->ctx_flags & AVFMTCTX_NOHEADER) {
        // ### dynamic streams, will get added in read_frame
    } else {
        checkStreams();
    }
    seekableChanged(!(decoder->context->ctx_flags & AVFMTCTX_UNSEEKABLE));
}

void QFFmpegMediaPlayer::play()
{
    decoder->play();
    stateChanged(QMediaPlayer::PlayingState);
}

void QFFmpegMediaPlayer::pause()
{
    decoder->pause();
    stateChanged(QMediaPlayer::PausedState);
}

void QFFmpegMediaPlayer::stop()
{
    decoder->pause();
    stateChanged(QMediaPlayer::StoppedState);
}

void QFFmpegMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    decoder->changeAudioSink(output);
}

void QFFmpegMediaPlayer::setVideoSink(QVideoSink *sink)
{
    decoder->videoSink = sink;
}

QVideoSink *QFFmpegMediaPlayer::videoSink() const
{
    return decoder->videoSink;
}

int QFFmpegMediaPlayer::trackCount(TrackType type)
{
    return m_streamMap[type].count();
}

QMediaMetaData QFFmpegMediaPlayer::trackMetaData(TrackType type, int streamNumber)
{
    if (streamNumber < 0 || streamNumber >= m_streamMap[type].count())
        return {};
    return m_streamMap[type].at(streamNumber).metaData;
}

int QFFmpegMediaPlayer::activeTrack(TrackType)
{
    // ###
    return 0;
}

void QFFmpegMediaPlayer::setActiveTrack(TrackType, int /*streamNumber*/)
{
    // ###

}

void QFFmpegMediaPlayer::closeContext()
{
    avformat_close_input(&decoder->context);
    decoder->context = nullptr;
}

void QFFmpegMediaPlayer::checkStreams()
{
    Q_ASSERT(decoder->context);

    qint64 duration = 0;
    for (unsigned int i = 0; i < decoder->context->nb_streams; ++i) {
        auto *stream = decoder->context->streams[i];

        QMediaMetaData metaData = QFFmpegMetaData::fromAVMetaData(stream->metadata);
        TrackType type = VideoStream;
        auto *codecPar = stream->codecpar;

        switch (codecPar->codec_type) {
        case AVMEDIA_TYPE_UNKNOWN:
        case AVMEDIA_TYPE_DATA:          ///< Opaque data information usually continuous
        case AVMEDIA_TYPE_ATTACHMENT:    ///< Opaque data information usually sparse
        case AVMEDIA_TYPE_NB:
            continue;
        case AVMEDIA_TYPE_VIDEO:
            type = VideoStream;
            metaData.insert(QMediaMetaData::VideoBitRate, (int)codecPar->bit_rate);
            metaData.insert(QMediaMetaData::VideoCodec, QVariant::fromValue(QFFmpegMediaFormatInfo::videoCodecForAVCodecId(codecPar->codec_id)));
            metaData.insert(QMediaMetaData::Resolution, QSize(codecPar->width, codecPar->height));
            metaData.insert(QMediaMetaData::VideoFrameRate,
                            (qreal)stream->avg_frame_rate.num*stream->time_base.num/(qreal)(stream->avg_frame_rate.den*stream->time_base.den));
            break;
        case AVMEDIA_TYPE_AUDIO:
            type = AudioStream;
            metaData.insert(QMediaMetaData::AudioBitRate, (int)codecPar->bit_rate);
            metaData.insert(QMediaMetaData::AudioCodec, QVariant::fromValue(QFFmpegMediaFormatInfo::videoCodecForAVCodecId(codecPar->codec_id)));
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            type = SubtitleStream;
            break;
        }

        m_streamMap[type].append({ (int)i, metaData });
        duration = qMax(duration, 1000*stream->duration*stream->time_base.num/stream->time_base.den);

        tracksChanged();
    }

    qDebug() << "stream counts" << m_streamMap[0].size() << m_streamMap[1].size() << m_streamMap[2].size();
    qDebug() << "duration" << duration;

    if (m_duration != duration) {
        m_duration = duration;
        durationChanged(duration);
    }
}

QT_BEGIN_NAMESPACE



QT_END_NAMESPACE
