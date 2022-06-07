// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMOCKMEDIAPLAYER_H
#define QMOCKMEDIAPLAYER_H

#include "private/qplatformmediaplayer_p.h"
#include <qurl.h>

QT_BEGIN_NAMESPACE

class QMockMediaPlayer : public QPlatformMediaPlayer
{
    friend class QMockMediaPlayerService;

public:
    QMockMediaPlayer(QMediaPlayer *parent)
        : QPlatformMediaPlayer(parent)
        , _state(QMediaPlayer::StoppedState)
        , _error(QMediaPlayer::NoError)
        , _duration(0)
        , _position(0)
        , _bufferProgress(0)
        , _audioAvailable(false)
        , _videoAvailable(false)
        , _isSeekable(true)
        , _playbackRate(qreal(1.0))
        , _stream(0)
        , _isValid(false)
    {
    }
    ~QMockMediaPlayer()
    {
    }

    QMediaPlayer::PlaybackState state() const override { return _state; }
    void updateState(QMediaPlayer::PlaybackState state) { setState(state); }
    void updateMediaStatus(QMediaPlayer::MediaStatus status, QMediaPlayer::PlaybackState state)
    {
        _state = state;

        mediaStatusChanged(status);
        stateChanged(_state);
    }

    qint64 duration() const override { return _duration; }
    void setDuration(qint64 duration) { emit durationChanged(_duration = duration); }

    qint64 position() const override { return _position; }

    void setPosition(qint64 position) override { if (position != _position) emit positionChanged(_position = position); }

    float bufferProgress() const override { return _bufferProgress; }
    void setBufferStatus(float status)
    {
        if (_bufferProgress == status)
            return;
        _bufferProgress = status;
        bufferProgressChanged(status);
    }

    bool isAudioAvailable() const override { return _audioAvailable; }
    bool isVideoAvailable() const override { return _videoAvailable; }

    bool isSeekable() const override { return _isSeekable; }
    void setSeekable(bool seekable) { emit seekableChanged(_isSeekable = seekable); }

    QMediaTimeRange availablePlaybackRanges() const override { return QMediaTimeRange(_seekRange.first, _seekRange.second); }
    void setSeekRange(qint64 minimum, qint64 maximum) { _seekRange = qMakePair(minimum, maximum); }

    qreal playbackRate() const override { return _playbackRate; }
    void setPlaybackRate(qreal rate) override { if (rate != _playbackRate) emit playbackRateChanged(_playbackRate = rate); }

    QUrl media() const override { return _media; }
    void setMedia(const QUrl &content, QIODevice *stream) override
    {
        _stream = stream;
        _media = content;
        setState(QMediaPlayer::StoppedState);
        mediaStatusChanged(_media.isEmpty() ? QMediaPlayer::NoMedia : QMediaPlayer::LoadingMedia);
    }
    QIODevice *mediaStream() const override { return _stream; }

    bool streamPlaybackSupported() const override { return m_supportsStreamPlayback; }
    void setStreamPlaybackSupported(bool b) { m_supportsStreamPlayback = b; }

    void play() override { if (_isValid && !_media.isEmpty()) setState(QMediaPlayer::PlayingState); }
    void pause() override { if (_isValid && !_media.isEmpty()) setState(QMediaPlayer::PausedState); }
    void stop() override { if (_state != QMediaPlayer::StoppedState) setState(QMediaPlayer::StoppedState); }

    void setVideoSink(QVideoSink *) override {}

    void setAudioOutput(QPlatformAudioOutput *output) override { m_audioOutput = output; }

    void emitError(QMediaPlayer::Error err, const QString &errorString)
    {
        emit error(err, errorString);
    }

    void setState(QMediaPlayer::PlaybackState state)
    {
        if (_state == state)
            return;
        _state = state;
        stateChanged(state);
    }
    void setState(QMediaPlayer::PlaybackState state, QMediaPlayer::MediaStatus status)
    {
        _state = state;
        mediaStatusChanged(status);
        stateChanged(state);
    }
    void setMediaStatus(QMediaPlayer::MediaStatus status)
    {
        if (status == QMediaPlayer::StalledMedia || status == QMediaPlayer::BufferingMedia)
            bufferProgressChanged(_bufferProgress);
        mediaStatusChanged(status);
    }
    void setIsValid(bool isValid) { _isValid = isValid; }
    void setMedia(QUrl media) { _media = media; }
    void setVideoAvailable(bool videoAvailable) { _videoAvailable = videoAvailable; }
    void setError(QMediaPlayer::Error err) { _error = err; emit error(_error, _errorString); }
    void setErrorString(QString errorString) { _errorString = errorString; emit error(_error, _errorString); }

    void reset()
    {
        _state = QMediaPlayer::StoppedState;
        _error = QMediaPlayer::NoError;
        _duration = 0;
        _position = 0;
        _bufferProgress = 0;
        _videoAvailable = false;
        _isSeekable = false;
        _playbackRate = 0.0;
        _media = QUrl();
        _stream = 0;
        _isValid = false;
        _errorString = QString();
    }


    QMediaPlayer::PlaybackState _state;
    QMediaPlayer::Error _error;
    qint64 _duration;
    qint64 _position;
    float _bufferProgress;
    bool _audioAvailable;
    bool _videoAvailable;
    bool _isSeekable;
    QPair<qint64, qint64> _seekRange;
    qreal _playbackRate;
    QUrl _media;
    QIODevice *_stream;
    bool _isValid;
    QString _errorString;
    bool m_supportsStreamPlayback = false;
    QPlatformAudioOutput *m_audioOutput = nullptr;
};

QT_END_NAMESPACE

#endif // QMOCKMEDIAPLAYER_H
