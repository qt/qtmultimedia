/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QANDROIDMEDIAPLAYERCONTROL_H
#define QANDROIDMEDIAPLAYERCONTROL_H

#include <qglobal.h>
#include <QMediaPlayerControl>
#include <qsize.h>
#include <QtCore/QTemporaryFile>

QT_BEGIN_NAMESPACE

class AndroidMediaPlayer;
class QAndroidVideoOutput;

class QAndroidMediaPlayerControl : public QMediaPlayerControl
{
    Q_OBJECT
public:
    explicit QAndroidMediaPlayerControl(QObject *parent = 0);
    ~QAndroidMediaPlayerControl() Q_DECL_OVERRIDE;

    QMediaPlayer::State state() const Q_DECL_OVERRIDE;
    QMediaPlayer::MediaStatus mediaStatus() const Q_DECL_OVERRIDE;
    qint64 duration() const Q_DECL_OVERRIDE;
    qint64 position() const Q_DECL_OVERRIDE;
    int volume() const Q_DECL_OVERRIDE;
    bool isMuted() const Q_DECL_OVERRIDE;
    int bufferStatus() const Q_DECL_OVERRIDE;
    bool isAudioAvailable() const Q_DECL_OVERRIDE;
    bool isVideoAvailable() const Q_DECL_OVERRIDE;
    bool isSeekable() const Q_DECL_OVERRIDE;
    QMediaTimeRange availablePlaybackRanges() const Q_DECL_OVERRIDE;
    qreal playbackRate() const Q_DECL_OVERRIDE;
    void setPlaybackRate(qreal rate) Q_DECL_OVERRIDE;
    QMediaContent media() const Q_DECL_OVERRIDE;
    const QIODevice *mediaStream() const Q_DECL_OVERRIDE;
    void setMedia(const QMediaContent &mediaContent, QIODevice *stream) Q_DECL_OVERRIDE;

    void setVideoOutput(QObject *videoOutput);

Q_SIGNALS:
    void metaDataUpdated();

public Q_SLOTS:
    void setPosition(qint64 position) Q_DECL_OVERRIDE;
    void play() Q_DECL_OVERRIDE;
    void pause() Q_DECL_OVERRIDE;
    void stop() Q_DECL_OVERRIDE;
    void setVolume(int volume) Q_DECL_OVERRIDE;
    void setMuted(bool muted) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onVideoOutputReady(bool ready);
    void onError(qint32 what, qint32 extra);
    void onInfo(qint32 what, qint32 extra);
    void onBufferingChanged(qint32 percent);
    void onVideoSizeChanged(qint32 width, qint32 height);
    void onStateChanged(qint32 state);

private:
    AndroidMediaPlayer *mMediaPlayer;
    QMediaPlayer::State mCurrentState;
    QMediaPlayer::MediaStatus mCurrentMediaStatus;
    QMediaContent mMediaContent;
    QIODevice *mMediaStream;
    QAndroidVideoOutput *mVideoOutput;
    bool mSeekable;
    int mBufferPercent;
    bool mBufferFilled;
    bool mAudioAvailable;
    bool mVideoAvailable;
    QSize mVideoSize;
    bool mBuffering;
    QMediaTimeRange mAvailablePlaybackRange;
    int mState;
    int mPendingState;
    qint64 mPendingPosition;
    bool mPendingSetMedia;
    int mPendingVolume;
    int mPendingMute;
    QScopedPointer<QTemporaryFile> mTempFile;

    void setState(QMediaPlayer::State state);
    void setMediaStatus(QMediaPlayer::MediaStatus status);
    void setSeekable(bool seekable);
    void setAudioAvailable(bool available);
    void setVideoAvailable(bool available);
    void updateAvailablePlaybackRanges();
    void resetBufferingProgress();
    void flushPendingStates();
    void updateBufferStatus();
};

QT_END_NAMESPACE

#endif // QANDROIDMEDIAPLAYERCONTROL_H
