/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
****************************************************************************/

#include "qaudioengine_p.h"
#include "qsoundsource_p.h"
#include "qdebug.h"

#include "qaudioengine_openal_p.h"

#define DEBUG_AUDIOENGINE

QT_USE_NAMESPACE

QSoundSource::QSoundSource(QObject *parent)
    : QObject(parent)
{
}

QSoundBuffer::QSoundBuffer(QObject *parent)
    : QObject(parent)
{
}

QAudioEngine* QAudioEngine::create(QObject *parent)
{
    return new QAudioEngine(parent);
}

QAudioEngine::QAudioEngine(QObject *parent)
    : QObject(parent)
    , m_listenerUp(0, 0, 1)
    , m_listenerDirection(0, 1, 0)
{
    d = new QAudioEnginePrivate(this);
    connect(d, SIGNAL(isLoadingChanged()), this, SIGNAL(isLoadingChanged()));
    setDopplerFactor(1);
    setSpeedOfSound(qreal(343.33));
    updateListenerOrientation();
}

QAudioEngine::~QAudioEngine()
{
#ifdef DEBUG_AUDIOENGINE
    qDebug() << "QAudioEngine::dtor";
#endif
}

QSoundBuffer* QAudioEngine::getStaticSoundBuffer(const QUrl& url)
{
    return d->getStaticSoundBuffer(url);
}

void QAudioEngine::releaseSoundBuffer(QSoundBuffer *buffer)
{
    d->releaseSoundBuffer(buffer);
}

QSoundSource* QAudioEngine::createSoundSource()
{
    return d->createSoundSource();
}

void QAudioEngine::releaseSoundSource(QSoundSource *soundInstance)
{
    d->releaseSoundSource(soundInstance);
}

bool QAudioEngine::isLoading() const
{
    return d->isLoading();
}

QVector3D QAudioEngine::listenerPosition() const
{
    return d->listenerPosition();
}

QVector3D QAudioEngine::listenerDirection() const
{
    return m_listenerDirection;
}

QVector3D QAudioEngine::listenerUp() const
{
    return m_listenerUp;
}

qreal QAudioEngine::listenerGain() const
{
    return d->listenerGain();
}

QVector3D QAudioEngine::listenerVelocity() const
{
    return d->listenerVelocity();
}

void QAudioEngine::setListenerPosition(const QVector3D& position)
{
    d->setListenerPosition(position);
}

void QAudioEngine::setListenerVelocity(const QVector3D& velocity)
{
    d->setListenerVelocity(velocity);
}

void QAudioEngine::setListenerDirection(const QVector3D& direction)
{
    if (m_listenerDirection == direction)
        return;
    m_listenerDirection = direction;
    updateListenerOrientation();
}

void QAudioEngine::setListenerUp(const QVector3D& up)
{
    if (m_listenerUp == up)
        return;
    m_listenerUp = up;
    updateListenerOrientation();
}

void QAudioEngine::updateListenerOrientation()
{
    QVector3D dir = m_listenerDirection;
    QVector3D up = m_listenerUp;
    dir.normalize();
    up.normalize();
    QVector3D u = up - dir * QVector3D::dotProduct(dir, up);
    u.normalize();
    d->setListenerOrientation(dir, u);
}

void QAudioEngine::setListenerGain(qreal gain)
{
    d->setListenerGain(gain);
}

qreal QAudioEngine::dopplerFactor() const
{
    return m_dopplerFactor;
}

void QAudioEngine::setDopplerFactor(qreal dopplerFactor)
{
    m_dopplerFactor = dopplerFactor;
    d->setDopplerFactor(dopplerFactor);
}

qreal QAudioEngine::speedOfSound() const
{
    return m_speedOfSound;
}

void QAudioEngine::setSpeedOfSound(qreal speedOfSound)
{
    m_speedOfSound = speedOfSound;
    d->setSpeedOfSound(speedOfSound);
}
