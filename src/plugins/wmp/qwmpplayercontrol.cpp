/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwmpplayercontrol.h"

#include "qwmpevents.h"
#include "qwmpglobal.h"
#include "qwmpmetadata.h"
#include "qwmpplaylist.h"

#include <qmediaplayer.h>
#include <qmediaplaylist.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>

QWmpPlayerControl::QWmpPlayerControl(IWMPCore3 *player, QWmpEvents *events, QObject *parent)
    : QMediaPlayerControl(parent)
    , m_player(player)
    , m_controls(0)
    , m_settings(0)
    , m_state(QMediaPlayer::StoppedState)
    , m_changes(0)
    , m_buffering(false)
    , m_audioAvailable(false)
    , m_videoAvailable(false)
{
    m_player->get_controls(&m_controls);
    m_player->get_settings(&m_settings);
    m_player->get_network(&m_network);

    if (m_settings)
        m_settings->put_autoStart(FALSE);

    WMPPlayState state = wmppsUndefined;
    if (m_player->get_playState(&state) == S_OK)
        playStateChangeEvent(state);

    connect(events, SIGNAL(Buffering(VARIANT_BOOL)), this, SLOT(bufferingEvent(VARIANT_BOOL)));
    connect(events, SIGNAL(PositionChange(double,double)),
            this, SLOT(positionChangeEvent(double,double)));
    connect(events, SIGNAL(PlayStateChange(long)), this, SLOT(playStateChangeEvent(long)));
    connect(events, SIGNAL(CurrentItemChange(IDispatch*)),
            this, SLOT(currentItemChangeEvent(IDispatch*)));
    connect(events, SIGNAL(MediaChange(IDispatch*)), this, SLOT(mediaChangeEvent(IDispatch*)));
}

QWmpPlayerControl::~QWmpPlayerControl()
{
    if (m_controls) m_controls->Release();
    if (m_settings) m_settings->Release();
    if (m_network) m_network->Release();
}

QMediaPlayer::State QWmpPlayerControl::state() const
{
    return m_state;
}

QMediaPlayer::MediaStatus QWmpPlayerControl::mediaStatus() const
{
    return m_status;
}

qint64 QWmpPlayerControl::duration() const
{
    double duration = 0.;

    IWMPMedia *media = 0;
    if (m_controls && m_controls->get_currentItem(&media) == S_OK) {
        media->get_duration(&duration);
        media->Release();
    }

    return duration * 1000;
}

qint64 QWmpPlayerControl::position() const
{
    double position = 0.0;

    if (m_controls)
        m_controls->get_currentPosition(&position);

    return position * 1000;
}

void QWmpPlayerControl::setPosition(qint64 position)
{
    if (m_controls)
        m_controls->put_currentPosition(double(position) / 1000.);
}

int QWmpPlayerControl::volume() const
{
    long volume = 0;

    if (m_settings)
        m_settings->get_volume(&volume);

    return volume;
}

void QWmpPlayerControl::setVolume(int volume)
{
    if (m_settings && m_settings->put_volume(volume) == S_OK)
        emit volumeChanged(volume);
}

bool QWmpPlayerControl::isMuted() const
{
    VARIANT_BOOL mute = FALSE;

    if (m_settings)
        m_settings->get_mute(&mute);

    return mute;
}

void QWmpPlayerControl::setMuted(bool muted)
{
    if (m_settings && m_settings->put_mute(muted ? TRUE : FALSE) == S_OK)
        emit mutedChanged(muted);

}

int QWmpPlayerControl::bufferStatus() const
{
    long progress = 0;

    if (m_network)
        m_network->get_bufferingProgress(&progress);

    return progress;
}

bool QWmpPlayerControl::isVideoAvailable() const
{
    return m_videoAvailable;
}

bool QWmpPlayerControl::isAudioAvailable() const
{
    return m_audioAvailable;
}

void QWmpPlayerControl::setAudioAvailable(bool available)
{
    if (m_audioAvailable != available)
        emit audioAvailableChanged(m_audioAvailable = available);
}

