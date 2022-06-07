// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQnxMediaPlayer_H
#define QQnxMediaPlayer_H

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

#include "qqnxmediametadata_p.h"
#include "mmrenderertypes.h"

#include <private/qplatformmediaplayer_p.h>
#include <QtCore/qabstractnativeeventfilter.h>
#include <QtCore/qpointer.h>
#include <QtCore/qtimer.h>

#include <mm/renderer.h>
#include <mm/renderer/types.h>

#include <optional>

QT_BEGIN_NAMESPACE

class QQnxVideoSink;
class QQnxMediaEventThread;
class QQnxWindowGrabber;

class QQnxMediaPlayer : public QObject
                      , public QPlatformMediaPlayer
{
    Q_OBJECT
public:
    explicit QQnxMediaPlayer(QMediaPlayer *parent = nullptr);
    ~QQnxMediaPlayer();

    qint64 duration() const override;

    qint64 position() const override;
    void setPosition(qint64 position) override;

    void setAudioOutput(QPlatformAudioOutput *) override;

    float bufferProgress() const override;

    bool isAudioAvailable() const override;
    bool isVideoAvailable() const override;

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

    void setVideoSink(QVideoSink *videoSink);

private Q_SLOTS:
    void setVolume(float volume);
    void setMuted(bool muted);
    void readEvents();

private:
    void startMonitoring();
    void stopMonitoring();
    void resetMonitoring();

    void openConnection();
    void emitMmError(const char *msg);
    void emitMmError(const QString &msg);
    void emitPError(const QString &msg);

    void handleMmPositionChanged(qint64 newPosition);
    void updateBufferLevel(int level, int capacity);
    void updateMetaData(const strm_dict_t *dict);

    void handleMmEventState(const mmr_event_t *event);
    void handleMmEventStatus(const mmr_event_t *event);
    void handleMmEventStatusData(const strm_dict_t *data);
    void handleMmEventError(const mmr_event_t *event);

    QByteArray resourcePathForUrl(const QUrl &url);

    void closeConnection();
    void attach();

    bool attachVideoOutput();
    bool attachAudioOutput();
    bool attachInput();

    void detach();
    void detachVideoOutput();
    void detachAudioOutput();
    void detachInput();

    bool isVideoOutputAttached() const;
    bool isAudioOutputAttached() const;
    bool isInputAttached() const;

    void updateScene(const QSize &size);

    void updateVolume();

    void setPositionInternal(qint64 position);
    void flushPosition();

    bool isPendingPositionFlush() const;

    void setDeferredSpeedEnabled(bool enabled);
    bool isDeferredSpeedEnabled() const;

    mmr_context_t *m_context = nullptr;
    mmr_connection_t *m_connection = nullptr;

    QString m_contextName;

    int m_id = -1;
    int m_audioId = -1;
    int m_volume = 50; // range is 0-100

    QUrl m_media;
    QPointer<QAudioOutput> m_audioOutput;
    QPointer<QQnxVideoSink> m_platformVideoSink;

    QQnxMediaMetaData m_metaData;

    qint64 m_position = 0;
    qint64 m_pendingPosition = 0;

    int m_bufferLevel = 0;

    int m_videoId = -1;

    QTimer m_flushPositionTimer;

    QQnxMediaEventThread *m_eventThread = nullptr;

    int m_speed = 1000;
    int m_configuredSpeed = 1000;

    std::optional<int> m_deferredSpeed;

    QQnxWindowGrabber* m_windowGrabber = nullptr;

    bool m_inputAttached = false;
    bool m_muted = false;
    bool m_deferredSpeedEnabled = false;
};

QT_END_NAMESPACE

#endif
