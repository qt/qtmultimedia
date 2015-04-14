/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QAUDIOENGINE_OPENAL_P_H
#define QAUDIOENGINE_OPENAL_P_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QUrl>

#if defined(HEADER_OPENAL_PREFIX)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "qsoundsource_p.h"
#include "qsoundbuffer_p.h"

QT_BEGIN_NAMESPACE

class QSample;
class QSampleCache;

class QSoundBufferPrivateAL : public QSoundBuffer
{
    Q_OBJECT
public:
    QSoundBufferPrivateAL(QObject* parent);
    virtual void bindToSource(ALuint alSource) = 0;
    virtual void unbindFromSource(ALuint alSource) = 0;
};


class StaticSoundBufferAL : public QSoundBufferPrivateAL
{
    Q_OBJECT

public:
    StaticSoundBufferAL(QObject *parent, const QUrl &url, QSampleCache *sampleLoader);
    ~StaticSoundBufferAL();

    State state() const Q_DECL_OVERRIDE;

    void load() Q_DECL_OVERRIDE;

    void bindToSource(ALuint alSource) Q_DECL_OVERRIDE;
    void unbindFromSource(ALuint alSource) Q_DECL_OVERRIDE;

    inline long addRef() { return ++m_ref; }
    inline long release() { return --m_ref; }
    inline long refCount() const { return m_ref; }

public Q_SLOTS:
    void sampleReady();
    void decoderError();

private:
    long m_ref;
    QUrl m_url;
    ALuint m_alBuffer;
    State m_state;
    QSample *m_sample;
    QSampleCache *m_sampleLoader;
};


class QSoundSourcePrivate : public QSoundSource
{
    Q_OBJECT
public:
    QSoundSourcePrivate(QObject *parent);
    ~QSoundSourcePrivate();

    void play();
    void pause();
    void stop();

    QSoundSource::State state() const;

    bool isLooping() const;
    void setLooping(bool looping);
    void setPosition(const QVector3D& position);
    void setDirection(const QVector3D& direction);
    void setVelocity(const QVector3D& velocity);

    QVector3D velocity() const;
    QVector3D position() const;
    QVector3D direction() const;

    void setGain(qreal gain);
    void setPitch(qreal pitch);
    void setCone(qreal innerAngle, qreal outerAngle, qreal outerGain);

    void bindBuffer(QSoundBuffer*);
    void unbindBuffer();

    void checkState();

    void release();

Q_SIGNALS:
    void activate(QObject*);

private:
    void sourcePlay();
    void sourcePause();

    ALuint  m_alSource;
    QSoundBufferPrivateAL *m_bindBuffer;
    bool                 m_isReady; //true if the sound source is already bound to some sound buffer
    QSoundSource::State  m_state;
    qreal   m_gain;
    qreal   m_pitch;
    qreal   m_coneInnerAngle;
    qreal   m_coneOuterAngle;
    qreal   m_coneOuterGain;
};


class QAudioEnginePrivate : public QObject
{
    Q_OBJECT
public:
    QAudioEnginePrivate(QObject *parent);
    ~QAudioEnginePrivate();

    bool isLoading() const;

    QSoundSource* createSoundSource();
    void releaseSoundSource(QSoundSource *soundInstance);
    QSoundBuffer* getStaticSoundBuffer(const QUrl& url);
    void releaseSoundBuffer(QSoundBuffer *buffer);

    QVector3D listenerPosition() const;
    QVector3D listenerVelocity() const;
    qreal listenerGain() const;
    void setListenerPosition(const QVector3D& position);
    void setListenerVelocity(const QVector3D& velocity);
    void setListenerOrientation(const QVector3D& direction, const QVector3D& up);
    void setListenerGain(qreal gain);
    void setDopplerFactor(qreal dopplerFactor);
    void setSpeedOfSound(qreal speedOfSound);

    static bool checkNoError(const char *msg);

Q_SIGNALS:
    void isLoadingChanged();

private Q_SLOTS:
    void updateSoundSources();
    void soundSourceActivate(QObject *soundSource);

private:
    QList<QSoundSourcePrivate*> m_activeInstances;
    QList<QSoundSourcePrivate*> m_instancePool;
    QMap<QUrl, QSoundBufferPrivateAL*> m_staticBufferPool;

    QSampleCache *m_sampleLoader;
    QTimer m_updateTimer;
};

QT_END_NAMESPACE

#endif
