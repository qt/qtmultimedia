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
#include "private/qiso639_2_p.h"
#include "qffmpeg_p.h"
#include "qffmpegmediametadata_p.h"
#include "qffmpegvideobuffer_p.h"
#include "private/qplatformaudiooutput_p.h"
#include "qvideosink.h"

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
    AVPacket *dequeue() {
        QMutexLocker locker(&mutex);
        if (queue.isEmpty())
            return nullptr;
        auto *p = queue.dequeue();
        size -= p->size;
        duration -= p->duration;
        return p;
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
};

struct FrameQueue
{
    FrameQueue(int size) : maxSize(size) {}
    QMutex mutex;
    QWaitCondition condition;
    QQueue<AVFrame *> queue;
    int maxSize = 0;

    void enqueue(AVFrame *f) {
        Q_ASSERT(f);
        QMutexLocker locker(&mutex);
        queue.enqueue(f);
    }
    AVFrame *peek() {
        QMutexLocker locker(&mutex);
        return queue.isEmpty() ? nullptr : queue.first();
    }
    AVFrame *dequeue() {
        QMutexLocker locker(&mutex);
        return queue.isEmpty() ? nullptr : queue.dequeue();
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

    void play() {
        startDemuxer();
    }
    void pause() {

    }

    AVStream *stream(QPlatformMediaPlayer::TrackType type) {
        int index = m_currentStream[type];
        return index < 0 ? nullptr : context->streams[index];
    }

    void startDemuxer();
    void openCodec(QPlatformMediaPlayer::TrackType type, int index);
    void closeCodec(QPlatformMediaPlayer::TrackType type);

    QFFmpegMediaPlayer *player = nullptr;

    DemuxerThread *demuxer = nullptr;
    VideoDecoderThread *videoDecoder = nullptr;
    VideoRendererThread *videoRenderer = nullptr;
    AudioDecoderThread *audioDecoder = nullptr;
    AudioRendererThread *audioRenderer = nullptr;

    AVFormatContext *context = nullptr;
    int m_currentStream[QPlatformMediaPlayer::NTrackTypes] = { -1, -1, -1 };
    AVCodecContext *codecContext[QPlatformMediaPlayer::NTrackTypes] = {};
    QVideoSink *videoSink = nullptr;
    QAudioOutput *audioOutput = nullptr;

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
    {}

    void run() override
    {
        qCDebug(qLcDemuxer) << "Demuxer started";
        AVPacket packet;
        av_init_packet(&packet);
        while (av_read_frame(data->context, &packet) >= 0) {
            auto *stream = data->context->streams[packet.stream_index];
            int trackType = -1;
            for (int i = 0; i < QPlatformMediaPlayer::NTrackTypes; ++i)
                if (packet.stream_index == data->m_currentStream[i])
                    trackType = i;

            auto *codec = data->codecContext[trackType];
            if (!codec || trackType < 0)
                continue;

            auto base = stream->time_base;
            qCDebug(qLcDemuxer) << "read a packet: track" << trackType << packet.stream_index
                     << timeStamp(packet.dts, base) << timeStamp(packet.pts, base) << timeStamp(packet.duration, base);

            if (trackType == QPlatformMediaPlayer::VideoStream)
                data->videoPacketQueue.enqueue(&packet);
            else if (trackType == QPlatformMediaPlayer::AudioStream)
                data->audioPacketQueue.enqueue(&packet);
            else if (trackType == QPlatformMediaPlayer::SubtitleStream)
                decodeSubtitle(stream, codec, &packet);

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

            while (haveEnoughPackets()) {
                msleep(10);
            }
        }
        av_packet_unref(&packet);
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
            // in this case subtitle.pts seems to be 0x8000000000000000, let's simply check for pts < 0
            qint64 start, end;
            if (subtitle.pts < 0) {
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
        for (;;) {
            data->videoFrameQueue.waitForSpace();

            auto *codec = data->codecContext[QPlatformMediaPlayer::VideoStream];
            Q_ASSERT(codec);
            AVFrame *frame = av_frame_alloc();
            int res = avcodec_receive_frame(codec, frame);
            if (res >= 0) {
                qCDebug(qLcVideoDecoder) << "received video frame";
                data->videoFrameQueue.enqueue(frame);
            } else if (res == AVERROR(EOF)) {
                break;
            } else if (res != AVERROR(EAGAIN)) {
                qWarning() << "error in video decoder" << res;
            }

            AVPacket *packet = data->videoPacketQueue.dequeue();
            if (!packet) {
                qCDebug(qLcVideoDecoder) << "no packets in packet queue, waiting";
                data->videoPacketQueue.waitForPacket();
                packet = data->videoPacketQueue.dequeue();
                Q_ASSERT(packet);
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
        for (;;) {
            data->audioFrameQueue.waitForSpace();

            auto *codec = data->codecContext[QPlatformMediaPlayer::AudioStream];
            Q_ASSERT(codec);
            AVFrame *frame = av_frame_alloc();
            int res = avcodec_receive_frame(codec, frame);
            if (res >= 0) {
                qCDebug(qLcAudioDecoder) << "received audio frame";
                data->audioFrameQueue.enqueue(frame);
            } else if (res == AVERROR(EOF)) {
                break;
            } else if (res != AVERROR(EAGAIN)) {
                qWarning() << "error in audio decoder" << res;
            }

            AVPacket *packet = data->audioPacketQueue.dequeue();
            if (!packet) {
                data->audioPacketQueue.waitForPacket();
                packet = data->audioPacketQueue.dequeue();
                Q_ASSERT(packet);
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
            auto *stream = data->context->streams[data->m_currentStream[QPlatformMediaPlayer::VideoStream]];
            auto base = stream->time_base;

            QVideoSink *sink = data->videoSink;
            qCDebug(qLcVideoRenderer) << "waiting for video frame" << sink;
            data->videoFrameQueue.waitForFrame();
            AVFrame *avFrame = data->videoFrameQueue.dequeue();
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
                qDebug() << ">>>> creating video frame" << startTime << (startTime + duration);

                // add in subtitles
                Subtitle *sub = data->subtitleQueue.peek();
                qDebug() << "frame: subtitle" << sub;
                if (sub) {
                    qDebug() << "    " << sub->start << sub->end << sub->text;
                    qDebug() << "    " << sub->start << sub->end << sub->text;
                    if (sub->start <= startTime && sub->end > startTime) {
                        qDebug() << "        setting text";
                        sink->setSubtitleText(sub->text);
                    }
                    if (sub->end < startTime) {
                        qDebug() << "        removing subtitle item";
                        delete data->subtitleQueue.dequeue();
                        sink->setSubtitleText({});
                    }
                }

                qDebug() << "    sending a video frame" << startTime << duration;
                sink->setVideoFrame(frame);
            }
            AVFrame *nextFrame = data->videoFrameQueue.peek();
            if (data->pts_base < 0) {
                qDebug() << "setting new time base" << startTime;
                data->pts_base = startTime;
                data->baseTimer.start();
            }
            if (nextFrame)
                data->nextFrameTime = timeStamp(nextFrame->pts, base);
            else
                data->nextFrameTime = startTime + duration;
            int msToWait = data->nextFrameTime - (data->baseTimer.elapsed() + data->pts_base);
            qDebug() << "next video frame in" << msToWait;
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
        for (;;) {
//            auto *stream = data->context->streams[data->m_currentStream[QPlatformMediaPlayer::AudioStream]];
//            auto base = stream->time_base;

            data->audioFrameQueue.waitForFrame();
            AVFrame *avFrame = data->audioFrameQueue.dequeue();
            Q_ASSERT(avFrame);

//            qint64 startTime = timeStamp(avFrame->pts, base);
//            qint64 duration = (1000*stream->avg_frame_rate.den + (stream->avg_frame_rate.num>>1))
//                              /stream->avg_frame_rate.num;
//            // ### push to audio output

//            AVFrame *nextFrame = data->audioFrameQueue.peek();
//            if (data->pts_base < 0) {
//                qDebug() << "setting new time base" << startTime;
//                data->pts_base = startTime;
//                data->baseTimer.start();
//            }
//            if (nextFrame)
//                data->nextFrameTime = timeStamp(nextFrame->pts, base);
//            else
//                data->nextFrameTime = startTime + duration;
//            int msToWait = data->nextFrameTime - (data->baseTimer.elapsed() + data->pts_base);
//            if (msToWait > 0)
//                msleep(msToWait);
        }
    }

    QFFmpegDecoder *data;
};


QFFmpegDecoder::QFFmpegDecoder(QFFmpegMediaPlayer *p)
    : player(p)
    , videoFrameQueue(3)
    , audioFrameQueue(9)
{}

void QFFmpegDecoder::startDemuxer()
{
    demuxer = new DemuxerThread(this);
    demuxer->start();
}

void QFFmpegDecoder::openCodec(QPlatformMediaPlayer::TrackType type, int index)
{
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
    Q_UNUSED(position);
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
    decoder->openCodec(VideoStream, 0);
    if (m_audioOutput)
        decoder->openCodec(AudioStream, 0);
    if (m_streamMap[SubtitleStream].count())
        decoder->openCodec(SubtitleStream, 0);
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
    if (m_audioOutput == output)
        return;

    m_audioOutput = output;
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
