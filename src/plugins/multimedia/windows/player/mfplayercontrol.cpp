// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mfplayercontrol_p.h"
#include "mfplayersession_p.h"
#include "mfvideorenderercontrol_p.h"
#include <qdebug.h>

//#define DEBUG_MEDIAFOUNDATION

MFPlayerControl::MFPlayerControl(QMediaPlayer *player)
    : QPlatformMediaPlayer(player)
    , m_state(QMediaPlayer::StoppedState)
    , m_stateDirty(false)
    , m_videoAvailable(false)
    , m_audioAvailable(false)
    , m_duration(0)
    , m_seekable(false)
{
    m_session = new MFPlayerSession(this);
}

MFPlayerControl::~MFPlayerControl()
{
    m_session->close();
    m_session->setPlayerControl(nullptr);
    m_session->Release();
}

void MFPlayerControl::setMedia(const QUrl &media, QIODevice *stream)
{
    if (m_state != QMediaPlayer::StoppedState) {
        changeState(QMediaPlayer::StoppedState);
        m_session->stop(true);
        refreshState();
    }

    m_media = media;
    m_stream = stream;
    resetAudioVideoAvailable();
    handleDurationUpdate(0);
    handleSeekableUpdate(false);
    m_session->load(media, stream);
}

void MFPlayerControl::play()
{
    if (m_state == QMediaPlayer::PlayingState)
        return;
    resetCurrentLoop();
    if (QMediaPlayer::InvalidMedia == m_session->status())
        m_session->load(m_media, m_stream);

    switch (m_session->status()) {
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::InvalidMedia:
        return;
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
    case QMediaPlayer::EndOfMedia:
        changeState(QMediaPlayer::PlayingState);
        m_session->start();
        break;
    default: //Loading/Stalled
        changeState(QMediaPlayer::PlayingState);
        break;
    }
    refreshState();
}

void MFPlayerControl::pause()
{
    if (m_state == QMediaPlayer::PausedState)
        return;

    if (m_session->status() == QMediaPlayer::NoMedia ||
        m_session->status() == QMediaPlayer::InvalidMedia)
        return;

    changeState(QMediaPlayer::PausedState);
    m_session->pause();
    refreshState();
}

void MFPlayerControl::stop()
{
    if (m_state == QMediaPlayer::StoppedState)
        return;
    changeState(QMediaPlayer::StoppedState);
    m_session->stop();
    refreshState();
}

QMediaMetaData MFPlayerControl::metaData() const
{
    return m_session->metaData();
}

void MFPlayerControl::setAudioOutput(QPlatformAudioOutput *output)
{
    m_session->setAudioOutput(output);
}

void MFPlayerControl::setVideoSink(QVideoSink *sink)
{
    m_session->setVideoSink(sink);
}

void MFPlayerControl::changeState(QMediaPlayer::PlaybackState state)
{
    if (m_state == state)
        return;
    m_state = state;
    m_stateDirty = true;
}

void MFPlayerControl::refreshState()
{
    if (!m_stateDirty)
        return;
    m_stateDirty = false;
#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "MFPlayerControl::emit stateChanged" << m_state;
#endif
    stateChanged(m_state);
}

void MFPlayerControl::handleStatusChanged()
{
    QMediaPlayer::MediaStatus status = m_session->status();
    switch (status) {
    case QMediaPlayer::EndOfMedia:
        if (doLoop()) {
            setPosition(0);
            m_session->start();
        } else {
            changeState(QMediaPlayer::StoppedState);
        }
        break;
    case QMediaPlayer::InvalidMedia:
        break;
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        if (m_state == QMediaPlayer::PlayingState)
            m_session->start();
        break;
    default:
        break;
    }
    mediaStatusChanged(m_session->status());
    refreshState();
}

void MFPlayerControl::handleTracksChanged()
{
    tracksChanged();
}

void MFPlayerControl::handleVideoAvailable()
{
    if (m_videoAvailable)
        return;
    m_videoAvailable = true;
    videoAvailableChanged(m_videoAvailable);
}

void MFPlayerControl::handleAudioAvailable()
{
    if (m_audioAvailable)
        return;
    m_audioAvailable = true;
    audioAvailableChanged(m_audioAvailable);
}

void MFPlayerControl::resetAudioVideoAvailable()
{
    bool videoDirty = false;
    if (m_videoAvailable) {
        m_videoAvailable = false;
        videoDirty = true;
    }
    if (m_audioAvailable) {
        m_audioAvailable = false;
        audioAvailableChanged(m_audioAvailable);
    }
    if (videoDirty)
        videoAvailableChanged(m_videoAvailable);
}

void MFPlayerControl::handleDurationUpdate(qint64 duration)
{
    if (m_duration == duration)
        return;
    m_duration = duration;
    durationChanged(m_duration);
}

void MFPlayerControl::handleSeekableUpdate(bool seekable)
{
    if (m_seekable == seekable)
        return;
    m_seekable = seekable;
    seekableChanged(m_seekable);
}

QMediaPlayer::PlaybackState MFPlayerControl::state() const
{
    return m_state;
}

QMediaPlayer::MediaStatus MFPlayerControl::mediaStatus() const
{
    return m_session->status();
}

qint64 MFPlayerControl::duration() const
{
    return m_duration;
}

qint64 MFPlayerControl::position() const
{
    return m_session->position();
}

void MFPlayerControl::setPosition(qint64 position)
{
    if (!m_seekable || position == m_session->position())
        return;
    m_session->setPosition(position);
}

float MFPlayerControl::bufferProgress() const
{
    return m_session->bufferProgress() / 100.;
}

bool MFPlayerControl::isAudioAvailable() const
{
    return m_audioAvailable;
}

bool MFPlayerControl::isVideoAvailable() const
{
    return m_videoAvailable;
}

bool MFPlayerControl::isSeekable() const
{
    return m_seekable;
}

QMediaTimeRange MFPlayerControl::availablePlaybackRanges() const
{
    return m_session->availablePlaybackRanges();
}

qreal MFPlayerControl::playbackRate() const
{
    return m_session->playbackRate();
}

void MFPlayerControl::setPlaybackRate(qreal rate)
{
    m_session->setPlaybackRate(rate);
}

QUrl MFPlayerControl::media() const
{
    return m_media;
}

const QIODevice* MFPlayerControl::mediaStream() const
{
    return m_stream;
}

void MFPlayerControl::handleError(QMediaPlayer::Error errorCode, const QString& errorString, bool isFatal)
{
    if (isFatal)
        stop();
    error(int(errorCode), errorString);
}

void MFPlayerControl::setActiveTrack(TrackType type, int index)
{
    m_session->setActiveTrack(type, index);
}

int MFPlayerControl::activeTrack(TrackType type)
{
    return m_session->activeTrack(type);
}

int MFPlayerControl::trackCount(TrackType type)
{
    return m_session->trackCount(type);
}

QMediaMetaData MFPlayerControl::trackMetaData(TrackType type, int trackNumber)
{
    return m_session->trackMetaData(type, trackNumber);
}

