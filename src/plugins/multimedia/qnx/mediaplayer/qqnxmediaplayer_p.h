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
