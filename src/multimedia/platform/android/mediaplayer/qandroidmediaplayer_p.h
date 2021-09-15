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
#include <private/qandroidmetadata_p.h>
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

    bool isSeekable() const override;

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
    qreal mPendingPlaybackRate = 1.;
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
