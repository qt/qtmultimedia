/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKMEDIAPLAYERCONTROL_H
#define MOCKMEDIAPLAYERCONTROL_H

#include "qmediaplayercontrol.h"
#include <qurl.h>

class MockMediaPlayerControl : public QMediaPlayerControl
{
    friend class MockMediaPlayerService;

public:
    MockMediaPlayerControl()
        : QMediaPlayerControl(0)
        , _state(QMediaPlayer::StoppedState)
        , _mediaStatus(QMediaPlayer::NoMedia)
        , _error(QMediaPlayer::NoError)
        , _duration(0)
        , _position(0)
        , _volume(100)
        , _muted(false)
        , _bufferStatus(0)
        , _audioAvailable(false)
        , _videoAvailable(false)
        , _isSeekable(true)
        , _playbackRate(qreal(1.0))
        , _stream(0)
        , _isValid(false)
    {}

    QMediaPlayer::State state() const { return _state; }
    void updateState(QMediaPlayer::State state) { emit stateChanged(_state = state); }
    QMediaPlayer::MediaStatus mediaStatus() const { return _mediaStatus; }
    void updateMediaStatus(QMediaPlayer::MediaStatus status)
    {
        emit mediaStatusChanged(_mediaStatus = status);
    }
    void updateMediaStatus(QMediaPlayer::MediaStatus status, QMediaPlayer::State state)
    {
        _mediaStatus = status;
        _state = state;

        emit mediaStatusChanged(_mediaStatus);
        emit stateChanged(_state);
    }

    qint64 duration() const { return _duration; }
    void setDuration(qint64 duration) { emit durationChanged(_duration = duration); }

    qint64 position() const { return _position; }

    void setPosition(qint64 position) { if (position != _position) emit positionChanged(_position = position); }

    int volume() const { return _volume; }
    void setVolume(int volume) { emit volumeChanged(_volume = volume); }

    bool isMuted() const { return _muted; }
    void setMuted(bool muted) { if (muted != _muted) emit mutedChanged(_muted = muted); }

    int bufferStatus() const { return _bufferStatus; }
    void setBufferStatus(int status) { emit bufferStatusChanged(_bufferStatus = status); }

    bool isAudioAvailable() const { return _audioAvailable; }
    bool isVideoAvailable() const { return _videoAvailable; }

    bool isSeekable() const { return _isSeekable; }
    void setSeekable(bool seekable) { emit seekableChanged(_isSeekable = seekable); }

    QMediaTimeRange availablePlaybackRanges() const { return QMediaTimeRange(_seekRange.first, _seekRange.second); }
    void setSeekRange(qint64 minimum, qint64 maximum) { _seekRange = qMakePair(minimum, maximum); }

    qreal playbackRate() const { return _playbackRate; }
    void setPlaybackRate(qreal rate) { if (rate != _playbackRate) emit playbackRateChanged(_playbackRate = rate); }

    QUrl media() const { return _media; }
    void setMedia(const QUrl &content, QIODevice *stream)
    {
        _stream = stream;
        _media = content;
        _mediaStatus = _media.isEmpty() ? QMediaPlayer::NoMedia : QMediaPlayer::LoadingMedia;
        if (_state != QMediaPlayer::StoppedState)
            emit stateChanged(_state = QMediaPlayer::StoppedState);
        emit mediaStatusChanged(_mediaStatus);
    }
    QIODevice *mediaStream() const { return _stream; }

    void play() { if (_isValid && !_media.isEmpty() && _state != QMediaPlayer::PlayingState) emit stateChanged(_state = QMediaPlayer::PlayingState); }
    void pause() { if (_isValid && !_media.isEmpty() && _state != QMediaPlayer::PausedState) emit stateChanged(_state = QMediaPlayer::PausedState); }
    void stop() { if (_state != QMediaPlayer::StoppedState) emit stateChanged(_state = QMediaPlayer::StoppedState); }

    void setAudioRole(QAudio::Role role)
    {
        if (hasAudioRole && role != m_audioRole)
            emit audioRoleChanged(m_audioRole = role);
    }

    QList<QAudio::Role> supportedAudioRoles() const
    {
        if (!hasAudioRole)
            return {};
        return QList<QAudio::Role>() << QAudio::MusicRole
                                     << QAudio::AlarmRole
                                     << QAudio::NotificationRole;
    }

    void setCustomAudioRole(const QString &role)
    {
        m_customAudioRole = role;
        m_audioRole = QAudio::CustomRole;
    }

    QStringList supportedCustomAudioRoles() const
    {
        if (!hasCustomAudioRole)
            return {};
        return QStringList() << QStringLiteral("customRole")
                             << QStringLiteral("customRole2");
    }

    void setVideoSurface(QAbstractVideoSurface *) {}


    void emitError(QMediaPlayer::Error err, const QString &errorString)
    {
        emit error(err, errorString);
    }

    bool hasAudioRole = true;
    bool hasCustomAudioRole = true;
    QAudio::Role m_audioRole = QAudio::UnknownRole;
    QString m_customAudioRole;

    QMediaPlayer::State _state;
    QMediaPlayer::MediaStatus _mediaStatus;
    QMediaPlayer::Error _error;
    qint64 _duration;
    qint64 _position;
    int _volume;
    bool _muted;
    int _bufferStatus;
    bool _audioAvailable;
    bool _videoAvailable;
    bool _isSeekable;
    QPair<qint64, qint64> _seekRange;
    qreal _playbackRate;
    QUrl _media;
    QIODevice *_stream;
    bool _isValid;
    QString _errorString;
};

#endif // MOCKMEDIAPLAYERCONTROL_H
