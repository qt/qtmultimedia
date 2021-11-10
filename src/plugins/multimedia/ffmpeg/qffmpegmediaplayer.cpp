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

static qint64 timeStamp(qint64 ts, AVRational base)
{
    return (1000*ts*base.num + 500)/base.den;
}

namespace {

struct AVFrameBuffer {
    AVFrameBuffer(int size)
        : size(size)
    {
        frames = new AVFrame *[size];
        memset(frames, 0, size*sizeof(AVFrame *));
    }
    ~AVFrameBuffer()
    {
        clear();
        delete [] frames;
    }
    AVFrame **frames;
    int size;
    int begin = 0;
    int end = 0;
    bool isEmpty() const { return frames[begin] == nullptr; }
    bool isFull() const { return frames[end] != nullptr; }
    void push(AVFrame *f) {
        Q_ASSERT(frames[end] == nullptr);
        frames[end] = f;
        ++end;
        if (end == size)
            end = 0;
    }
    AVFrame *peek() {
        return frames[begin];
    }
    AVFrame *take() {
        Q_ASSERT(begin >= 0);
        AVFrame *f = frames[begin];
        if (f) {
            frames[begin] = nullptr;
            ++begin;
            if (begin == size)
                begin = 0;
        }
        return f;
    }
    void clear() {
        for (int i = 0; i < size; ++i) {
            if (frames[i])
                av_frame_free(&frames[i]);
            frames[i] = nullptr;
        }
    }
};

struct Subtitle {
    QString text;
    qint64 start;
    qint64 end;
};

struct SubtitleBuffer {

    SubtitleBuffer(int size)
        : size(size)
    {
        frames = new Subtitle *[size];
        memset(frames, 0, size*sizeof(Subtitle *));
    }
    ~SubtitleBuffer()
    {
        clear();
        delete [] frames;
    }
    Subtitle **frames;
    int size;
    int begin = 0;
    int end = 0;
    bool isEmpty() const { return frames[begin] == nullptr; }
    bool isFull() const { return frames[end] != nullptr; }
    void push(Subtitle *f) {
        Q_ASSERT(frames[end] == nullptr);
        frames[end] = f;
        ++end;
        if (end == size)
            end = 0;
    }
    Subtitle *peek() {
        return frames[begin];
    }
    Subtitle *take() {
        Q_ASSERT(begin >= 0);
        Subtitle *f = frames[begin];
        if (f) {
            frames[begin] = nullptr;
            ++begin;
            if (begin == size)
                begin = 0;
        }
        return f;
    }
    void clear() {
        for (int i = 0; i < size; ++i) {
            if (frames[i])
                delete frames[i];
            frames[i] = nullptr;
        }
    }
};

}

class DecoderThread;

class QFFmpegDecoder : public QObject
{
public:
    QFFmpegDecoder(QFFmpegMediaPlayer *player);

public slots:
    void nextFrame() { processFrames(); }

public:
    void processFrames()
    {
        if (!playing)
            return;
        qDebug() << "processing frames";

        int streamIndex = m_player->m_currentStream[QPlatformMediaPlayer::AudioStream];
        AVStream *stream = m_player->context->streams[streamIndex];
        while (1) {
            mutex.lock();
            AVFrame *frame = m_audioBuffer.take();
            mutex.unlock();
            if (!frame)
                break;
            sendAudioFrame(stream, frame);
        }

        streamIndex = m_player->m_currentStream[QPlatformMediaPlayer::VideoStream];
        stream = m_player->context->streams[streamIndex];
        mutex.lock();
        AVFrame *videoFrame = m_videoBuffer.take();
        mutex.unlock();
        qDebug() << "retrieved a video frame" << videoFrame;
        if (videoFrame)
            sendVideoFrame(stream, videoFrame);
        else
            frameTimer.start(1);
        condition.wakeAll();
    }

