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

#include <qlocale.h>

QFFmpegMediaPlayer::QFFmpegMediaPlayer(QMediaPlayer *player)
    : QPlatformMediaPlayer(player)
{

}

QFFmpegMediaPlayer::~QFFmpegMediaPlayer()
{
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
    // ### use stream when provided

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

}

void QFFmpegMediaPlayer::pause()
{

}

void QFFmpegMediaPlayer::stop()
{

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

        QMediaMetaData metaData;
        TrackType type = VideoStream;
        auto *avMetaData = stream->metadata;
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

        // get title and language
        auto *lang = av_dict_get(avMetaData, "language", nullptr, 0);
        if (lang) {
            qDebug() << "got lang" << lang->value << QtMultimediaPrivate::fromIso639(lang->value);
            metaData.insert(QMediaMetaData::Language, QVariant::fromValue(QtMultimediaPrivate::fromIso639(lang->value)));
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

QT_BEGIN_NAMESPACE



QT_END_NAMESPACE
