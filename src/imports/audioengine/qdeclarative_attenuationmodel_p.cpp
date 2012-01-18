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

#include "qdeclarative_attenuationmodel_p.h"
#include "qdebug.h"

#define DEBUG_AUDIOENGINE

QT_USE_NAMESPACE

QDeclarativeAttenuationModel::QDeclarativeAttenuationModel(QObject *parent)
    : QObject(parent)
    , m_complete(false)
{
}

QDeclarativeAttenuationModel::~QDeclarativeAttenuationModel()
{
}

void QDeclarativeAttenuationModel::classBegin()
{
    if (!parent() || !parent()->inherits("QDeclarativeAudioEngine")) {
        qWarning("AttenuationModel must be defined inside AudioEngine!");
        //TODO: COMPILE_EXCEPTION ?
        return;
    }
}

void QDeclarativeAttenuationModel::componentComplete()
{
    if (m_name.isEmpty()) {
        qWarning("AttenuationModel must have a name!");
        return;
    }
    m_complete = true;
}

QString QDeclarativeAttenuationModel::name() const
{
    return m_name;
}

void QDeclarativeAttenuationModel::setName(const QString& name)
{
    if (m_complete) {
        qWarning("AttenuationModel: you can not change name after initialization.");
        return;
    }
    m_name = name;
}

//////////////////////////////////////////////////////////////////////////////////////////
/*!
    \qmlclass AttenuationModelLinear QDeclarativeAttenuationModelLinear
    \since 5.0
    \brief The AttenuationModelLinear element allows you to define a linear attenuation curve for
    Sound element.
    \ingroup qml-multimedia
    \inherits Item

    This element is part of the \bold{QtAudioEngine 1.0} module.

    AttenuationModelLinear must be defined inside AudioEngine.

    \qml
    import QtQuick 2.0
    import QtAudioEngine 1.0


    Rectangle {
        color:"white"
        width: 300
        height: 500

        AudioEngine {
            id:audioengine

            AttenuationModelLinear {
               name:"linear"
               start: 20
               end: 180
            }

            AudioSample {
                name:"explosion"
                source: "explosion-02.wav"
            }

            Sound {
                name:"explosion"
                attenuationModel: "linear"
                PlayVariation {
                    sample:"explosion"
                }
            }
        }
    }
    \endqml
*/

/*!
    \qmlproperty string AttenuationModelLinear::name

    This property holds the name of AttenuationModelLinear, must be unique among all attenuation
    models and only defined once.
*/
QDeclarativeAttenuationModelLinear::QDeclarativeAttenuationModelLinear(QObject *parent)
    : QDeclarativeAttenuationModel(parent)
    , m_start(0)
    , m_end(1)
{
}

void QDeclarativeAttenuationModelLinear::componentComplete()
{
    if (m_start > m_end) {
        qSwap(m_start, m_end);
        qWarning() << "AttenuationModelLinear[" << m_name << "]: start must be less or equal than end.";
    }
    QDeclarativeAttenuationModel::componentComplete();
}

/*!
    \qmlproperty real AttenuationModelLinear::start

    This property holds the start distance. There will be no attenuation if the distance from sound
    to listener is within this range.
    The default value is 0.
*/
qreal QDeclarativeAttenuationModelLinear::startDistance() const
{
    return m_start;
}

void QDeclarativeAttenuationModelLinear::setStartDistance(qreal startDist)
{
    if (m_complete) {
        qWarning() << "AttenuationModelLinear[" << m_name << "]: you can not change properties after initialization.";
        return;
    }
    if (startDist < 0) {
        qWarning() << "AttenuationModelLinear[" << m_name << "]: start must be no less than 0.";
        return;
    }
    m_start = startDist;
}

/*!
    \qmlproperty real AttenuationModelLinear::end

    This property holds the end distance. There will be no sound hearable if the distance from sound
    to listener is larger than this.
    The default value is 1.
*/
qreal QDeclarativeAttenuationModelLinear::endDistance() const
{
    return m_end;
}

void QDeclarativeAttenuationModelLinear::setEndDistance(qreal endDist)
{
    if (m_complete) {
        qWarning() << "AttenuationModelLinear[" << m_name << "]: you can not change properties after initialization.";
        return;
    }
    if (endDist < 0) {
        qWarning() << "AttenuationModelLinear[" << m_name << "]: end must be no greater than 0.";
        return;
    }
    m_end = endDist;
}

