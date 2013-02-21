/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qdeclarative_playvariation_p.h"
#include "qdeclarative_audioengine_p.h"
#include "qsoundinstance_p.h"
#include "qdebug.h"

#define DEBUG_AUDIOENGINE

QT_USE_NAMESPACE

/*!
    \qmltype PlayVariation
    \instantiates QDeclarativePlayVariation
    \since 1.0
    \brief Define a playback variation for \l {Sound} {sounds}.
    So each time the playback of the same sound can be a slightly different even with the same
    AudioSample.

    \inqmlmodule QtAudioEngine 1.0
    \ingroup multimedia_audioengine
    \inherits Item
    \preliminary

    This type is part of the \b{QtAudioEngine 1.0} module.

    PlayVariation must be defined inside a \l Sound.

    \qml
    import QtQuick 2.0
    import QtAudioEngine 1.0

    Rectangle {
        color:"white"
        width: 300
        height: 500

        AudioEngine {
            id:audioengine

            AudioSample {
                name:"explosion01"
                source: "explosion-01.wav"
            }

            AudioSample {
                name:"explosion02"
                source: "explosion-02.wav"
            }

            Sound {
                name:"explosion"
                PlayVariation {
                    sample:"explosion01"
                    minPitch: 0.8
                    maxPitch: 1.1
                }
                PlayVariation {
                    sample:"explosion01"
                    minGain: 1.1
                    maxGain: 1.5
                }
            }
        }
    }
    \endqml

*/
QDeclarativePlayVariation::QDeclarativePlayVariation(QObject *parent)
    : QObject(parent)
    , m_complete(false)
    , m_looping(false)
    , m_maxGain(1)
    , m_minGain(1)
    , m_maxPitch(1)
    , m_minPitch(1)
    , m_sampleObject(0)
{
}

QDeclarativePlayVariation::~QDeclarativePlayVariation()
{
}

void QDeclarativePlayVariation::classBegin()
{
    if (!parent() || !parent()->inherits("QDeclarativeSound")) {
        qWarning("PlayVariation must be defined inside Sound!");
        return;
    }
}

void QDeclarativePlayVariation::componentComplete()
{
    if (m_maxGain < m_minGain) {
        qWarning("PlayVariation: maxGain must be no less than minGain");
        qSwap(m_minGain, m_maxGain);
    }
    if (m_maxPitch < m_minPitch) {
        qWarning("PlayVariation: maxPitch must be no less than minPitch");
        qSwap(m_minPitch, m_maxPitch);
    }
    m_complete = true;
}

/*!
    \qmlproperty string QtAudioEngine1::PlayVariation::sample

    This property specifies which \l AudioSample this variation will use.
*/
QString QDeclarativePlayVariation::sample() const
{
    return m_sample;
}

void QDeclarativePlayVariation::setSample(const QString& sample)
{
    if (m_complete) {
        qWarning("PlayVariation: cannot change properties after initialization.");
        return;
    }
    m_sample = sample;
}

/*!
    \qmlproperty bool QtAudioEngine1::PlayVariation::looping

    This property indicates whether the playback will be looped or not.
*/
bool QDeclarativePlayVariation::isLooping() const
{
    return m_looping;
}

void QDeclarativePlayVariation::setLooping(bool looping)
{
    if (m_complete) {
        qWarning("PlayVariation: cannot change properties after initialization.");
        return;
    }
    m_looping = looping;
}

/*!
    \qmlproperty real QtAudioEngine1::PlayVariation::maxGain

    This property specifies the maximum gain adjustment that can be applied in any playback.
*/
qreal QDeclarativePlayVariation::maxGain() const
{
    return m_maxGain;
}

void QDeclarativePlayVariation::setMaxGain(qreal maxGain)
{
    if (m_complete) {
        qWarning("PlayVariation: cannot change properties after initialization.");
        return;
    }
    if (maxGain <= 0) {
        qWarning("PlayVariation: maxGain must be greater than 0");
        return;
    }
    m_maxGain = maxGain;
}

/*!
    \qmlproperty real QtAudioEngine1::PlayVariation::minGain

    This property specifies the minimum gain adjustment that can be applied in any playback.
*/
qreal QDeclarativePlayVariation::minGain() const
{
    return m_minGain;
}

void QDeclarativePlayVariation::setMinGain(qreal minGain)
{
    if (m_complete) {
        qWarning("PlayVariation: cannot change properties after initialization.");
        return;
    }
    if (minGain < 0) {
        qWarning("PlayVariation: minGain must be no less than 0");
        return;
    }
    m_minGain = minGain;
}

/*!
    \qmlproperty real QtAudioEngine1::PlayVariation::maxPitch

    This property specifies the maximum pitch adjustment that can be applied in any playback.
*/
qreal QDeclarativePlayVariation::maxPitch() const
{
    return m_maxPitch;
}

void QDeclarativePlayVariation::setMaxPitch(qreal maxPitch)
{
    if (m_complete) {
        qWarning("PlayVariation: cannot change properties after initialization.");
        return;
    }
    if (maxPitch < 0) {
        qWarning("PlayVariation: maxPitch must be no less than 0");
        return;
    }
    m_maxPitch = maxPitch;
}

/*!
    \qmlproperty real QtAudioEngine1::PlayVariation::minPitch

    This property specifies the minimum pitch adjustment that can be applied in any playback.
*/
qreal QDeclarativePlayVariation::minPitch() const
{
    return m_minPitch;
}

void QDeclarativePlayVariation::setMinPitch(qreal minPitch)
{
    if (m_complete) {
        qWarning("PlayVariation: cannot change properties after initialization.");
        return;
    }
    if (m_minPitch < 0) {
        qWarning("PlayVariation: m_minPitch must be no less than 0");
        return;
    }
    m_minPitch = minPitch;
}

QDeclarativeAudioSample* QDeclarativePlayVariation::sampleObject() const
{
    return m_sampleObject;
}

void QDeclarativePlayVariation::setSampleObject(QDeclarativeAudioSample *sampleObject)
{
    m_sampleObject = sampleObject;
}

void QDeclarativePlayVariation::applyParameters(QSoundInstance *soundInstance)
{
    qreal pitch = qreal(qrand() % 1001) * 0.001f * (m_maxPitch - m_minPitch) + m_minPitch;
    qreal gain = qreal(qrand() % 1001) * 0.001f * (m_maxGain - m_minGain) + m_minGain;
    soundInstance->updateVariationParameters(pitch, gain, m_looping);
}
