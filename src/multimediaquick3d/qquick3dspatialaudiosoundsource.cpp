/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Spatial Audio module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-NOGPL2$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qquick3dspatialaudiosoundsource_p.h"
#include "qquick3dspatialaudioengine_p.h"
#include "qspatialaudiosoundsource.h"
#include <QAudioFormat>
#include <qdir.h>

QT_BEGIN_NAMESPACE

QQuick3DSpatialAudioSoundSource::QQuick3DSpatialAudioSoundSource()
{
    m_sound = new QSpatialAudioSoundSource(QQuick3DSpatialAudioEngine::getEngine());

    connect(this, &QQuick3DNode::scenePositionChanged, this, &QQuick3DSpatialAudioSoundSource::updatePosition);
    connect(this, &QQuick3DNode::sceneRotationChanged, this, &QQuick3DSpatialAudioSoundSource::updateRotation);
    connect(m_sound, &QSpatialAudioSoundSource::sourceChanged, this, &QQuick3DSpatialAudioSoundSource::sourceChanged);
    connect(m_sound, &QSpatialAudioSoundSource::volumeChanged, this, &QQuick3DSpatialAudioSoundSource::volumeChanged);
    connect(m_sound, &QSpatialAudioSoundSource::distanceModelChanged, this, &QQuick3DSpatialAudioSoundSource::distanceModelChanged);
    connect(m_sound, &QSpatialAudioSoundSource::minimumDistanceChanged, this, &QQuick3DSpatialAudioSoundSource::minimumDistanceChanged);
    connect(m_sound, &QSpatialAudioSoundSource::maximumDistanceChanged, this, &QQuick3DSpatialAudioSoundSource::maximumDistanceChanged);
    connect(m_sound, &QSpatialAudioSoundSource::manualAttenuationChanged, this, &QQuick3DSpatialAudioSoundSource::manualAttenuationChanged);
    connect(m_sound, &QSpatialAudioSoundSource::occlusionIntensityChanged, this, &QQuick3DSpatialAudioSoundSource::occlusionIntensityChanged);
    connect(m_sound, &QSpatialAudioSoundSource::directivityChanged, this, &QQuick3DSpatialAudioSoundSource::directivityChanged);
    connect(m_sound, &QSpatialAudioSoundSource::directivityOrderChanged, this, &QQuick3DSpatialAudioSoundSource::directivityOrderChanged);
    connect(m_sound, &QSpatialAudioSoundSource::nearFieldGainChanged, this, &QQuick3DSpatialAudioSoundSource::nearFieldGainChanged);
}

QQuick3DSpatialAudioSoundSource::~QQuick3DSpatialAudioSoundSource()
{
    delete m_sound;
}

QUrl QQuick3DSpatialAudioSoundSource::source() const
{
    return m_sound->source();
}

void QQuick3DSpatialAudioSoundSource::setSource(QUrl source)
{
    QUrl url = QUrl::fromLocalFile(QDir::currentPath() + u"/");
    url = url.resolved(source);

    m_sound->setSource(url);
}

void QQuick3DSpatialAudioSoundSource::setVolume(float volume)
{
    m_sound->setVolume(volume);
}

float QQuick3DSpatialAudioSoundSource::volume() const
{
    return m_sound->volume();
}

void QQuick3DSpatialAudioSoundSource::setDistanceModel(DistanceModel model)
{
    m_sound->setDistanceModel(QSpatialAudioSoundSource::DistanceModel(model));
}

QQuick3DSpatialAudioSoundSource::DistanceModel QQuick3DSpatialAudioSoundSource::distanceModel() const
{
    return DistanceModel(m_sound->distanceModel());
}

void QQuick3DSpatialAudioSoundSource::setMinimumDistance(float min)
{
    m_sound->setMinimumDistance(min);
}

float QQuick3DSpatialAudioSoundSource::minimumDistance() const
{
    return m_sound->minimumDistance();
}

void QQuick3DSpatialAudioSoundSource::setMaximumDistance(float max)
{
    m_sound->setMaximumDistance(max);
}

float QQuick3DSpatialAudioSoundSource::maximumDistance() const
{
    return m_sound->maximumDistance();
}

void QQuick3DSpatialAudioSoundSource::setManualAttenuation(float attenuation)
{
    m_sound->setManualAttenuation(attenuation);
}

float QQuick3DSpatialAudioSoundSource::manualAttenuation() const
{
    return m_sound->manualAttenuation();
}

void QQuick3DSpatialAudioSoundSource::setOcclusionIntensity(float occlusion)
{
    m_sound->setOcclusionIntensity(occlusion);
}

float QQuick3DSpatialAudioSoundSource::occlusionIntensity() const
{
    return m_sound->occlusionIntensity();
}

void QQuick3DSpatialAudioSoundSource::setDirectivity(float alpha)
{
    m_sound->setDirectivity(alpha);
}

float QQuick3DSpatialAudioSoundSource::directivity() const
{
    return m_sound->directivity();
}

void QQuick3DSpatialAudioSoundSource::setDirectivityOrder(float alpha)
{
    m_sound->setDirectivityOrder(alpha);
}

float QQuick3DSpatialAudioSoundSource::directivityOrder() const
{
    return m_sound->directivityOrder();
}

void QQuick3DSpatialAudioSoundSource::setNearFieldGain(float gain)
{
    m_sound->setNearFieldGain(gain);
}

float QQuick3DSpatialAudioSoundSource::nearFieldGain() const
{
    return m_sound->nearFieldGain();
}

void QQuick3DSpatialAudioSoundSource::updatePosition()
{
    m_sound->setPosition(scenePosition());
}

void QQuick3DSpatialAudioSoundSource::updateRotation()
{
    m_sound->setRotation(sceneRotation());
}

QT_END_NAMESPACE