    void sendVideoFrame(AVStream *stream, AVFrame *avFrame)
    {
        auto base = stream->time_base;

        QVideoSink *sink = m_player->videoSink();
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
            Subtitle *sub = m_subtitleBuffer.peek();
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
                    delete m_subtitleBuffer.take();
                    sink->setSubtitleText({});
                }
            }

            qDebug() << "    sending a video frame" << startTime << duration;
            sink->setVideoFrame(frame);
        }
        mutex.lock();
        AVFrame *nextFrame = m_videoBuffer.peek();
        mutex.unlock();
        if (pts_base < 0) {
            qDebug() << "setting new time base" << startTime;
            pts_base = startTime;
            baseTimer.start();
        }
        if (nextFrame)
            nextFrameTime = timeStamp(nextFrame->pts, base);
        else
            nextFrameTime = startTime + duration;
        qDebug() << "restarting timer:" << nextFrameTime << baseTimer.elapsed() << pts_base;
        frameTimer.start(nextFrameTime - (baseTimer.elapsed() + pts_base));
    }

    void sendAudioFrame(AVStream *stream, AVFrame *avFrame)
    {
        Q_UNUSED(stream);
        av_frame_free(&avFrame);
    }

    void play() {
        playing = true;
        pts_base = -1;
        processFrames();
    }
    void pause() {
        playing = false;
    }

    friend class DecoderThread;
    DecoderThread *decoder;
    QFFmpegMediaPlayer *m_player = nullptr;

    QMutex mutex;
    QWaitCondition condition;
    AVFrameBuffer m_videoBuffer{3};
    AVFrameBuffer m_audioBuffer{9};
    SubtitleBuffer m_subtitleBuffer{3};
    qint64 nextFrameTime = 0;
    bool playing = false;
    QTimer frameTimer;
    qint64 pts_base = -1;
    QElapsedTimer baseTimer;
};

class DecoderThread : public QThread
{
public:
    DecoderThread(QFFmpegDecoder *parent)
        : QThread(parent)
        , decoder(parent)
    {}

