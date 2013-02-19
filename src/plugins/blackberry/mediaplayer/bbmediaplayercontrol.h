/****************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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

class BbMetaDataReaderControl;
class BbPlayerVideoRendererControl;
class BbVideoWindowControl;

class BbMediaPlayerControl : public QMediaPlayerControl, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit BbMediaPlayerControl(QObject *parent = 0);
    ~BbMediaPlayerControl();

    QMediaPlayer::State state() const Q_DECL_OVERRIDE;

    QMediaPlayer::MediaStatus mediaStatus() const Q_DECL_OVERRIDE;

    qint64 duration() const Q_DECL_OVERRIDE;

    qint64 position() const Q_DECL_OVERRIDE;
    void setPosition(qint64 position) Q_DECL_OVERRIDE;

    int volume() const Q_DECL_OVERRIDE;
    void setVolume(int volume) Q_DECL_OVERRIDE;

    bool isMuted() const Q_DECL_OVERRIDE;
    void setMuted(bool muted) Q_DECL_OVERRIDE;

    int bufferStatus() const Q_DECL_OVERRIDE;

    bool isAudioAvailable() const Q_DECL_OVERRIDE;
    bool isVideoAvailable() const Q_DECL_OVERRIDE;

    bool isSeekable() const Q_DECL_OVERRIDE;

    QMediaTimeRange availablePlaybackRanges() const Q_DECL_OVERRIDE;

    qreal playbackRate() const Q_DECL_OVERRIDE;
    void setPlaybackRate(qreal rate) Q_DECL_OVERRIDE;

    QMediaContent media() const Q_DECL_OVERRIDE;
    const QIODevice *mediaStream() const Q_DECL_OVERRIDE;
    void setMedia(const QMediaContent &media, QIODevice *stream) Q_DECL_OVERRIDE;

    void play() Q_DECL_OVERRIDE;
    void pause() Q_DECL_OVERRIDE;
    void stop() Q_DECL_OVERRIDE;

    void setVideoRendererControl(BbPlayerVideoRendererControl *videoControl);
    void setVideoWindowControl(BbVideoWindowControl *videoControl);
    void setMetaDataReaderControl(BbMetaDataReaderControl *metaDataReaderControl);
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
    QPointer<BbPlayerVideoRendererControl> m_videoRendererControl;
    QPointer<BbVideoWindowControl> m_videoWindowControl;
    QPointer<BbMetaDataReaderControl> m_metaDataReaderControl;
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
