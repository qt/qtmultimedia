/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QANDROIDMEDIAPLAYERCONTROL_H
#define QANDROIDMEDIAPLAYERCONTROL_H

#include <qglobal.h>
#include <QMediaPlayerControl>
#include <qsize.h>

QT_BEGIN_NAMESPACE

class AndroidMediaPlayer;
class QAndroidVideoOutput;

class QAndroidMediaPlayerControl : public QMediaPlayerControl
{
    Q_OBJECT
public:
    explicit QAndroidMediaPlayerControl(QObject *parent = 0);
    ~QAndroidMediaPlayerControl() override;

    QMediaPlayer::State state() const override;
    QMediaPlayer::MediaStatus mediaStatus() const override;
    qint64 duration() const override;
    qint64 position() const override;
    int volume() const override;
    bool isMuted() const override;
    int bufferStatus() const override;
    bool isAudioAvailable() const override;
    bool isVideoAvailable() const override;
    bool isSeekable() const override;
    QMediaTimeRange availablePlaybackRanges() const override;
    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;
    QMediaContent media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QMediaContent &mediaContent, QIODevice *stream) override;

    void setVideoOutput(QAndroidVideoOutput *videoOutput);

Q_SIGNALS:
    void metaDataUpdated();

public Q_SLOTS:
    void setPosition(qint64 position) override;
    void play() override;
    void pause() override;
    void stop() override;
    void setVolume(int volume) override;
    void setMuted(bool muted) override;
    void setAudioRole(QAudio::Role role);
    void setCustomAudioRole(const QString &role);

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
    bool mReloadingMedia;
    int mActiveStateChangeNotifiers;
    qreal mPendingPlaybackRate;
    bool mHasPendingPlaybackRate; // we need this because the rate can theoretically be negative

    void setState(QMediaPlayer::State state);
    void setMediaStatus(QMediaPlayer::MediaStatus status);
    void setSeekable(bool seekable);
    void setAudioAvailable(bool available);
    void setVideoAvailable(bool available);
    void updateAvailablePlaybackRanges();
    void resetBufferingProgress();
    void flushPendingStates();
    void updateBufferStatus();

    friend class StateChangeNotifier;
};

QT_END_NAMESPACE

#endif // QANDROIDMEDIAPLAYERCONTROL_H
