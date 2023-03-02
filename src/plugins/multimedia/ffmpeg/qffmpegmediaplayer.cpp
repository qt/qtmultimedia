// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediaplayer_p.h"
#include "private/qplatformaudiooutput_p.h"
#include "qvideosink.h"
#include "qaudiooutput.h"

#include "qffmpegplaybackengine_p.h"
#include <qiodevice.h>
#include <qvideosink.h>
#include <qtimer.h>

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

QFFmpegMediaPlayer::QFFmpegMediaPlayer(QMediaPlayer *player)
    : QPlatformMediaPlayer(player)
{
    m_positionUpdateTimer.setInterval(50);
    m_positionUpdateTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_positionUpdateTimer, &QTimer::timeout, this, &QFFmpegMediaPlayer::updatePosition);
}

QFFmpegMediaPlayer::~QFFmpegMediaPlayer() = default;

qint64 QFFmpegMediaPlayer::duration() const
{
    return m_playbackEngine ? m_playbackEngine->duration() / 1000 : 0;
}

void QFFmpegMediaPlayer::setPosition(qint64 position)
{
    if (m_playbackEngine) {
        m_playbackEngine->seek(position * 1000);
        updatePosition();
    }
    if (state() == QMediaPlayer::StoppedState)
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
}

void QFFmpegMediaPlayer::updatePosition()
{
    positionChanged(m_playbackEngine ? m_playbackEngine->currentPosition() / 1000 : 0);
}

void QFFmpegMediaPlayer::endOfStream()
{
    // start update timer and report end position anyway
    m_positionUpdateTimer.stop();
    positionChanged(duration());

    if (doLoop()) {
        m_playbackEngine->seek(0);
        positionChanged(0);

        runPlayback();
    } else {
        stateChanged(QMediaPlayer::StoppedState);
        mediaStatusChanged(QMediaPlayer::EndOfMedia);
    }
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
    if (m_playbackEngine)
        m_playbackEngine->setPlaybackRate(rate);
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
    m_playbackEngine = nullptr;

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
    m_playbackEngine = std::make_unique<PlaybackEngine>();
    connect(m_playbackEngine.get(), &PlaybackEngine::endOfStream, this,
            &QFFmpegMediaPlayer::endOfStream);
    connect(m_playbackEngine.get(), &PlaybackEngine::errorOccured, this,
            &QFFmpegMediaPlayer::error);

    if (!m_playbackEngine->setMedia(media, stream)) {
        m_playbackEngine.reset();
        handleIncorrectMedia(QMediaPlayer::InvalidMedia);
        return;
    }

    m_playbackEngine->setAudioSink(m_audioOutput);
    m_playbackEngine->setVideoSink(m_videoSink);

    durationChanged(duration());
    tracksChanged();
    metaDataChanged();
    seekableChanged(m_playbackEngine->isSeekable());

    audioAvailableChanged(
            !m_playbackEngine->streamInfo(QPlatformMediaPlayer::AudioStream).isEmpty());
    videoAvailableChanged(
            !m_playbackEngine->streamInfo(QPlatformMediaPlayer::VideoStream).isEmpty());

    QMetaObject::invokeMethod(this, "delayedLoadedStatus", Qt::QueuedConnection);
}

void QFFmpegMediaPlayer::play()
{
    if (!m_playbackEngine)
        return;

    if (mediaStatus() == QMediaPlayer::EndOfMedia && state() == QMediaPlayer::StoppedState) {
        m_playbackEngine->seek(0);
        positionChanged(0);
        resetCurrentLoop();
    }

    runPlayback();
}

void QFFmpegMediaPlayer::runPlayback()
{
    m_playbackEngine->play();
    m_positionUpdateTimer.start();
    stateChanged(QMediaPlayer::PlayingState);
    mediaStatusChanged(QMediaPlayer::BufferedMedia);
}

void QFFmpegMediaPlayer::pause()
{
    if (!m_playbackEngine)
        return;
    if (mediaStatus() == QMediaPlayer::EndOfMedia && state() == QMediaPlayer::StoppedState) {
        m_playbackEngine->seek(0);
        positionChanged(0);
    }
    m_playbackEngine->pause();
    m_positionUpdateTimer.stop();
    stateChanged(QMediaPlayer::PausedState);
    mediaStatusChanged(QMediaPlayer::BufferedMedia);
}

void QFFmpegMediaPlayer::stop()
{
    if (!m_playbackEngine)
        return;
    m_playbackEngine->stop();
    m_positionUpdateTimer.stop();
    positionChanged(0);
    stateChanged(QMediaPlayer::StoppedState);
    mediaStatusChanged(QMediaPlayer::LoadedMedia);
}

void QFFmpegMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;

    m_audioOutput = output;
    if (m_playbackEngine)
        m_playbackEngine->setAudioSink(output);
}

QMediaMetaData QFFmpegMediaPlayer::metaData() const
{
    return m_playbackEngine ? m_playbackEngine->metaData() : QMediaMetaData{};
}

void QFFmpegMediaPlayer::setVideoSink(QVideoSink *sink)
{
    if (m_videoSink == sink)
        return;

    m_videoSink = sink;
    if (m_playbackEngine)
        m_playbackEngine->setVideoSink(sink);
}

QVideoSink *QFFmpegMediaPlayer::videoSink() const
{
    return m_videoSink;
}

int QFFmpegMediaPlayer::trackCount(TrackType type)
{
    return m_playbackEngine ? m_playbackEngine->streamInfo(type).count() : 0;
}

QMediaMetaData QFFmpegMediaPlayer::trackMetaData(TrackType type, int streamNumber)
{
    if (!m_playbackEngine || streamNumber < 0
        || streamNumber >= m_playbackEngine->streamInfo(type).count())
        return {};
    return m_playbackEngine->streamInfo(type).at(streamNumber).metaData;
}

int QFFmpegMediaPlayer::activeTrack(TrackType type)
{
    return m_playbackEngine ? m_playbackEngine->activeTrack(type) : -1;
}

void QFFmpegMediaPlayer::setActiveTrack(TrackType type, int streamNumber)
{
    if (m_playbackEngine)
        m_playbackEngine->setActiveTrack(type, streamNumber);
}

QT_END_NAMESPACE

#include "moc_qffmpegmediaplayer_p.cpp"