void QWmpPlayerControl::setVideoAvailable(bool available)
{
    if (m_videoAvailable != available)
        emit videoAvailableChanged(m_videoAvailable = available);
}

bool QWmpPlayerControl::isSeekable() const
{
    return true;
}

QMediaTimeRange QWmpPlayerControl::availablePlaybackRanges() const
{
    QMediaTimeRange ranges;

    IWMPMedia *media = 0;
    if (m_controls && m_controls->get_currentItem(&media) == S_OK) {
        double duration = 0;
        media->get_duration(&duration);
        media->Release();

        if(duration > 0)
            ranges.addInterval(0, duration * 1000);
    }

    return ranges;
}

qreal QWmpPlayerControl::playbackRate() const
{
    double rate = 0.;

    if (m_settings)
        m_settings->get_rate(&rate);

    return rate;
}

void QWmpPlayerControl::setPlaybackRate(qreal rate)
{
    if (m_settings)
        m_settings->put_rate(rate);
}

void QWmpPlayerControl::play()
{
    if (m_controls)
        m_controls->play();
}

void QWmpPlayerControl::pause()
{
    if (m_controls)
        m_controls->pause();
}

void QWmpPlayerControl::stop()
{
    if (m_controls)
        m_controls->stop();
}

QMediaContent QWmpPlayerControl::media() const
{
    QMediaResourceList resources;

    QUrl tmpUrl = url();

    if (!tmpUrl.isEmpty())
        resources << QMediaResource(tmpUrl);

    return resources;
}

const QIODevice *QWmpPlayerControl::mediaStream() const
{
    return 0;
}

void QWmpPlayerControl::setMedia(const QMediaContent &content, QIODevice *stream)
{
    if (!content.isNull() && !stream)
        setUrl(content.canonicalUrl());
    else
        setUrl(QUrl());
}

bool QWmpPlayerControl::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        const int changes = m_changes;
        m_changes = 0;

        if (changes & DurationChanged)
            emit durationChanged(duration());
        if (changes & PositionChanged)
            emit positionChanged(position());
        if (changes & StatusChanged)
            emit mediaStatusChanged(m_status);
        if (changes & StateChanged)
            emit stateChanged(m_state);

        return true;
    } else {
        return QMediaPlayerControl::event(event);
    }
}

void QWmpPlayerControl::scheduleUpdate(int change)
{
    if (m_changes == 0)
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));

    m_changes |= change;
}

void QWmpPlayerControl::bufferingEvent(VARIANT_BOOL buffering)
{
    if (m_state != QMediaPlayer::StoppedState) {
        m_status = buffering
                ? QMediaPlayer::BufferingMedia
                : QMediaPlayer::BufferedMedia;

        scheduleUpdate(StatusChanged);
    }
}

void QWmpPlayerControl::currentItemChangeEvent(IDispatch *)
{
    scheduleUpdate(DurationChanged);
}

void QWmpPlayerControl::mediaChangeEvent(IDispatch *dispatch)
{
    IWMPMedia *media = 0;
    if (dispatch &&  dispatch->QueryInterface(
            __uuidof(IWMPMedia), reinterpret_cast<void **>(&media)) == S_OK) {
        IWMPMedia *currentMedia = 0;
        if (m_controls && m_controls->get_currentItem(&currentMedia) == S_OK) {
            VARIANT_BOOL isEqual = VARIANT_FALSE;
            if (media->get_isIdentical(currentMedia, &isEqual) == S_OK && isEqual)
                scheduleUpdate(DurationChanged);

            currentMedia->Release();
        }
        media->Release();
    }
}

void QWmpPlayerControl::positionChangeEvent(double , double)
{
    scheduleUpdate(PositionChanged);
}

