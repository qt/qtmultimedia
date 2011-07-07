/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEMEDIABASE_P_H
#define QDECLARATIVEMEDIABASE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbasictimer.h>
#include <qmediaplayer.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QMediaPlayerControl;
class QMediaService;
class QMediaServiceProvider;
class QMetaDataReaderControl;
class QDeclarativeMediaBaseAnimation;
class QDeclarativeMediaMetaData;

class QDeclarativeMediaBase
{
public:
    enum Loop {
        INFINITE = -1
    };

    QDeclarativeMediaBase();
    virtual ~QDeclarativeMediaBase();

    QUrl source() const;
    void setSource(const QUrl &url);

    bool isAutoLoad() const;
    void setAutoLoad(bool autoLoad);

    int loopCount() const;
    void setLoopCount(int loopCount);

    bool isPlaying() const;
    void setPlaying(bool playing);

    bool isPaused() const;
    void setPaused(bool paused);

    int duration() const;

    int position() const;
    void setPosition(int position);

    qreal volume() const;
    void setVolume(qreal volume);

    bool isMuted() const;
    void setMuted(bool muted);

    qreal bufferProgress() const;

    bool isSeekable() const;

    qreal playbackRate() const;
    void setPlaybackRate(qreal rate);

    QString errorString() const;

    QDeclarativeMediaMetaData *metaData() const;

    void _q_statusChanged();

    void _q_metaDataChanged();

    void componentComplete();

protected:
    void shutdown();

    void setObject(QObject *object);

    virtual void sourceChanged() = 0;
    virtual void autoLoadChanged() = 0;
    virtual void playingChanged() = 0;
    virtual void pausedChanged() = 0;
    virtual void loopCountChanged() = 0;

    virtual void started() = 0;
    virtual void resumed() = 0;
    virtual void paused() = 0;
    virtual void stopped() = 0;

    virtual void statusChanged() = 0;

    virtual void durationChanged() = 0;
    virtual void positionChanged() = 0;

    virtual void volumeChanged() = 0;
    virtual void mutedChanged() = 0;

    virtual void bufferProgressChanged() = 0;

    virtual void seekableChanged() = 0;
    virtual void playbackRateChanged() = 0;

    virtual void errorChanged() = 0;

    bool m_paused;
    bool m_playing;
    bool m_autoLoad;
    bool m_loaded;
    bool m_muted;
    bool m_complete;
    int m_loopCount;
    int m_runningCount;
    int m_position;
    qreal m_vol;
    qreal m_playbackRate;
    QMediaService *m_mediaService;
    QMediaPlayerControl *m_playerControl;

    QObject *m_qmlObject;
    QMediaObject *m_mediaObject;
    QMediaServiceProvider *m_mediaProvider;
    QMetaDataReaderControl *m_metaDataControl;
    QDeclarativeMediaBaseAnimation *m_animation;
    QScopedPointer<QDeclarativeMediaMetaData> m_metaData;

    QMediaPlayer::MediaStatus m_status;
    QMediaPlayer::Error m_error;
    QString m_errorString;
    QUrl m_source;

    friend class QDeclarativeMediaBaseAnimation;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
