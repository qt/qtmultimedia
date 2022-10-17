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

#include "qffmpegplaybackengine_p.h"

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
    positionUpdateTimer.setInterval(50);
    positionUpdateTimer.setTimerType(Qt::PreciseTimer);
    connect(&positionUpdateTimer, &QTimer::timeout, this, &QFFmpegMediaPlayer::updatePosition);
}

QFFmpegMediaPlayer::~QFFmpegMediaPlayer() = default;

qint64 QFFmpegMediaPlayer::duration() const
{
    return decoder ? decoder->m_duration / 1000 : 0;
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
    positionChanged(decoder ? decoder->currentPosition() / 1000 : 0);
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
    decoder = nullptr;

    positionChanged(0);

    auto handleIncorrectMedia = [this](QMediaPlayer::MediaStatus status) {
        seekableChanged(false);
        audioAvailableChanged(false);
        videoAvailableChanged(false);
        metaDataChanged();
        mediaStatusChanged(status);
    };

    if (media.isEmpty() && !stream) {
        handleIncorrectMedia(QMediaPlayer::NoMedia);
        return;
    }

    mediaStatusChanged(QMediaPlayer::LoadingMedia);
    decoder = std::make_unique<Decoder>();
    connect(decoder.get(), &Decoder::endOfStream, this, &QFFmpegMediaPlayer::endOfStream);
    connect(decoder.get(), &Decoder::errorOccured, this, &QFFmpegMediaPlayer::error);

    if (!decoder->setMedia(media, stream)) {
        decoder.reset();
        handleIncorrectMedia(QMediaPlayer::InvalidMedia);
        return;
    }

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
    return decoder ? decoder->activeTrack(type) : -1;
}

void QFFmpegMediaPlayer::setActiveTrack(TrackType type, int streamNumber)
{
    if (decoder)
        decoder->setActiveTrack(type, streamNumber);
}

QT_END_NAMESPACE