qreal QDeclarativeAttenuationModelLinear::calculateGain(const QVector3D &listenerPosition, const QVector3D &sourcePosition) const
{
    qreal md = m_end - m_start;
    if (md == 0)
        return 1;
    qreal d = qBound(qreal(0), (listenerPosition - sourcePosition).length() - m_start, md);
    return qreal(1) - (d / md);
}

//////////////////////////////////////////////////////////////////////////////////////////
/*!
    \qmlclass AttenuationModelInverse QDeclarativeAttenuationModelInverse
    \since 5.0
    \brief The AttenuationModelInverse element allows you to define a non-linear attenuation curve
    for Sound element.
    \ingroup qml-multimedia
    \inherits Item

    This element is part of the \bold{QtAudioEngine 1.0} module.

    AttenuationModelInverse must be defined inside AudioEngine.

    \qml
    import QtQuick 2.0
    import QtAudioEngine 1.0


    Rectangle {
        color:"white"
        width: 300
        height: 500

        AudioEngine {
            id:audioengine

            AttenuationModelInverse {
               name:"linear"
               start: 20
               end: 500
               rolloff: 1.5
            }

            AudioSample {
                name:"explosion"
                source: "explosion-02.wav"
            }

            Sound {
                name:"explosion"
                attenuationModel: "linear"
                PlayVariation {
                    sample:"explosion"
                }
            }
        }
    }
    \endqml

    Attenuation factor is calculated as below:

    distance: distance from sound to listener
    d = min(max(distance, start), end);
    attenuation = start / (start + (d - start) * rolloff);
*/

/*!
    \qmlproperty string AttenuationModelInverse::name

    This property holds the name of AttenuationModelInverse, must be unique among all attenuation
    models and only defined once.
*/

/*!
    \qmlproperty real AttenuationModelInverse::start

    This property holds the start distance. There will be no attenuation if the distance from sound
    to listener is within this range.
    The default value is 1.
*/

/*!
    \qmlproperty real AttenuationModelInverse::end

    This property holds the end distance. There will be no further attenuation if the distance from
    sound to listener is larger than this.
    The default value is 1000.
*/

/*!
    \qmlproperty real AttenuationModelInverse::rolloff

    This property holds the rolloff factor. The bigger the value is, the faster the sound attenuates.
    The default value is 1.
*/

QDeclarativeAttenuationModelInverse::QDeclarativeAttenuationModelInverse(QObject *parent)
    : QDeclarativeAttenuationModel(parent)
    , m_ref(1)
    , m_max(1000)
    , m_rolloff(1)
{
}

void QDeclarativeAttenuationModelInverse::componentComplete()
{
    if (m_ref > m_max) {
        qSwap(m_ref, m_max);
        qWarning() << "AttenuationModelInverse[" << m_name << "]: referenceDistance must be less or equal than maxDistance.";
    }
    QDeclarativeAttenuationModel::componentComplete();
}

qreal QDeclarativeAttenuationModelInverse::referenceDistance() const
{
    return m_ref;
}

void QDeclarativeAttenuationModelInverse::setReferenceDistance(qreal referenceDistance)
{
    if (m_complete) {
        qWarning() << "AttenuationModelInverse[" << m_name << "]: you can not change properties after initialization.";
        return;
    }
    if (referenceDistance <= 0) {
        qWarning() << "AttenuationModelInverse[" << m_name << "]: referenceDistance must be greater than 0.";
        return;
    }
    m_ref = referenceDistance;
}

qreal QDeclarativeAttenuationModelInverse::maxDistance() const
{
    return m_max;
}

void QDeclarativeAttenuationModelInverse::setMaxDistance(qreal maxDistance)
{
    if (m_complete) {
        qWarning() << "AttenuationModelInverse[" << m_name << "]: you can not change properties after initialization.";
        return;
    }
    if (maxDistance <= 0) {
        qWarning() << "AttenuationModelInverse[" << m_name << "]: maxDistance must be greater than 0.";
        return;
    }
    m_max = maxDistance;
}

qreal QDeclarativeAttenuationModelInverse::rolloffFactor() const
{
    return m_rolloff;
}

void QDeclarativeAttenuationModelInverse::setRolloffFactor(qreal rolloffFactor)
{
    if (m_complete) {
        qWarning() << "AttenuationModelInverse[" << m_name << "]: you can not change properties after initialization.";
        return;
    }
    m_rolloff = rolloffFactor;
}

qreal QDeclarativeAttenuationModelInverse::calculateGain(const QVector3D &listenerPosition, const QVector3D &sourcePosition) const
{
    Q_ASSERT(m_ref > 0);
    return m_ref / (m_ref + (qBound(m_ref, (listenerPosition - sourcePosition).length(), m_max) - m_ref) * m_rolloff);
}

