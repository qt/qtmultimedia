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

    bool m_tryingAsync;
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
