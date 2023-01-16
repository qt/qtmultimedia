// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMMEDIAPLAYER_H
#define QWASMMEDIAPLAYER_H

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
#include <qsize.h>
#include <qurl.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QWasmAudioOutput;
class QWasmVideoOutput;

class QWasmMediaPlayer : public QObject, public QPlatformMediaPlayer
{
    Q_OBJECT

public:
    explicit QWasmMediaPlayer(QMediaPlayer *parent = 0);
    ~QWasmMediaPlayer() override;

     enum QWasmMediaPlayerState {
         Error,
         Idle,
         Uninitialized,
         Preparing,
         Prepared,
         Started,
         Paused,
         Stopped,
         PlaybackCompleted
     };
    Q_ENUM(QWasmMediaPlayerState)

    enum QWasmMediaNetworkState { NetworkEmpty = 0, NetworkIdle, NetworkLoading, NetworkNoSource };
    Q_ENUM(QWasmMediaNetworkState)

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
    void setVideoSink(QVideoSink *surface) override;
    void setAudioOutput(QPlatformAudioOutput *output) override;
    void setPosition(qint64 position) override;
    void play() override;
    void pause() override;
    void stop() override;
    bool isSeekable() const override;
    int trackCount(TrackType trackType) override;

    void updateAudioDevice();

private Q_SLOTS:
    void volumeChanged(float volume);
    void mutedChanged(bool muted);
    void videoOutputReady(bool ready);
    void errorOccured(qint32 code, const QString &message);
    void bufferingChanged(qint32 percent);
    void videoSizeChanged(qint32 width, qint32 height);
    void mediaStateChanged(QWasmMediaPlayer::QWasmMediaPlayerState state);
    void setPositionChanged(qint64 position);
    void setDurationChanged(qint64 duration);
    void videoMetaDataChanged();

    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    void setMediaStatus(QMediaPlayer::MediaStatus status);
    void setAudioAvailable(bool available);
    void setVideoAvailable(bool available);
    void updateAvailablePlaybackRanges();
    void resetBufferingProgress();

    void setSubtitle(QString subtitle);
    void disableTrack(TrackType trackType);
    void initVideo();
    void initAudio();

    friend class StateChangeNotifier;

    QPointer<QWasmVideoOutput> m_videoOutput;
    QWasmAudioOutput *m_audioOutput = nullptr;

    QUrl m_mediaContent;
    QIODevice *m_mediaStream = nullptr;

    QVideoSink *m_videoSink = nullptr;
    int m_bufferPercent = -1;
    bool m_audioAvailable = false;
    bool m_videoAvailable = false;
    QSize m_videoSize;
    bool m_buffering = false;
    QMediaTimeRange m_availablePlaybackRange;
    int m_State;

    bool m_playWhenReady = false;

};

QT_END_NAMESPACE

#endif // QWASMMEDIAPLAYER_H
