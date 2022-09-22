// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    positionUpdateTimer.setInterval(100);
    positionUpdateTimer.setTimerType(Qt::PreciseTimer);
    connect(&positionUpdateTimer, &QTimer::timeout, this, &QFFmpegMediaPlayer::updatePosition);
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
    if (decoder) {
        decoder->seek(position * 1000);
        updatePosition();
    }
    if (state() == QMediaPlayer::StoppedState)
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
}

void QFFmpegMediaPlayer::updatePosition()
{
    positionChanged(decoder ? decoder->clockController.currentTime() / 1000 : 0);
}

void QFFmpegMediaPlayer::endOfStream()
{
    positionChanged(duration());
    stateChanged(QMediaPlayer::StoppedState);
    mediaStatusChanged(QMediaPlayer::EndOfMedia);
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
    decoder = new Decoder;
    connect(decoder, &Decoder::endOfStream, this, &QFFmpegMediaPlayer::endOfStream);
    connect(decoder, &Decoder::errorOccured, this, &QFFmpegMediaPlayer::error);
    decoder->setMedia(media, stream);
    decoder->setAudioSink(m_audioOutput);
    decoder->setVideoSink(m_videoSink);

    durationChanged(duration());
    tracksChanged();
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

    if (mediaStatus() == QMediaPlayer::EndOfMedia && state() == QMediaPlayer::StoppedState) {
        decoder->seek(0);
        positionChanged(0);
    }
    decoder->play();
    positionUpdateTimer.start();
    stateChanged(QMediaPlayer::PlayingState);
    mediaStatusChanged(QMediaPlayer::BufferedMedia);
}

void QFFmpegMediaPlayer::pause()
{
    if (!decoder)
        return;
    if (mediaStatus() == QMediaPlayer::EndOfMedia && state() == QMediaPlayer::StoppedState) {
        decoder->seek(0);
        positionChanged(0);
    }
    decoder->pause();
    positionUpdateTimer.stop();
    stateChanged(QMediaPlayer::PausedState);
    mediaStatusChanged(QMediaPlayer::BufferedMedia);
}

void QFFmpegMediaPlayer::stop()
{
    if (!decoder)
        return;
    decoder->stop();
    positionUpdateTimer.stop();
    positionChanged(0);
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
