/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfmediaplayer_p.h"
#include "avfmediaplayersession_p.h"

QT_USE_NAMESPACE

AVFMediaPlayer::AVFMediaPlayer(QObject *parent) :
    QMediaPlayerControl(parent)
{
    setSession(new AVFMediaPlayerSession(this));
}

AVFMediaPlayer::~AVFMediaPlayer()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
}

void AVFMediaPlayer::setSession(AVFMediaPlayerSession *session)
{
    m_session = session;

    connect(m_session, SIGNAL(positionChanged(qint64)), this, SIGNAL(positionChanged(qint64)));
    connect(m_session, SIGNAL(durationChanged(qint64)), this, SIGNAL(durationChanged(qint64)));
    connect(m_session, SIGNAL(bufferStatusChanged(int)), this, SIGNAL(bufferStatusChanged(int)));
    connect(m_session, SIGNAL(stateChanged(QMediaPlayer::State)),
            this, SIGNAL(stateChanged(QMediaPlayer::State)));
    connect(m_session, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            this, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    connect(m_session, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)));
    connect(m_session, SIGNAL(mutedChanged(bool)), this, SIGNAL(mutedChanged(bool)));
    connect(m_session, SIGNAL(audioAvailableChanged(bool)), this, SIGNAL(audioAvailableChanged(bool)));
    connect(m_session, SIGNAL(videoAvailableChanged(bool)), this, SIGNAL(videoAvailableChanged(bool)));
    connect(m_session, SIGNAL(error(int,QString)), this, SIGNAL(error(int,QString)));
    connect(m_session, &AVFMediaPlayerSession::playbackRateChanged,
            this, &AVFMediaPlayer::playbackRateChanged);
    connect(m_session, &AVFMediaPlayerSession::seekableChanged,
            this, &AVFMediaPlayer::seekableChanged);
}

QMediaPlayer::State AVFMediaPlayer::state() const
{
    return m_session->state();
}

QMediaPlayer::MediaStatus AVFMediaPlayer::mediaStatus() const
{
    return m_session->mediaStatus();
}

QUrl AVFMediaPlayer::media() const
{
    return m_session->media();
}

const QIODevice *AVFMediaPlayer::mediaStream() const
{
    return m_session->mediaStream();
}

void AVFMediaPlayer::setMedia(const QUrl &content, QIODevice *stream)
{
    const QUrl oldContent = m_session->media();

    m_session->setMedia(content, stream);
}

qint64 AVFMediaPlayer::position() const
{
    return m_session->position();
}

qint64 AVFMediaPlayer::duration() const
{
    return m_session->duration();
}

int AVFMediaPlayer::bufferStatus() const
{
    return m_session->bufferStatus();
}

int AVFMediaPlayer::volume() const
{
    return m_session->volume();
}

bool AVFMediaPlayer::isMuted() const
{
    return m_session->isMuted();
}

bool AVFMediaPlayer::isAudioAvailable() const
{
    return m_session->isAudioAvailable();
}

bool AVFMediaPlayer::isVideoAvailable() const
{
    return m_session->isVideoAvailable();
}

bool AVFMediaPlayer::isSeekable() const
{
    return m_session->isSeekable();
}

QMediaTimeRange AVFMediaPlayer::availablePlaybackRanges() const
{
    return m_session->availablePlaybackRanges();
}

qreal AVFMediaPlayer::playbackRate() const
{
    return m_session->playbackRate();
}

void AVFMediaPlayer::setPlaybackRate(qreal rate)
{
    m_session->setPlaybackRate(rate);
}

bool AVFMediaPlayer::setAudioOutput(const QAudioDeviceInfo &info)
{
#ifdef Q_OS_IOS
    Q_UNUSED(info);
    return false;
#else
    return m_session->setAudioOutput(info);
#endif
}

QAudioDeviceInfo AVFMediaPlayer::audioOutput() const
{
#ifdef Q_OS_IOS
    return QAudioDeviceInfo();
#else
    return m_session->audioOutput();
#endif
}

QMediaMetaData AVFMediaPlayer::metaData() const
{
    return m_metaData;
}

void AVFMediaPlayer::setMetaData(const QMediaMetaData &metaData)
{
    m_metaData = metaData;
    metaDataChanged();
}

void AVFMediaPlayer::setPosition(qint64 pos)
{
    m_session->setPosition(pos);
}

void AVFMediaPlayer::play()
{
    m_session->play();
}

void AVFMediaPlayer::pause()
{
    m_session->pause();
}

void AVFMediaPlayer::stop()
{
    m_session->stop();
}

void AVFMediaPlayer::setVolume(int volume)
{
    m_session->setVolume(volume);
}

void AVFMediaPlayer::setMuted(bool muted)
{
    m_session->setMuted(muted);
}

void AVFMediaPlayer::setVideoSurface(QAbstractVideoSurface *surface)
{
    m_session->setVideoSurface(surface);
}
