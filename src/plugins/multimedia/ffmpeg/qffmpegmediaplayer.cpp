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
#include "qvideosink.h"

#include <qlocale.h>
#include <qthread.h>
#include <qatomic.h>

static float timeStamp(qint64 ts, AVRational base)
{
    return 1000*ts*base.num/(float)base.den;
}

class QFFmpegDecoder : public QObject
{
public:
    QFFmpegDecoder(QFFmpegMediaPlayer *player)
        : QObject()
        , m_player(player)
    {
        startTimer(20, Qt::PreciseTimer);
    }

    void timerEvent(QTimerEvent *)
    {
        if (state == Play) {
            qDebug() << "timer Event";
            AVPacket packet;
            av_init_packet(&packet);
            if (av_read_frame(m_player->context, &packet) >= 0) {
                auto base = m_player->context->streams[packet.stream_index]->time_base;
                qDebug() << "    read a packet: track" << packet.stream_index
                         << timeStamp(packet.dts, base) << timeStamp(packet.pts, base) << timeStamp(packet.duration, base);
                int trackType = -1;
    //            for (int i = 0; i < QPlatformMediaPlayer::NTrackTypes; ++i)
                    if (packet.stream_index == m_player->m_currentStream[QPlatformMediaPlayer::VideoStream])
                        trackType = QPlatformMediaPlayer::VideoStream;

                if (trackType >= 0) {
                    // send the frame to the decoder
                    avcodec_send_packet(m_player->codecContext[trackType], &packet);
                    qDebug() << "   sent a packet to decoder";
                }
            }
            av_packet_unref(&packet);

            AVFrame *frame = av_frame_alloc();
            int ret = avcodec_receive_frame(m_player->codecContext[QPlatformMediaPlayer::VideoStream], frame);
            if (ret == 0) {
                auto base = m_player->context->streams[m_player->m_currentStream[QPlatformMediaPlayer::VideoStream]]->time_base;
                qDebug() << "    got a frame" << timeStamp(frame->pts, base);
                QVideoSink *sink = m_player->videoSink();
                if (sink) {
                    QFFmpegVideoBuffer *buffer = new QFFmpegVideoBuffer(frame);
                    QVideoFrameFormat format(buffer->size(), buffer->pixelFormat());
                    QVideoFrame frame(buffer, format);
                    sink->setVideoFrame(frame);
                }
            }
        } else if (state == Stop) {
            QVideoSink *sink = m_player->videoSink();
            if (sink)
                sink->setVideoFrame({});
        }
    }

    enum State {
        Pause,
        Play,
        Stop,
        Exit
    };

    QFFmpegMediaPlayer *m_player = nullptr;
    State state = Pause;
};


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
    decoder->state = QFFmpegDecoder::Play;
    stateChanged(QMediaPlayer::PlayingState);
}

void QFFmpegMediaPlayer::pause()
{
    decoder->state = QFFmpegDecoder::Pause;
    stateChanged(QMediaPlayer::PausedState);
}

void QFFmpegMediaPlayer::stop()
{
    decoder->state = QFFmpegDecoder::Stop;
    stateChanged(QMediaPlayer::StoppedState);
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
    int streamIdx = m_streamMap[type].value(index).avStreamIndex;
    if (streamIdx < 0)
        return;
    // ### return is same track as before

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