void QWmpPlayerControl::playStateChangeEvent(long state)
{
    switch (state) {
    case wmppsUndefined:
        m_state = QMediaPlayer::StoppedState;
        m_status = QMediaPlayer::UnknownMediaStatus;
        scheduleUpdate(StatusChanged | StateChanged);
        break;
    case wmppsStopped:
        if (m_state != QMediaPlayer::StoppedState) {
            m_state = QMediaPlayer::StoppedState;
            scheduleUpdate(StateChanged);

            if (m_status != QMediaPlayer::EndOfMedia) {
                m_status = QMediaPlayer::LoadedMedia;
                scheduleUpdate(StatusChanged);
            }
        }
        break;
    case wmppsPaused:
        if (m_state != QMediaPlayer::PausedState && m_status != QMediaPlayer::BufferedMedia) {
            m_state = QMediaPlayer::PausedState;
            m_status = QMediaPlayer::BufferedMedia;
            scheduleUpdate(StatusChanged | StateChanged);
        } else if (m_state != QMediaPlayer::PausedState) {
            m_state = QMediaPlayer::PausedState;
            scheduleUpdate(StateChanged);
        } else if (m_status != QMediaPlayer::BufferedMedia) {
            m_status = QMediaPlayer::BufferedMedia;

            scheduleUpdate(StatusChanged);
        }
        break;
    case wmppsPlaying:
    case wmppsScanForward:
    case wmppsScanReverse:
        if (m_state != QMediaPlayer::PlayingState && m_status != QMediaPlayer::BufferedMedia) {
            m_state = QMediaPlayer::PlayingState;
            m_status = QMediaPlayer::BufferedMedia;
            scheduleUpdate(StatusChanged | StateChanged);
        } else if (m_state != QMediaPlayer::PlayingState) {
            m_state = QMediaPlayer::PlayingState;
            scheduleUpdate(StateChanged);
        } else if (m_status != QMediaPlayer::BufferedMedia) {
            m_status = QMediaPlayer::BufferedMedia;
            scheduleUpdate(StatusChanged);
        }

        if (m_state != QMediaPlayer::PlayingState) {
            m_state = QMediaPlayer::PlayingState;
            scheduleUpdate(StateChanged);
        }
        if (m_status != QMediaPlayer::BufferedMedia) {
            m_status = QMediaPlayer::BufferedMedia;
            scheduleUpdate(StatusChanged);
        }
        break;
    case wmppsBuffering:
    case wmppsWaiting:
        if (m_status != QMediaPlayer::StalledMedia && m_state != QMediaPlayer::StoppedState) {
            m_status = QMediaPlayer::StalledMedia;
            scheduleUpdate(StatusChanged);
        }
        break;
    case wmppsMediaEnded:
        if (m_status != QMediaPlayer::EndOfMedia && m_state != QMediaPlayer::StoppedState) {
            m_state = QMediaPlayer::StoppedState;
            m_status = QMediaPlayer::EndOfMedia;
            scheduleUpdate(StatusChanged | StateChanged);
        }
        break;
    case wmppsTransitioning:
        break;
    case wmppsReady:
        if (m_status != QMediaPlayer::LoadedMedia) {
            m_status = QMediaPlayer::LoadedMedia;
            scheduleUpdate(StatusChanged);
        }

        if (m_state != QMediaPlayer::StoppedState) {
            m_state = QMediaPlayer::StoppedState;
            scheduleUpdate(StateChanged);
        }
        break;
    case wmppsReconnecting:
        if (m_status != QMediaPlayer::StalledMedia && m_state != QMediaPlayer::StoppedState) {
            m_status = QMediaPlayer::StalledMedia;
            scheduleUpdate(StatusChanged);
        }
        break;
    default:
        break;
    }
}

QUrl QWmpPlayerControl::url() const
{
    BSTR string;
    if (m_player && m_player->get_URL(&string) == S_OK) {
        QUrl url(QString::fromWCharArray(string, SysStringLen(string)), QUrl::StrictMode);

        SysFreeString(string);

        return url;
    } else {
        return QUrl();
    }
}

void QWmpPlayerControl::setUrl(const QUrl &url)
{
    if (url != QWmpPlayerControl::url() && m_player) {
        BSTR string = SysAllocString(reinterpret_cast<const wchar_t *>(url.toString().unicode()));

        m_player->put_URL(string);

        SysFreeString(string);
    }
}