    void sendOnePacket()
    {
        AVPacket packet;
        av_init_packet(&packet);
        while (av_read_frame(decoder->m_player->context, &packet) >= 0) {
            auto *stream = decoder->m_player->context->streams[packet.stream_index];
            int trackType = -1;
            for (int i = 0; i < QPlatformMediaPlayer::NTrackTypes; ++i)
                if (packet.stream_index == decoder->m_player->m_currentStream[i])
                    trackType = i;

            auto *codec = decoder->m_player->codecContext[trackType];
            if (!codec || trackType < 0)
                continue;

            auto base = stream->time_base;
            qDebug() << ">> read a packet: track" << trackType << packet.stream_index
                     << timeStamp(packet.dts, base) << timeStamp(packet.pts, base) << timeStamp(packet.duration, base);

            if (trackType == QPlatformMediaPlayer::VideoStream ||
                trackType == QPlatformMediaPlayer::AudioStream) {
                // send the frame to the decoder
                avcodec_send_packet(codec, &packet);
                qDebug() << "   sent packet to decoder";
                break;
            } else if (trackType == QPlatformMediaPlayer::SubtitleStream) {
//                qDebug() << "    decoding subtitle" << "has delay:" << (codec->codec->capabilities & AV_CODEC_CAP_DELAY);
                AVSubtitle subtitle;
                memset(&subtitle, 0, sizeof(subtitle));
                int gotSubtitle = 0;
                int res = avcodec_decode_subtitle2(codec, &subtitle, &gotSubtitle, &packet);
//                qDebug() << "       subtitle got:" << res << gotSubtitle << subtitle.format << Qt::hex << (quint64)subtitle.pts;
                if (res >= 0 && gotSubtitle) {
                    // apparently the timestamps in the AVSubtitle structure are not always filled in
                    // if they are missing, use the packets pts and duration values instead
                    // in this case subtitle.pts seems to be 0x8000000000000000, let's simply check for pts < 0
                    qint64 start, end;
                    if (subtitle.pts < 0) {
                        start = timeStamp(packet.pts, base);
                        end = start + timeStamp(packet.duration, base);
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
                    decoder->mutex.lock();
                    decoder->m_subtitleBuffer.push(sub);
                    decoder->mutex.unlock();
                }
            }
        }
        av_packet_unref(&packet);
    }

    void run()
    {
        for (;;) {
            decoder->mutex.lock();
            if (decoder->m_audioBuffer.isFull() && decoder->m_videoBuffer.isFull())
                decoder->condition.wait(&decoder->mutex);

            bool audioFull = decoder->m_audioBuffer.isFull();
            bool videoFull = decoder->m_videoBuffer.isFull();
            decoder->mutex.unlock();

            AVCodecContext *codec = decoder->m_player->codecContext[QPlatformMediaPlayer::VideoStream];
            if (codec && !videoFull) {
                AVFrame *avFrame = av_frame_alloc();
                int ret = avcodec_receive_frame(codec, avFrame);
                if (ret < 0) {
                    qDebug() << "no video frame from decoder";
                    if (ret == AVERROR(EAGAIN))
                        sendOnePacket();
                } else {
                    decoder->mutex.lock();
                    decoder->m_videoBuffer.push(avFrame);
                    qDebug() << "pushed a video frame" << avFrame
                             << decoder->m_videoBuffer.begin << decoder->m_videoBuffer.end;
                    decoder->mutex.unlock();
                }
            }

            codec = decoder->m_player->codecContext[QPlatformMediaPlayer::AudioStream];
            if (codec && !audioFull) {
                AVFrame *avFrame = av_frame_alloc();
                int ret = avcodec_receive_frame(codec, avFrame);
                if (ret < 0) {
                    qDebug() << "no audio frame from decoder";
                    if (ret == AVERROR(EAGAIN))
                        sendOnePacket();
                } else {
                    decoder->mutex.lock();
                    decoder->m_audioBuffer.push(avFrame);
                    decoder->mutex.unlock();
                }
            }
        }
    }

    QFFmpegDecoder *decoder = nullptr;
};

QFFmpegDecoder::QFFmpegDecoder(QFFmpegMediaPlayer *player)
    : QObject()
    , m_player(player)
{
    decoder = new DecoderThread(this);
    decoder->start();
    frameTimer.setTimerType(Qt::PreciseTimer);
    frameTimer.setSingleShot(true);
    connect(&frameTimer, &QTimer::timeout, this, &QFFmpegDecoder::nextFrame);
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
    return 0;
}

qint64 QFFmpegMediaPlayer::position() const
{
    return 0;
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
    context = nullptr;
    int ret = avformat_open_input(&context, url.constData(), nullptr, nullptr);
    if (ret < 0) {
        error(QMediaPlayer::AccessDeniedError, QMediaPlayer::tr("Could not open file"));
        return;
    }

    ret = avformat_find_stream_info(context, nullptr);
    if (ret < 0) {
        error(QMediaPlayer::FormatError, QMediaPlayer::tr("Could not find stream information for media file"));
        return;
    }
    av_dump_format(context, 0, url.constData(), 0);

    m_metaData = QFFmpegMetaData::fromAVMetaData(context->metadata);
    metaDataChanged();

    // check streams and seekable
    if (context->ctx_flags & AVFMTCTX_NOHEADER) {
        // ### dynamic streams, will get added in read_frame
    } else {
        checkStreams();
    }
    seekableChanged(!(context->ctx_flags & AVFMTCTX_UNSEEKABLE));

}

void QFFmpegMediaPlayer::play()
{
    openCodec(VideoStream, 0);
//    openCodec(AudioStream, 0);
    if (m_streamMap[SubtitleStream].count())
        openCodec(SubtitleStream, 0);
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
    m_sink = sink;
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
    avformat_close_input(&context);
    context = nullptr;
}

void QFFmpegMediaPlayer::checkStreams()
{
    Q_ASSERT(context);

    qint64 duration = 0;
    for (unsigned int i = 0; i < context->nb_streams; ++i) {
        auto *stream = context->streams[i];

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

    if (m_duration != duration) {
        m_duration = duration;
        durationChanged(duration);
    }
}

void QFFmpegMediaPlayer::openCodec(TrackType type, int index)
{
    qDebug() << "openCodec" << type << index;
    int streamIdx = m_streamMap[type].value(index).avStreamIndex;
    if (streamIdx < 0)
        return;
    qDebug() << "    using stream index" << streamIdx;
    // ### return if same track as before

    if (codecContext[type])
        closeCodec(type);

    auto *stream = context->streams[streamIdx];
    AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        error(QMediaPlayer::FormatError, QMediaPlayer::tr("Can't decode track."));
        return;
    }
    codecContext[type] = avcodec_alloc_context3(decoder);
    if (!codecContext[type]) {
        error(QMediaPlayer::FormatError, QMediaPlayer::tr("Failed to allocate a FFmpeg codec context"));
        return;
    }
    int ret = avcodec_parameters_to_context(codecContext[type], stream->codecpar);
    if (ret < 0) {
        error(QMediaPlayer::FormatError, QMediaPlayer::tr("Can't decode track."));
        return;
    }
    /* Init the decoders, with or without reference counting */
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    ret = avcodec_open2(codecContext[type], decoder, &opts);
    if (ret < 0) {
        error(QMediaPlayer::FormatError, QMediaPlayer::tr("Can't decode track."));
        return;
    }
    m_currentStream[type] = streamIdx;
}

void QFFmpegMediaPlayer::closeCodec(TrackType type)
{
    if (codecContext[type])
        avcodec_close(codecContext[type]);
    codecContext[type] = nullptr;
    m_currentStream[type] = -1;
}

QT_BEGIN_NAMESPACE



QT_END_NAMESPACE
