// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDMEDIAPLAYERCONTROL_H
#define QANDROIDMEDIAPLAYERCONTROL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>
#include <private/qplatformmediaplayer_p.h>
#include <qandroidmetadata_p.h>
#include <qmap.h>
#include <qsize.h>
#include <qurl.h>

QT_BEGIN_NAMESPACE

class AndroidMediaPlayer;
class QAndroidTextureVideoOutput;
class QAndroidMediaPlayerVideoRendererControl;
class QAndroidAudioOutput;

class QAndroidMediaPlayer : public QObject, public QPlatformMediaPlayer
{
    Q_OBJECT

public:
    explicit QAndroidMediaPlayer(QMediaPlayer *parent = 0);
    ~QAndroidMediaPlayer() override;

    qint64 duration() const override;
    qint64 position() const override;
    float bufferProgress() const override;
    bool isAudioAvailable() const override;
    bool isVideoAvailable() const override;
    QMediaTimeRange availablePlaybackRanges() const override;
    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;
    QUrl media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QUrl &mediaContent, QIODevice *stream) override;

    QMediaMetaData metaData() const override;

    void setVideoSink(QVideoSink *surface) override;

    void setAudioOutput(QPlatformAudioOutput *output) override;
    void updateAudioDevice();

    void setPosition(qint64 position) override;
    void play() override;
    void pause() override;
    void stop() override;

    int trackCount(TrackType trackType) override;
    QMediaMetaData trackMetaData(TrackType trackType, int streamNumber) override;
    int activeTrack(TrackType trackType) override;
    void setActiveTrack(TrackType trackType, int streamNumber) override;

private Q_SLOTS:
    void setVolume(float volume);
    void setMuted(bool muted);
    void onVideoOutputReady(bool ready);
    void onError(qint32 what, qint32 extra);
    void onInfo(qint32 what, qint32 extra);
    void onBufferingChanged(qint32 percent);
    void onVideoSizeChanged(qint32 width, qint32 height);
    void onStateChanged(qint32 state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);

private:
    AndroidMediaPlayer *mMediaPlayer = nullptr;
    QAndroidAudioOutput *m_audioOutput = nullptr;
    QUrl mMediaContent;
    QIODevice *mMediaStream = nullptr;
    QAndroidTextureVideoOutput *mVideoOutput = nullptr;
    QVideoSink *m_videoSink = nullptr;
    int mBufferPercent = -1;
    bool mBufferFilled = false;
    bool mAudioAvailable = false;
    bool mVideoAvailable = false;
    QSize mVideoSize;
    bool mBuffering = false;
    QMediaTimeRange mAvailablePlaybackRange;
    int mState;
    int mPendingState = -1;
    qint64 mPendingPosition = -1;
    bool mPendingSetMedia = false;
    float mPendingVolume = -1;
    int mPendingMute = -1;
    bool mReloadingMedia = false;
    int mActiveStateChangeNotifiers = 0;
    qreal mCurrentPlaybackRate = 1.;
    bool mHasPendingPlaybackRate = false; // we need this because the rate can theoretically be negative
    QMap<TrackType, QList<QAndroidMetaData>> mTracksMetadata;

    bool mIsVideoTrackEnabled = true;
    bool mIsAudioTrackEnabled = true;

    void setMediaStatus(QMediaPlayer::MediaStatus status);
    void setAudioAvailable(bool available);
    void setVideoAvailable(bool available);
    void updateAvailablePlaybackRanges();
    void resetBufferingProgress();
    void flushPendingStates();
    void updateBufferStatus();
    void updateTrackInfo();
    void setSubtitle(QString subtitle);
    void disableTrack(TrackType trackType);

    int convertTrackNumber(int androidTrackNumber);
    friend class StateChangeNotifier;
};

QT_END_NAMESPACE

#endif // QANDROIDMEDIAPLAYERCONTROL_H
