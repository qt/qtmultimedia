/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef AVFMEDIAPLAYERSESSION_H
#define AVFMEDIAPLAYERSESSION_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QSet>
#include <QtCore/QResource>

#include <QtMultimedia/QMediaPlayerControl>
#include <QtMultimedia/QMediaPlayer>

QT_BEGIN_NAMESPACE

class AVFMediaPlayerService;
class AVFVideoOutput;

class AVFMediaPlayerSession : public QObject
{
    Q_OBJECT
public:
    AVFMediaPlayerSession(AVFMediaPlayerService *service, QObject *parent = nullptr);
    virtual ~AVFMediaPlayerSession();

    void setVideoOutput(AVFVideoOutput *output);
    void *currentAssetHandle();

    QMediaPlayer::State state() const;
    QMediaPlayer::MediaStatus mediaStatus() const;

    QMediaContent media() const;
    QIODevice *mediaStream() const;
    void setMedia(const QMediaContent &content, QIODevice *stream);

    qint64 position() const;
    qint64 duration() const;

    int bufferStatus() const;

    int volume() const;
    bool isMuted() const;

    bool isAudioAvailable() const;
    bool isVideoAvailable() const;

    bool isSeekable() const;
    QMediaTimeRange availablePlaybackRanges() const;

    qreal playbackRate() const;

public Q_SLOTS:
    void setPlaybackRate(qreal rate);

    void setPosition(qint64 pos);

    void play();
    void pause();
    void stop();

    void setVolume(int volume);
    void setMuted(bool muted);

    void processEOS();
    void processLoadStateChange(QMediaPlayer::State newState);
    void processPositionChange();
    void processMediaLoadError();

    void processLoadStateChange();
    void processLoadStateFailure();

    void processBufferStateChange(int bufferStatus);

    void processDurationChange(qint64 duration);

    void streamReady();
    void streamDestroyed();

Q_SIGNALS:
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void stateChanged(QMediaPlayer::State newState);
    void bufferStatusChanged(int bufferStatus);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    void audioAvailableChanged(bool audioAvailable);
    void videoAvailableChanged(bool videoAvailable);
    void playbackRateChanged(qreal rate);
    void seekableChanged(bool seekable);
    void error(int error, const QString &errorString);

private:
    void setAudioAvailable(bool available);
    void setVideoAvailable(bool available);
    void setSeekable(bool seekable);
    void resetStream(QIODevice *stream = nullptr);

    AVFMediaPlayerService *m_service;
    AVFVideoOutput *m_videoOutput;

    QMediaPlayer::State m_state;
    QMediaPlayer::MediaStatus m_mediaStatus;
    QIODevice *m_mediaStream;
    QMediaContent m_resources;

    bool m_muted;
    bool m_tryingAsync;
    int m_volume;
    qreal m_rate;
    qint64 m_requestedPosition;

    qint64 m_duration;
    int m_bufferStatus;
    bool m_videoAvailable;
    bool m_audioAvailable;
    bool m_seekable;

    void *m_observer;
};

QT_END_NAMESPACE

#endif // AVFMEDIAPLAYERSESSION_H
