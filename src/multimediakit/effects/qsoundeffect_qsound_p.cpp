/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// INTERNAL USE ONLY: Do NOT use for any other purpose.
//

#include "qsoundeffect_qsound_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtWidgets/qsound.h>
#include <QtCore/qstringlist.h>


QT_BEGIN_NAMESPACE

QSoundEffectPrivate::QSoundEffectPrivate(QObject* parent):
    QObject(parent),
    m_playing(false),
    m_timerID(0),
    m_muted(false),
    m_loopCount(1),
    m_volume(100),
    m_status(QSoundEffect::Null),
    m_sound(0)
{
    if (!QSound::isAvailable())
        qWarning("SoundEffect(qsound) : not available");
}

QSoundEffectPrivate::~QSoundEffectPrivate()
{
}

QStringList QSoundEffectPrivate::supportedMimeTypes()
{
    QStringList supportedTypes;
    supportedTypes << QLatin1String("audio/x-wav") << QLatin1String("audio/vnd.wave") ;
    return supportedTypes;
}

QUrl QSoundEffectPrivate::source() const
{
    return m_source;
}

void QSoundEffectPrivate::setSource(const QUrl &url)
{
    if (url.isEmpty()) {
        m_source = QUrl();
        setStatus(QSoundEffect::Null);
        return;
    }

    if (url.scheme() != QLatin1String("file")) {
        m_source = url;
        setStatus(QSoundEffect::Error);
        return;
    }

    if (m_sound != 0)
        delete m_sound;

    m_source = url;
    m_sound = new QSound(m_source.toLocalFile(), this);
    m_sound->setLoops(m_loopCount);
    m_status = QSoundEffect::Ready;
    emit statusChanged();
    emit loadedChanged();
}

int QSoundEffectPrivate::loopCount() const
{
    return m_loopCount;
}

void QSoundEffectPrivate::setLoopCount(int lc)
{
    m_loopCount = lc;
    if (m_sound)
        m_sound->setLoops(lc);
}

int QSoundEffectPrivate::volume() const
{
    return m_volume;
}

void QSoundEffectPrivate::setVolume(int v)
{
    m_volume = v;
}

bool QSoundEffectPrivate::isMuted() const
{
    return m_muted;
}

void QSoundEffectPrivate::setMuted(bool muted)
{
    m_muted = muted;
}

bool QSoundEffectPrivate::isLoaded() const
{
    return m_status == QSoundEffect::Ready;
}

void QSoundEffectPrivate::play()
{
    if (m_status == QSoundEffect::Null || m_status == QSoundEffect::Error)
        return;
    if (m_timerID != 0)
        killTimer(m_timerID);
    m_timerID = startTimer(500);
    m_sound->play();
    setPlaying(true);
}


void QSoundEffectPrivate::stop()
{
    if (m_timerID != 0)
        killTimer(m_timerID);
    m_timerID = 0;
    m_sound->stop();
    setPlaying(false);
}

bool QSoundEffectPrivate::isPlaying()
{
    if (m_playing && m_sound && m_sound->isFinished()) {
        if (m_timerID != 0)
            killTimer(m_timerID);
        m_timerID = 0;
        setPlaying(false);
    }
    return m_playing;
}

QSoundEffect::Status QSoundEffectPrivate::status() const
{
    return m_status;
}

void QSoundEffectPrivate::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
    setPlaying(!m_sound->isFinished());
    if (isPlaying())
        return;
    killTimer(m_timerID);
    m_timerID = 0;
}

void QSoundEffectPrivate::setStatus(QSoundEffect::Status status)
{
    if (m_status == status)
        return;
    bool oldLoaded = isLoaded();
    m_status = status;
    emit statusChanged();
    if (oldLoaded != isLoaded())
        emit loadedChanged();
}

void QSoundEffectPrivate::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
    emit playingChanged();
}

QT_END_NAMESPACE

#include "moc_qsoundeffect_qsound_p.cpp"
