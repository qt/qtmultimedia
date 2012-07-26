/****************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef BBMEDIAPLAYERCONTROL_H
#define BBMEDIAPLAYERCONTROL_H

#include "bbmetadata.h"
#include <qmediaplayercontrol.h>
#include <QtCore/qabstractnativeeventfilter.h>
#include <QtCore/qpointer.h>
#include <QtCore/qtimer.h>

struct bps_event_t;
typedef struct mmr_connection mmr_connection_t;
typedef struct mmr_context mmr_context_t;
typedef struct mmrenderer_monitor mmrenderer_monitor_t;

QT_BEGIN_NAMESPACE

class BbVideoWindowControl;

class BbMediaPlayerControl : public QMediaPlayerControl, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit BbMediaPlayerControl(QObject *parent = 0);
    ~BbMediaPlayerControl();

    QMediaPlayer::State state() const;

    QMediaPlayer::MediaStatus mediaStatus() const;

    qint64 duration() const;

    qint64 position() const;
    void setPosition(qint64 position);

    int volume() const;
    void setVolume(int volume);

    bool isMuted() const;
    void setMuted(bool muted);

    int bufferStatus() const;

    bool isAudioAvailable() const;
    bool isVideoAvailable() const;

    bool isSeekable() const;

    QMediaTimeRange availablePlaybackRanges() const;

    qreal playbackRate() const;
    void setPlaybackRate(qreal rate);

    QMediaContent media() const;
    const QIODevice *mediaStream() const;
    void setMedia(const QMediaContent &media, QIODevice *stream);

    void play();
    void pause();
    void stop();

    void setVideoControl(BbVideoWindowControl *videoControl);
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void continueLoadMedia();

private:
    QString resourcePathForUrl(const QUrl &url);
    void openConnection();
    void closeConnection();
    void attach();
    void detach();
    void updateMetaData();

    void emitMmError(const QString &msg);
    void emitPError(const QString &msg);

    // All these set the specified value to the backend, but neither emit changed signals
    // nor change the member value.
    void setVolumeInternal(int newVolume);
    void setPlaybackRateInternal(qreal rate);
    void setPositionInternal(qint64 position);

    void setMediaStatus(QMediaPlayer::MediaStatus status);
    void setState(QMediaPlayer::State state);

    enum StopCommand { StopMmRenderer, IgnoreMmRenderer };
    void stopInternal(StopCommand stopCommand);

    QMediaContent m_media;
    mmr_connection_t *m_connection;
    mmr_context_t *m_context;
    QString m_contextName;
    int m_audioId;
    QMediaPlayer::State m_state;
    int m_volume;
    bool m_muted;
    qreal m_rate;
    QPointer<BbVideoWindowControl> m_videoControl;
    BbMetaData m_metaData;
    int m_id;
    mmrenderer_monitor_t *m_eventMonitor;
    qint64 m_position;
    QMediaPlayer::MediaStatus m_mediaStatus;
    bool m_playAfterMediaLoaded;
    bool m_inputAttached;
    int m_stopEventsToIgnore;
    int m_bufferStatus;
    QString m_tempMediaFileName;
    QTimer m_loadingTimer;
};

QT_END_NAMESPACE

#endif
