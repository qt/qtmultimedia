/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#include <private/qplatformmediaplayer_p.h>
#include <private/qqnxmediametadata_p.h>
#include <QtCore/qabstractnativeeventfilter.h>
#include <QtCore/qpointer.h>
#include <QtCore/qtimer.h>

typedef struct mmr_connection mmr_connection_t;
typedef struct mmr_context mmr_context_t;
typedef struct mmrenderer_monitor mmrenderer_monitor_t;
typedef struct strm_dict strm_dict_t;
typedef struct strm_string strm_string_t;

#include <mm/renderer/types.h>

// ### replace with proper include: mm/renderer/events.h
typedef enum mmr_state {
    MMR_STATE_DESTROYED,
    MMR_STATE_IDLE,
    MMR_STATE_STOPPED,
    MMR_STATE_PLAYING
} mmr_state_t;
typedef enum mmr_event_type {
    MMR_EVENT_NONE,
    MMR_EVENT_ERROR,
    MMR_EVENT_STATE,
    MMR_EVENT_OVERFLOW,
    MMR_EVENT_WARNING,
    MMR_EVENT_STATUS,
    MMR_EVENT_METADATA,
    MMR_EVENT_PLAYLIST,
    MMR_EVENT_INPUT,
    MMR_EVENT_OUTPUT,
    MMR_EVENT_CTXTPAR,
    MMR_EVENT_TRKPAR,
    MMR_EVENT_OTHER
} mmr_event_type_t;
typedef struct mmr_event {
    mmr_event_type_t type;
    mmr_state_t state;
    int speed;
    union mmr_event_details {

        struct mmr_event_state {
            mmr_state_t oldstate;
            int oldspeed;
        } state;

        struct mmr_event_error {
            mmr_error_info_t info;
        } error;

        struct mmr_event_warning {
            const char *str;
            const strm_string_t *obj;
        } warning;

        struct mmr_event_metadata {
            unsigned index;
        } metadata;

        struct mmr_event_trkparam {
            unsigned index;
        } trkparam;

        struct mmr_event_playlist {
            unsigned start;
            unsigned end;
            unsigned length;
        } playlist;

        struct mmr_event_output {
            unsigned id;
        } output;
    } details;
    const strm_string_t* pos_obj;
    const char* pos_str;
    const strm_dict_t* data;
    const char* objname;
    void* usrdata;
} mmr_event_t;
const mmr_event_t* mmr_event_get(mmr_context_t *ctxt);

QT_BEGIN_NAMESPACE

class QQnxVideoSink;
class QQnxMediaEventThread;

class QQnxMediaPlayer : public QObject, public QPlatformMediaPlayer, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit QQnxMediaPlayer(QMediaPlayer *parent = 0);
    ~QQnxMediaPlayer();

    QMediaPlayer::MediaStatus mediaStatus() const override;

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

    QQnxVideoSink *videoRendererControl() const;
    void setVideoRendererControl(QQnxVideoSink *videoControl);

protected:
    void startMonitoring();
    void stopMonitoring();
    void resetMonitoring();

    void setState(QMediaPlayer::PlaybackState state);

    void openConnection();
    void emitMmError(const char *msg);
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

    mmr_context_t *m_context;
    int m_id;
    QString m_contextName;

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private Q_SLOTS:
    void continueLoadMedia();
    void setVolume(float volume);
    void setMuted(bool muted);
    void readEvents();

private:
    QByteArray resourcePathForUrl(const QUrl &url);
    void closeConnection();
    void attach();
    void detach();

    // All these set the specified value to the backend, but neither emit changed signals
    // nor change the member value.
    void setVolumeInternal(float newVolume);
    void setPlaybackRateInternal(qreal rate);
    void setPositionInternal(qint64 position);

    void setMediaStatus(QMediaPlayer::MediaStatus status);

    enum StopCommand { StopMmRenderer, IgnoreMmRenderer };
    void stopInternal(StopCommand stopCommand);

    QUrl m_media;
    mmr_connection_t *m_connection = nullptr;
    int m_audioId;
    float m_volume = 1.;
    bool m_muted = true;
    qreal m_rate = 1.;
    QPointer<QAudioOutput> m_audioOutput;
    QPointer<QQnxVideoSink> m_videoRenderer;
    MmRendererMetaData m_metaData;
    qint64 m_position = 0;
    QMediaPlayer::MediaStatus m_mediaStatus = QMediaPlayer::NoMedia;
    bool m_playAfterMediaLoaded = false;
    bool m_inputAttached = false;
    int m_bufferLevel = 0;
    QTimer m_loadingTimer;


    QQnxMediaEventThread *m_eventThread = nullptr;

    // status properties.
    QByteArray m_bufferProgress;
    int m_bufferCapacity = 0;
    bool m_suspended = false;
    QByteArray m_suspendedReason;

    // state properties.
    mmr_state_t m_state = MMR_STATE_IDLE;
    int m_speed = 0;
};

QT_END_NAMESPACE

#endif
