/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGSTREAMERPLAYERCONTROL_H
#define QGSTREAMERPLAYERCONTROL_H

#include <QtCore/qobject.h>
#include <QtCore/qstack.h>

#include <qmediaplayercontrol.h>
#include <qmediaplayer.h>

#include <limits.h>

class QMediaPlayerResourceSetInterface;

QT_BEGIN_NAMESPACE
class QMediaPlaylist;
class QMediaPlaylistNavigator;
class QSocketNotifier;

class QGstreamerPlayerSession;
class QGstreamerPlayerService;

class QGstreamerPlayerControl : public QMediaPlayerControl
{
    Q_OBJECT
    Q_PROPERTY(bool mediaDownloadEnabled READ isMediaDownloadEnabled WRITE setMediaDownloadEnabled)

public:
    QGstreamerPlayerControl(QGstreamerPlayerSession *session, QObject *parent = 0);
    ~QGstreamerPlayerControl();

    QMediaPlayer::State state() const;
    QMediaPlayer::MediaStatus mediaStatus() const;

    qint64 position() const;
    qint64 duration() const;

    int bufferStatus() const;

    int volume() const;
    bool isMuted() const;

    bool isAudioAvailable() const;
    bool isVideoAvailable() const;
    void setVideoOutput(QObject *output);

    bool isSeekable() const;
    QMediaTimeRange availablePlaybackRanges() const;

    qreal playbackRate() const;
    void setPlaybackRate(qreal rate);

    QMediaContent media() const;
    const QIODevice *mediaStream() const;
    void setMedia(const QMediaContent&, QIODevice *);

    bool isMediaDownloadEnabled() const;
    void setMediaDownloadEnabled(bool enabled);

    QMediaPlayerResourceSetInterface* resources() const;

public Q_SLOTS:
    void setPosition(qint64 pos);

    void play();
    void pause();
    void stop();

    void setVolume(int volume);
    void setMuted(bool muted);

private Q_SLOTS:
    void writeFifo();
    void fifoReadyWrite(int socket);

    void updateSessionState(QMediaPlayer::State state);
    void updateMediaStatus();
    void processEOS();
    void setBufferProgress(int progress);
    void applyPendingSeek(bool isSeekable);
    void updatePosition(qint64 pos);

    void handleInvalidMedia();

    void handleResourcesGranted();
    void handleResourcesLost();
    void handleResourcesDenied();

private:
    bool openFifo();
    void closeFifo();
    void playOrPause(QMediaPlayer::State state);

    void pushState();
    void popAndNotifyState();

    bool m_ownStream;
    QGstreamerPlayerSession *m_session;
    QMediaPlayer::State m_state;
    QMediaPlayer::MediaStatus m_mediaStatus;
    QStack<QMediaPlayer::State> m_stateStack;
    QStack<QMediaPlayer::MediaStatus> m_mediaStatusStack;

    int m_bufferProgress;
    bool m_seekToStartPending;
    qint64 m_pendingSeekPosition;
    QMediaContent m_currentResource;
    QIODevice *m_stream;
    QSocketNotifier *m_fifoNotifier;
    int m_fifoFd[2];
    bool m_fifoCanWrite;
    int m_bufferSize;
    int m_bufferOffset;
    char m_buffer[PIPE_BUF];

    QMediaPlayerResourceSetInterface *m_resources;
};

QT_END_NAMESPACE

#endif
