// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDMEDIAPLAYER_H
#define ANDROIDMEDIAPLAYER_H

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

#include <QObject>
#include <QNetworkRequest>
#include <QtCore/qjniobject.h>
#include <QAudio>

QT_BEGIN_NAMESPACE

class AndroidSurfaceTexture;

class AndroidMediaPlayer : public QObject
{
    Q_OBJECT
public:
    AndroidMediaPlayer();
    ~AndroidMediaPlayer();

    enum MediaError
    {
        // What
        MEDIA_ERROR_UNKNOWN = 1,
        MEDIA_ERROR_SERVER_DIED = 100,
        MEDIA_ERROR_INVALID_STATE = -38, // Undocumented
        // Extra
        MEDIA_ERROR_IO = -1004,
        MEDIA_ERROR_MALFORMED = -1007,
        MEDIA_ERROR_UNSUPPORTED = -1010,
        MEDIA_ERROR_TIMED_OUT = -110,
        MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,
        MEDIA_ERROR_BAD_THINGS_ARE_GOING_TO_HAPPEN = -2147483648 // Undocumented
    };

    enum MediaInfo
    {
        MEDIA_INFO_UNKNOWN = 1,
        MEDIA_INFO_VIDEO_TRACK_LAGGING = 700,
        MEDIA_INFO_VIDEO_RENDERING_START = 3,
        MEDIA_INFO_BUFFERING_START = 701,
        MEDIA_INFO_BUFFERING_END = 702,
        MEDIA_INFO_BAD_INTERLEAVING = 800,
        MEDIA_INFO_NOT_SEEKABLE = 801,
        MEDIA_INFO_METADATA_UPDATE = 802
    };

    enum MediaPlayerState {
        Uninitialized = 0x1, /* End */
        Idle = 0x2,
        Preparing = 0x4,
        Prepared = 0x8,
        Initialized = 0x10,
        Started = 0x20,
        Stopped = 0x40,
        Paused = 0x80,
        PlaybackCompleted = 0x100,
        Error = 0x200
    };

    enum TrackType { Unknown = 0, Video, Audio, TimedText, Subtitle, Metadata };

    struct TrackInfo
    {
        int trackNumber;
        TrackType trackType;
        QString language;
        QString mimeType;
    };

    void release();
    void reset();

    int getCurrentPosition();
    int getDuration();
    bool isPlaying();
    int volume();
    bool isMuted();
    qreal playbackRate();
    jobject display();

    void play();
    void pause();
    void stop();
    void seekTo(qint32 msec);
    void setMuted(bool mute);
    void setDataSource(const QNetworkRequest &request);
    void prepareAsync();
    void setVolume(int volume);
    static void startSoundStreaming(const int inputId, const int outputId);
    static void stopSoundStreaming();
    bool setPlaybackRate(qreal rate);
    void setDisplay(AndroidSurfaceTexture *surfaceTexture);
    static bool setAudioOutput(const QByteArray &deviceId);
    QList<TrackInfo> tracksInfo();
    int activeTrack(TrackType trackType);
    void deselectTrack(int trackNumber);
    void selectTrack(int trackNumber);

    static bool registerNativeMethods();

    void blockAudio();
    void unblockAudio();
Q_SIGNALS:
    void error(qint32 what, qint32 extra);
    void bufferingChanged(qint32 percent);
    void durationChanged(qint64 duration);
    void progressChanged(qint64 progress);
    void stateChanged(qint32 state);
    void info(qint32 what, qint32 extra);
    void videoSizeChanged(qint32 width, qint32 height);
    void timedTextChanged(QString text);
    void tracksInfoChanged();

private:
    QJniObject mMediaPlayer;
    bool mAudioBlocked = false;
};

QT_END_NAMESPACE

#endif // ANDROIDMEDIAPLAYER_H
