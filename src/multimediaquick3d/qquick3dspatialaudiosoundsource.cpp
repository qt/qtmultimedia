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

/*!
    \qmltype SpatialAudioSoundSource
    \inqmlmodule QtQuick3D.SpatialAudio
    \ingroup quick3d_spatialaudio

    \brief A sound object in 3D space.

    A SpatialAudioSoundSource represents an audible object in 3D space. You can define
    it's position and orientation in space, set the sound it is playing and define a
    volume for the object.

    The object can have different attenuation behavior, emit sound mainly in one direction
    or spherically, and behave as if occluded by some other object.
  */

QQuick3DSpatialAudioSoundSource::QQuick3DSpatialAudioSoundSource()
{
    m_sound = new QSpatialAudioSoundSource(QQuick3DSpatialAudioEngine::getEngine());

    connect(this, &QQuick3DNode::scenePositionChanged, this, &QQuick3DSpatialAudioSoundSource::updatePosition);
    connect(this, &QQuick3DNode::sceneRotationChanged, this, &QQuick3DSpatialAudioSoundSource::updateRotation);
    connect(m_sound, &QSpatialAudioSoundSource::sourceChanged, this, &QQuick3DSpatialAudioSoundSource::sourceChanged);
    connect(m_sound, &QSpatialAudioSoundSource::volumeChanged, this, &QQuick3DSpatialAudioSoundSource::volumeChanged);
    connect(m_sound, &QSpatialAudioSoundSource::distanceModelChanged, this, &QQuick3DSpatialAudioSoundSource::distanceModelChanged);
    connect(m_sound, &QSpatialAudioSoundSource::sizeChanged, this, &QQuick3DSpatialAudioSoundSource::sizeChanged);
    connect(m_sound, &QSpatialAudioSoundSource::distanceCutoffChanged, this, &QQuick3DSpatialAudioSoundSource::distanceCutoffChanged);
    connect(m_sound, &QSpatialAudioSoundSource::manualAttenuationChanged, this, &QQuick3DSpatialAudioSoundSource::manualAttenuationChanged);
    connect(m_sound, &QSpatialAudioSoundSource::occlusionIntensityChanged, this, &QQuick3DSpatialAudioSoundSource::occlusionIntensityChanged);
    connect(m_sound, &QSpatialAudioSoundSource::directivityChanged, this, &QQuick3DSpatialAudioSoundSource::directivityChanged);
    connect(m_sound, &QSpatialAudioSoundSource::directivityOrderChanged, this, &QQuick3DSpatialAudioSoundSource::directivityOrderChanged);
    connect(m_sound, &QSpatialAudioSoundSource::nearFieldGainChanged, this, &QQuick3DSpatialAudioSoundSource::nearFieldGainChanged);
    connect(m_sound, &QSpatialAudioSoundSource::loopsChanged, this, &QQuick3DSpatialAudioSoundSource::loopsChanged);
    connect(m_sound, &QSpatialAudioSoundSource::autoPlayChanged, this, &QQuick3DSpatialAudioSoundSource::autoPlayChanged);
}

QQuick3DSpatialAudioSoundSource::~QQuick3DSpatialAudioSoundSource()
{
    delete m_sound;
}

/*!
    \qmlproperty url QSpatialAudioSoundSource::source

    The source file for the sound to be played.
 */
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

/*!
    \qmlproperty float QSpatialAudioSoundSource::volume

    Defines an overall volume for this sound source.
 */
void QQuick3DSpatialAudioSoundSource::setVolume(float volume)
{
    m_sound->setVolume(volume);
}

float QQuick3DSpatialAudioSoundSource::volume() const
{
    return m_sound->volume();
}

/*!
    \qmlproperty enumeration SpatialAudioSoundSource::distanceModel

    Defines how the volume of the sound scales with distance to the listener.
    The volume starts scaling down
    from \l size to \l distanceCutoff. The volume is constant for distances smaller
    than size and zero for distances larger than the cutoff distance.

    \table
    \header \li Property value
            \li Description
    \row \li Logarithmic
        \li Volume decreases logarithmically with distance.
    \row \li Linear
        \li Volume decreases linearly with distance.
    \row \li ManualAttenutation
        \li Attenuation is defined manually using the \l manualAttenuation property.
    \endtable
 */
void QQuick3DSpatialAudioSoundSource::setDistanceModel(DistanceModel model)
{
    m_sound->setDistanceModel(QSpatialAudioSoundSource::DistanceModel(model));
}

QQuick3DSpatialAudioSoundSource::DistanceModel QQuick3DSpatialAudioSoundSource::distanceModel() const
{
    return DistanceModel(m_sound->distanceModel());
}

/*!
    \qmlproperty float SpatialAudioSoundSource::size

    Defines the size of the sound source. If the listener is closer to the sound
    object than the size, volume will stay constant. The size is also used to for
    occlusion calculations, where large sources can be partially occluded by a wall.
 */
