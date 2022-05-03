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
#include "qlocale.h"
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
    return decoder ? decoder->m_duration/1000 : 0;
}

void QFFmpegMediaPlayer::setPosition(qint64 position)
{
    if (decoder)
        decoder->seek(position*1000);
    if (state() == QMediaPlayer::StoppedState)
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
}

float QFFmpegMediaPlayer::bufferProgress() const
{
    return 1.;
}

QMediaTimeRange QFFmpegMediaPlayer::availablePlaybackRanges() const
{
    return {};
}

qreal QFFmpegMediaPlayer::playbackRate() const
{
    return m_playbackRate;
}

void QFFmpegMediaPlayer::setPlaybackRate(qreal rate)
{
    if (m_playbackRate == rate)
        return;
    m_playbackRate = rate;
    if (decoder)
        decoder->setPlaybackRate(rate);
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
    if (decoder)
        delete decoder;
    decoder = nullptr;

    positionChanged(0);

    if (media.isEmpty() && !stream) {
        seekableChanged(false);
        audioAvailableChanged(false);
        videoAvailableChanged(false);
        metaDataChanged();
        mediaStatusChanged(QMediaPlayer::NoMedia);
        return;
    }

    mediaStatusChanged(QMediaPlayer::LoadingMedia);
    decoder = new Decoder(this);
    decoder->setMedia(media, stream);
    decoder->setAudioSink(m_audioOutput);
    decoder->setVideoSink(m_videoSink);

    metaDataChanged();
    seekableChanged(decoder->isSeekable());

    audioAvailableChanged(!decoder->m_streamMap[QPlatformMediaPlayer::AudioStream].isEmpty());
    videoAvailableChanged(!decoder->m_streamMap[QPlatformMediaPlayer::VideoStream].isEmpty());

    QMetaObject::invokeMethod(this, "delayedLoadedStatus", Qt::QueuedConnection);
}

void QFFmpegMediaPlayer::play()
{
    if (!decoder)
        return;

    if (mediaStatus() == QMediaPlayer::EndOfMedia && state() == QMediaPlayer::StoppedState)
        decoder->seek(0);
    decoder->play();
    stateChanged(QMediaPlayer::PlayingState);
    mediaStatusChanged(QMediaPlayer::BufferedMedia);
}

void QFFmpegMediaPlayer::pause()
{
    if (!decoder)
        return;
    if (mediaStatus() == QMediaPlayer::EndOfMedia && state() == QMediaPlayer::StoppedState)
        decoder->seek(0);
    decoder->pause();
    stateChanged(QMediaPlayer::PausedState);
    mediaStatusChanged(QMediaPlayer::BufferedMedia);
}

void QFFmpegMediaPlayer::stop()
{
    if (!decoder)
        return;
    decoder->stop();
    stateChanged(QMediaPlayer::StoppedState);
    mediaStatusChanged(QMediaPlayer::LoadedMedia);
}

void QFFmpegMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;

    m_audioOutput = output;
    if (decoder)
        decoder->setAudioSink(output);
}

QMediaMetaData QFFmpegMediaPlayer::metaData() const
{
    return decoder ? decoder->m_metaData : QMediaMetaData{};
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
    return decoder ? decoder->m_streamMap[type].count() : 0;
}

QMediaMetaData QFFmpegMediaPlayer::trackMetaData(TrackType type, int streamNumber)
{
    if (!decoder || streamNumber < 0 || streamNumber >= decoder->m_streamMap[type].count())
        return {};
    return decoder->m_streamMap[type].at(streamNumber).metaData;
}

int QFFmpegMediaPlayer::activeTrack(TrackType type)
{
    return decoder ? decoder->m_requestedStreams[type] : -1;
}

void QFFmpegMediaPlayer::setActiveTrack(TrackType type, int streamNumber)
{
    if (decoder)
        decoder->setActiveTrack(type, streamNumber);
}

QT_END_NAMESPACE
