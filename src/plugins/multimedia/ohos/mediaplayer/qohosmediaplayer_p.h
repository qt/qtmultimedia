// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSMEDIAPLAYER_P_H
#define QOHOSMEDIAPLAYER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformmediaplayer_p.h>

#include <QtCore/qmutex.h>
#include <QtCore/qpointer.h>
#include <QtCore/qurl.h>
#include <QtCore/qfile.h>

#include <multimedia/player_framework/avplayer.h>
#include <multimedia/player_framework/avplayer_base.h>

#include <atomic>
#include <memory>

QT_BEGIN_NAMESPACE

class QAudioOutput;
class QVideoSink;
class QPlatformAudioOutput;
class QOhosVideoOutput;
class QOhosVideoSink;

class QOhosMediaPlayer : public QObject, public QPlatformMediaPlayer
{
    Q_OBJECT

public:
    explicit QOhosMediaPlayer(QMediaPlayer *parent);
    ~QOhosMediaPlayer() override;

    // QPlatformMediaPlayer interface
    qint64 duration() const override;
    qint64 position() const override;
    void setPosition(qint64 position) override;
    float bufferProgress() const override;
    bool isSeekable() const override;
    QMediaTimeRange availablePlaybackRanges() const override;
    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;
    QUrl media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QUrl &media, QIODevice *stream) override;
    void play() override;
    void pause() override;
    void stop() override;
    void setAudioOutput(QPlatformAudioOutput *) override;
    void setVideoSink(QVideoSink *sink) override;

    int trackCount(TrackType type) override;
    QMediaMetaData trackMetaData(TrackType type, int streamNumber) override;
    int activeTrack(TrackType type) override;
    QMediaMetaData metaData() const override;

private:
    // Called from Qt thread, dispatched from native callbacks.
    void handleStateChange(AVPlayerState newState);
    void handleEndOfStream();
    void handleSeekDone();
    void handleResolutionChange();
    void handleBufferingUpdate(int bufferingPercent);
    void handleError(int32_t errorCode, const QString &errorMsg);

    void releasePlayer();
    void clearSource();
    bool ensurePlayer();
    void applyVolume();

    void onVideoSurfaceReady();
    void applyPendingSource();

    // Native trampoline callbacks. Run on system threads — must marshal to Qt thread.
    static void onInfoTrampoline(OH_AVPlayer *player, AVPlayerOnInfoType type, OH_AVFormat *body,
                                 void *userData);
    static void onErrorTrampoline(OH_AVPlayer *player, int32_t errorCode, const char *errorMsg,
                                  void *userData);

    OH_AVPlayer *m_player{ nullptr };
    QUrl m_media;
    QPointer<QIODevice> m_stream;
    // Local-file sources require the QFile to outlive the AVPlayer because
    // OH_AVPlayer_SetFDSource only borrows the descriptor. Mirrors the gotcha
    // documented in the Qt 5 OHOS port.
    std::unique_ptr<QFileDevice> m_sourceFile;

    QPointer<QVideoSink> m_videoSink;
    QPlatformAudioOutput *m_audioOutput{ nullptr };
    std::unique_ptr<QOhosVideoOutput> m_videoOutput;
    bool m_videoSurfaceAttached{ false };
    bool m_pendingSetMedia{ false };

    std::atomic<qint64> m_position{ 0 };
    qint64 m_duration{ 0 };
    int m_videoWidth{ 0 };
    int m_videoHeight{ 0 };
    float m_bufferProgress{ 0.0f };
    bool m_hasVideoTrack{ false };
    bool m_hasAudioTrack{ false };
    qreal m_playbackRate{ 1.0 };
    bool m_seekable{ false };
    bool m_pendingPlay{ false };

    AVPlayerState m_avState{ AV_IDLE };
};

QT_END_NAMESPACE

#endif // QOHOSMEDIAPLAYER_P_H
