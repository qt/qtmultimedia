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
#include "qffmpegdecoder_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegmediadevices_p.h"
#include "private/qiso639_2_p.h"
#include "qffmpeg_p.h"
#include "qffmpegmediametadata_p.h"
#include "qffmpegvideobuffer_p.h"
#include "private/qplatformaudiooutput_p.h"
#include "qvideosink.h"
#include "qaudiosink.h"
#include "qaudiooutput.h"

#include <qlocale.h>
#include <qthread.h>
#include <qatomic.h>
#include <qwaitcondition.h>
#include <qmutex.h>
#include <qtimer.h>
#include <qqueue.h>

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

QFFmpegMediaPlayer::QFFmpegMediaPlayer(QMediaPlayer *player)
    : QPlatformMediaPlayer(player)
{
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
    return decoder ? decoder->currentTime.loadRelaxed() : 0;
}

void QFFmpegMediaPlayer::setPosition(qint64 position)
{
    if (decoder)
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
    if (decoder)
        delete decoder;
    decoder = nullptr;

    // ### use io device when provided

    QByteArray url = media.toEncoded(QUrl::PreferLocalFile);
    AVFormatContext *context = nullptr;
    int ret = avformat_open_input(&context, url.constData(), nullptr, nullptr);
    if (ret < 0) {
        error(QMediaPlayer::AccessDeniedError, QMediaPlayer::tr("Could not open file"));
        return;
    }
    decoder = new Decoder(this, context);
    decoder->setAudioSink(m_audioOutput);
    decoder->setVideoSink(m_videoSink);

    ret = avformat_find_stream_info(context, nullptr);
    if (ret < 0) {
        error(QMediaPlayer::FormatError, QMediaPlayer::tr("Could not find stream information for media file"));
        return;
    }
    av_dump_format(context, 0, url.constData(), 0);

    m_metaData = QFFmpegMetaData::fromAVMetaData(context->metadata);
    metaDataChanged();

    // check streams and seekable
    checkStreams();

    seekableChanged(!(context->ctx_flags & AVFMTCTX_UNSEEKABLE));
}

void QFFmpegMediaPlayer::play()
{
    if (!decoder)
        return;
    decoder->play();
    stateChanged(QMediaPlayer::PlayingState);
}

void QFFmpegMediaPlayer::pause()
{
    if (!decoder)
        return;
    decoder->pause();
    stateChanged(QMediaPlayer::PausedState);
}

void QFFmpegMediaPlayer::stop()
{
    if (!decoder)
        return;
    decoder->stop();
    stateChanged(QMediaPlayer::StoppedState);
}

void QFFmpegMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;

    m_audioOutput = output;
    if (decoder)
        decoder->setAudioSink(output);
}

void QFFmpegMediaPlayer::setVideoSink(QVideoSink *sink)
{
    if (m_videoSink == sink)
        return;

    m_videoSink = sink;
    if (decoder)
        decoder->setVideoSink(sink);
}

QVideoSink *QFFmpegMediaPlayer::videoSink() const
{
    return m_videoSink;
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

int QFFmpegMediaPlayer::activeTrack(TrackType type)
{
    return m_requestedStreams[type];
}

void QFFmpegMediaPlayer::setActiveTrack(TrackType type, int streamNumber)
{
    if (streamNumber < 0 || streamNumber >= m_streamMap[type].size())
        streamNumber = -1;
    if (m_requestedStreams[type] == streamNumber)
        return;
    m_requestedStreams[type] = streamNumber;
    int avStreamIndex = m_streamMap[type].value(streamNumber).avStreamIndex;
    qDebug() << "setActiveTrack" << streamNumber << avStreamIndex;
    decoder->changeTrack(type, avStreamIndex);
}

void QFFmpegMediaPlayer::checkStreams()
{
    Q_ASSERT(decoder && decoder->context);

    qint64 duration = 0;
    m_requestedStreams[QPlatformMediaPlayer::VideoStream] = -1;
    m_requestedStreams[QPlatformMediaPlayer::AudioStream] = -1;
    m_requestedStreams[QPlatformMediaPlayer::SubtitleStream] = -1;

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
        bool isDefault = stream->disposition & AV_DISPOSITION_DEFAULT;
        if (isDefault && m_requestedStreams[type] < 0)
            m_requestedStreams[type] = m_streamMap[type].size();

        m_streamMap[type].append({ (int)i, isDefault, metaData });
        duration = qMax(duration, 1000*stream->duration*stream->time_base.num/stream->time_base.den);
    }

    if (m_requestedStreams[QPlatformMediaPlayer::VideoStream] < 0 && m_streamMap[QPlatformMediaPlayer::VideoStream].size())
        m_requestedStreams[QPlatformMediaPlayer::VideoStream] = 0;
    if (m_requestedStreams[QPlatformMediaPlayer::AudioStream] < 0 && m_streamMap[QPlatformMediaPlayer::AudioStream].size())
        m_requestedStreams[QPlatformMediaPlayer::AudioStream] = 0;

    tracksChanged();

    if (m_duration != duration) {
        m_duration = duration;
        durationChanged(duration);
    }
}

QT_END_NAMESPACE
