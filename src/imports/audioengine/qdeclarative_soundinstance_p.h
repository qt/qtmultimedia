/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QDECLARATIVE_SOUNDINSTANCE_P_H
#define QDECLARATIVE_SOUNDINSTANCE_P_H

#include <QtCore/QObject>
#include <QtGui/qvector3d.h>
#include "qsoundinstance_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDeclarativeSound;
class QAudioEngine;
class QDeclarativeAudioEngine;

class QDeclarativeSoundInstance : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDeclarativeAudioEngine* engine READ engine WRITE setEngine)
    Q_PROPERTY(QString sound READ sound WRITE setSound NOTIFY soundChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(QVector3D direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(QVector3D velocity READ velocity WRITE setVelocity NOTIFY velocityChanged)
    Q_PROPERTY(qreal gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(qreal pitch READ pitch WRITE setPitch NOTIFY pitchChanged)

    Q_ENUMS(State)

public:
    enum State
    {
        StopppedState = QSoundInstance::StopppedState,
        PlayingState = QSoundInstance::PlayingState,
        PausedState = QSoundInstance::PausedState
    };

    QDeclarativeSoundInstance(QObject *parent = 0);
    ~QDeclarativeSoundInstance();

    QDeclarativeAudioEngine* engine() const;
    void setEngine(QDeclarativeAudioEngine *engine);

    QString sound() const;
    void setSound(const QString& sound);

    State state() const;

    QVector3D position() const;
    void setPosition(const QVector3D& position);

    QVector3D direction() const;
    void setDirection(const QVector3D& direction);

    QVector3D velocity() const;
    void setVelocity(const QVector3D& velocity);

    qreal gain() const;
    void setGain(qreal gain);

    qreal pitch() const;
    void setPitch(qreal pitch);

    void setConeInnerAngle(qreal innerAngle);
    void setConeOuterAngle(qreal outerAngle);
    void setConeOuterGain(qreal outerGain);

Q_SIGNALS:
    void stateChanged();
    void positionChanged();
    void directionChanged();
    void velocityChanged();
    void gainChanged();
    void pitchChanged();
    void soundChanged();

public Q_SLOTS:
    void play();
    void stop();
    void pause();
    void updatePosition(qreal deltaTime);

private Q_SLOTS:
    void handleStateChanged();

private:
    Q_DISABLE_COPY(QDeclarativeSoundInstance);
    QString m_sound;
    QVector3D m_position;
    QVector3D m_direction;
    QVector3D m_velocity;
    qreal m_gain;
    qreal m_pitch;
    State m_requestState;

    qreal m_coneInnerAngle;
    qreal m_coneOuterAngle;
    qreal m_coneOuterGain;

    void dropInstance();

    QSoundInstance *m_instance;
    QDeclarativeAudioEngine *m_engine;


private Q_SLOTS:
    void engineComplete();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
