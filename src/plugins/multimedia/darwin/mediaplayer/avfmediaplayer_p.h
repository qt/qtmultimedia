// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFMEDIAPLAYER_H
#define AVFMEDIAPLAYER_H

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

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QSet>
#include <QtCore/QResource>
#include <QtCore/QUrl>
#include <QtCore/QTimer>

#include <private/qplatformmediaplayer_p.h>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QVideoFrame>

Q_FORWARD_DECLARE_OBJC_CLASS(AVAsset);
Q_FORWARD_DECLARE_OBJC_CLASS(AVPlayerItemTrack);
Q_FORWARD_DECLARE_OBJC_CLASS(AVFMediaPlayerObserver);
Q_FORWARD_DECLARE_OBJC_CLASS(AVAssetTrack);

QT_BEGIN_NAMESPACE

class AVFMediaPlayer;
class AVFVideoRendererControl;
class AVFVideoSink;

class AVFMediaPlayer : public QObject, public QPlatformMediaPlayer
{
    Q_OBJECT
public:
    AVFMediaPlayer(QMediaPlayer *parent);
    virtual ~AVFMediaPlayer();

    void setVideoSink(QVideoSink *sink) override;
    void setVideoOutput(AVFVideoRendererControl *output);
    AVAsset *currentAssetHandle();

    QMediaPlayer::PlaybackState state() const override;
    QMediaPlayer::MediaStatus mediaStatus() const override;

    QUrl media() const override;
    QIODevice *mediaStream() const override;
    void setMedia(const QUrl &content, QIODevice *stream) override;

    qint64 position() const override;
    qint64 duration() const override;

    float bufferProgress() const override;

    bool isAudioAvailable() const override;
    bool isVideoAvailable() const override;

    bool isSeekable() const override;
    QMediaTimeRange availablePlaybackRanges() const override;

    qreal playbackRate() const override;

    void setAudioOutput(QPlatformAudioOutput *output) override;
    QPlatformAudioOutput *m_audioOutput = nullptr;

    QMediaMetaData metaData() const override;

    static void videoOrientationForAssetTrack(AVAssetTrack *track,
                                       QVideoFrame::RotationAngle &angle,
                                       bool &mirrored);

public Q_SLOTS:
    void setPlaybackRate(qreal rate) override;
    void nativeSizeChanged(QSize size);

    void setPosition(qint64 pos) override;

    void play() override;
    void pause() override;
    void stop() override;

    void setVolume(float volume);
    void setMuted(bool muted);
    void audioOutputChanged();

    void processEOS();
    void processLoadStateChange(QMediaPlayer::PlaybackState newState);
    void processPositionChange();
    void processMediaLoadError();

    void processLoadStateChange();
    void processLoadStateFailure();

    void processBufferStateChange(int bufferProgress);

    void processDurationChange(qint64 duration);

    void streamReady();
    void streamDestroyed();
    void updateTracks();
    void setActiveTrack(QPlatformMediaPlayer::TrackType type, int index) override;
    int activeTrack(QPlatformMediaPlayer::TrackType type) override;
    int trackCount(TrackType) override;
    QMediaMetaData trackMetaData(TrackType type, int trackNumber) override;

public:
    QList<QMediaMetaData> tracks[QPlatformMediaPlayer::NTrackTypes];
    QList<AVPlayerItemTrack *> nativeTracks[QPlatformMediaPlayer::NTrackTypes];

private:
    void setAudioAvailable(bool available);
    void setVideoAvailable(bool available);
    void setSeekable(bool seekable);
    void resetStream(QIODevice *stream = nullptr);

    void orientationChanged(QVideoFrame::RotationAngle rotation, bool mirrored);

    AVFVideoRendererControl *m_videoOutput = nullptr;
    AVFVideoSink *m_videoSink = nullptr;

    QMediaPlayer::PlaybackState m_state;
    QMediaPlayer::MediaStatus m_mediaStatus;
    QIODevice *m_mediaStream;
    QUrl m_resources;
    QMediaMetaData m_metaData;

    qreal m_rate;
    qint64 m_requestedPosition;

    qint64 m_duration;
    int m_bufferProgress;
    bool m_videoAvailable;
    bool m_audioAvailable;
    bool m_seekable;

    AVFMediaPlayerObserver *m_observer;

    QTimer m_playbackTimer;
};

QT_END_NAMESPACE

#endif // AVFMEDIAPLAYER_H