void QQuick3DSpatialAudioSoundSource::setSize(float min)
{
    m_sound->setSize(min);
}

float QQuick3DSpatialAudioSoundSource::size() const
{
    return m_sound->size();
}

/*!
    \qmlproperty float SpatialAudioSoundSource::distanceCutoff

    Defines a distance beyond which sound coming from the source will cutoff.
    If the listener is further away from the sound object than the cutoff
    distance it won't be audible anymore.
 */
void QQuick3DSpatialAudioSoundSource::setDistanceCutoff(float max)
{
    m_sound->setDistanceCutoff(max);
}

float QQuick3DSpatialAudioSoundSource::distanceCutoff() const
{
    return m_sound->distanceCutoff();
}

/*!
    \qmlproperty float SpatialAudioSoundSource::manualAttenuation

    Defines a manual attenuation factor if \l distanceModel is set to
    SpatialAudioSoundSource.ManualAttenutation.
 */
void QQuick3DSpatialAudioSoundSource::setManualAttenuation(float attenuation)
{
    m_sound->setManualAttenuation(attenuation);
}

float QQuick3DSpatialAudioSoundSource::manualAttenuation() const
{
    return m_sound->manualAttenuation();
}

/*!
    \qmlproperty float SpatialAudioSoundSource::occlusionIntensity

    Defines how much the object is occluded. 0 implies the object is
    not occluded at all, while a large number implies a large occlusion.

    The default is 0.
 */
void QQuick3DSpatialAudioSoundSource::setOcclusionIntensity(float occlusion)
{
    m_sound->setOcclusionIntensity(occlusion);
}

float QQuick3DSpatialAudioSoundSource::occlusionIntensity() const
{
    return m_sound->occlusionIntensity();
}

/*!
    \qmlproperty float SpatialAudioSoundSource::directivity

    Defines the directivity of the sound source. A value of 0 implies that the sound is
    emitted equally in all directions, while a value of 1 implies that the source mainly
    emits sound in the forward direction.

    Valid values are between 0 and 1, the default is 0.
 */
void QQuick3DSpatialAudioSoundSource::setDirectivity(float alpha)
{
    m_sound->setDirectivity(alpha);
}

float QQuick3DSpatialAudioSoundSource::directivity() const
{
    return m_sound->directivity();
}

/*!
    \qmlproperty float SpatialAudioSoundSource::directivityOrder

    Defines the order of the directivity of the sound source. A higher order
    implies a sharper localization of the sound cone.

    The minimum value and default for this property is 1.
 */
void QQuick3DSpatialAudioSoundSource::setDirectivityOrder(float alpha)
{
    m_sound->setDirectivityOrder(alpha);
}

float QQuick3DSpatialAudioSoundSource::directivityOrder() const
{
    return m_sound->directivityOrder();
}

/*!
    \qmlproperty float SpatialAudioSoundSource::nearFieldGain

    Defines the near field gain for the sound source. Valid values are between 0 and 1.
    A near field gain of 1 will raise the volume of the sound signal by approx 20 dB for
    distances very close to the listener.
 */
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

/*!
   \qmlproperty int QSpatialAudioSoundSource::loops

    Determines how often the sound is played before the player stops.
    Set to SpatialAudioSoundSource::Infinite to loop the current sound forever.

    The default value is \c 1.
 */
int QQuick3DSpatialAudioSoundSource::loops() const
{
    return m_sound->loops();
}

void QQuick3DSpatialAudioSoundSource::setLoops(int loops)
{
    m_sound->setLoops(loops);
}

/*!
   \qmlproperty bool SpatialAudioSoundSource::autoPlay

    Determines whether the sound should automatically start playing when a source
    gets specified.

    The default value is \c true.
 */
bool QQuick3DSpatialAudioSoundSource::autoPlay() const
{
    return m_sound->autoPlay();
}

void QQuick3DSpatialAudioSoundSource::setAutoPlay(bool autoPlay)
{
    m_sound->setAutoPlay(autoPlay);
}

/*!
    \qmlmethod SpatialAudioSoundSource::play()

    Starts playing back the sound. Does nothing if the sound is already playing.
 */
void QQuick3DSpatialAudioSoundSource::play()
{
    m_sound->play();
}

/*!
    \qmlmethod SpatialAudioSoundSource::pause()

    Pauses sound playback at the current position. Calling play() will continue playback.
 */
void QQuick3DSpatialAudioSoundSource::pause()
{
    m_sound->pause();
}

/*!
    \qmlmethod SpatialAudioSoundSource::stop()

    Stops sound playback and resets the current position and loop count to 0. Calling play() will
    begin playback at the beginning of the sound file.
 */
void QQuick3DSpatialAudioSoundSource::stop()
{
    m_sound->stop();
}

QT_END_NAMESPACE
