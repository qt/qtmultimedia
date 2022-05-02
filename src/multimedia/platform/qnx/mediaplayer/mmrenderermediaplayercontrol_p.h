/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
******************************************************************************/
#ifndef MMRENDERERMEDIAPLAYERCONTROL_H
#define MMRENDERERMEDIAPLAYERCONTROL_H

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

#include "mmrenderermetadata_p.h"
#include <private/qplatformmediaplayer_p.h>
#include <QtCore/qabstractnativeeventfilter.h>
#include <QtCore/qpointer.h>
#include <QtCore/qtimer.h>

typedef struct mmr_connection mmr_connection_t;
typedef struct mmr_context mmr_context_t;
typedef struct mmrenderer_monitor mmrenderer_monitor_t;
typedef struct strm_dict strm_dict_t;

QT_BEGIN_NAMESPACE

class MmRendererAudioRoleControl;
class MmRendererPlayerVideoRendererControl;
class MmRendererVideoWindowControl;

class MmRendererMediaPlayerControl : public QPlatformMediaPlayer, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit MmRendererMediaPlayerControl(QMediaPlayer *parent = 0);

    QMediaPlayer::State state() const override;

    QMediaPlayer::MediaStatus mediaStatus() const override;

    qint64 duration() const override;

    qint64 position() const override;
    void setPosition(qint64 position) override;

    int volume() const override;
    void setVolume(int volume) override;

    bool isMuted() const override;
    void setMuted(bool muted) override;

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

    MmRendererPlayerVideoRendererControl *videoRendererControl() const;
    void setVideoRendererControl(MmRendererPlayerVideoRendererControl *videoControl);

    MmRendererVideoWindowControl *videoWindowControl() const;
    void setVideoWindowControl(MmRendererVideoWindowControl *videoControl);

protected:
    virtual void startMonitoring() = 0;
    virtual void stopMonitoring() = 0;
    virtual void resetMonitoring() = 0;

    void openConnection();
    void emitMmError(const QString &msg);
    void emitPError(const QString &msg);
    void setMmPosition(qint64 newPosition);
    void setMmBufferStatus(const QString &bufferProgress);
    void setMmBufferLevel(int level, int capacity);
    void handleMmStopped();
    void handleMmSuspend(const QString &reason);
    void handleMmSuspendRemoval(const QString &bufferProgress);
    void handleMmPause();
    void handleMmPlay();
    void updateMetaData(const strm_dict_t *dict);

    // must be called from subclass dtors (calls virtual function stopMonitoring())
    void destroy();

    mmr_context_t *m_context;
    int m_id;
    QString m_contextName;

private Q_SLOTS:
    void continueLoadMedia();

private:
    QByteArray resourcePathForUrl(const QUrl &url);
    void closeConnection();
    void attach();
    void detach();

    // All these set the specified value to the backend, but neither emit changed signals
    // nor change the member value.
    void setVolumeInternal(int newVolume);
    void setPlaybackRateInternal(qreal rate);
    void setPositionInternal(qint64 position);

    void setMediaStatus(QMediaPlayer::MediaStatus status);
    void setState(QMediaPlayer::State state);

    enum StopCommand { StopMmRenderer, IgnoreMmRenderer };
    void stopInternal(StopCommand stopCommand);

    QUrl m_media;
    mmr_connection_t *m_connection;
    int m_audioId;
    QMediaPlayer::State m_state;
    int m_volume;
    bool m_muted;
    qreal m_rate;
    QPointer<MmRendererPlayerVideoRendererControl> m_videoRendererControl;
    QPointer<MmRendererVideoWindowControl> m_videoWindowControl;
    MmRendererMetaData m_metaData;
    qint64 m_position;
    QMediaPlayer::MediaStatus m_mediaStatus;
    bool m_playAfterMediaLoaded;
    bool m_inputAttached;
    int m_bufferLevel;
    QTimer m_loadingTimer;
};

QT_END_NAMESPACE

#endif
