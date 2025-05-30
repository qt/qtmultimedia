// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#include "qquick3dspatialsound_p.h"
#include "qquick3daudioengine_p.h"
#include "qspatialsound.h"
#include <QAudioFormat>
#include <qdir.h>
#include <QQmlContext>
#include <QQmlFile>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SpatialSound
    \inqmlmodule QtQuick3D.SpatialAudio
    \ingroup quick3d_spatialaudio
    \ingroup multimedia_audio_qml

    \brief A sound object in 3D space.

    A SpatialSound represents an audible object in 3D space. You can define
    it's position and orientation in space, set the sound it is playing and define a
    volume for the object.

    The object can have different attenuation behavior, emit sound mainly in one direction
    or spherically, and behave as if occluded by some other object.
  */

QQuick3DSpatialSound::QQuick3DSpatialSound()
{
    m_sound = new QSpatialSound(QQuick3DAudioEngine::getEngine());

    connect(this, &QQuick3DNode::scenePositionChanged, this, &QQuick3DSpatialSound::updatePosition);
    connect(this, &QQuick3DNode::sceneRotationChanged, this, &QQuick3DSpatialSound::updateRotation);
    connect(m_sound, &QSpatialSound::sourceChanged, this, &QQuick3DSpatialSound::sourceChanged);
    connect(m_sound, &QSpatialSound::volumeChanged, this, &QQuick3DSpatialSound::volumeChanged);
    connect(m_sound, &QSpatialSound::distanceModelChanged, this, &QQuick3DSpatialSound::distanceModelChanged);
    connect(m_sound, &QSpatialSound::sizeChanged, this, &QQuick3DSpatialSound::sizeChanged);
    connect(m_sound, &QSpatialSound::distanceCutoffChanged, this, &QQuick3DSpatialSound::distanceCutoffChanged);
    connect(m_sound, &QSpatialSound::manualAttenuationChanged, this, &QQuick3DSpatialSound::manualAttenuationChanged);
    connect(m_sound, &QSpatialSound::occlusionIntensityChanged, this, &QQuick3DSpatialSound::occlusionIntensityChanged);
    connect(m_sound, &QSpatialSound::directivityChanged, this, &QQuick3DSpatialSound::directivityChanged);
    connect(m_sound, &QSpatialSound::directivityOrderChanged, this, &QQuick3DSpatialSound::directivityOrderChanged);
    connect(m_sound, &QSpatialSound::nearFieldGainChanged, this, &QQuick3DSpatialSound::nearFieldGainChanged);
    connect(m_sound, &QSpatialSound::loopsChanged, this, &QQuick3DSpatialSound::loopsChanged);
    connect(m_sound, &QSpatialSound::autoPlayChanged, this, &QQuick3DSpatialSound::autoPlayChanged);
}

QQuick3DSpatialSound::~QQuick3DSpatialSound()
{
    delete m_sound;
}

/*!
    \qmlproperty url SpatialSound::source

    The source file for the sound to be played.
 */
QUrl QQuick3DSpatialSound::source() const
{
    return m_sound->source();
}

void QQuick3DSpatialSound::setSource(QUrl source)
{
    const QQmlContext *context = qmlContext(this);
    QUrl url;
    if (context) {
        url = context->resolvedUrl(source);
    } else {
        url = QUrl::fromLocalFile(QDir::currentPath() + u"/");
        url = url.resolved(source);
    }
    m_sound->setSource(url);
}

/*!
    \qmlproperty float SpatialSound::volume

    Defines an overall volume for this sound source.

    Values between 0 and 1 will attenuate the sound, while values above 1
    provide an additional gain boost.
 */
void QQuick3DSpatialSound::setVolume(float volume)
{
    m_sound->setVolume(volume);
}

float QQuick3DSpatialSound::volume() const
{
    return m_sound->volume();
}

/*!
    \qmlproperty enumeration SpatialSound::distanceModel

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
    \row \li ManualAttenuation
        \li Attenuation is defined manually using the \l manualAttenuation property.
    \endtable
 */
void QQuick3DSpatialSound::setDistanceModel(DistanceModel model)
{
    m_sound->setDistanceModel(QSpatialSound::DistanceModel(model));
}

QQuick3DSpatialSound::DistanceModel QQuick3DSpatialSound::distanceModel() const
{
    return DistanceModel(m_sound->distanceModel());
}

/*!
    \qmlproperty float SpatialSound::size

    Defines the size of the sound source. If the listener is closer to the sound
    object than the size, volume will stay constant. The size is also used to for
    occlusion calculations, where large sources can be partially occluded by a wall.
 */
void QQuick3DSpatialSound::setSize(float min)
{
    m_sound->setSize(min);
}

float QQuick3DSpatialSound::size() const
{
    return m_sound->size();
}

/*!
    \qmlproperty float SpatialSound::distanceCutoff

    Defines a distance beyond which sound coming from the source will cutoff.
    If the listener is further away from the sound object than the cutoff
    distance it won't be audible anymore.
 */
void QQuick3DSpatialSound::setDistanceCutoff(float max)
{
    m_sound->setDistanceCutoff(max);
}

float QQuick3DSpatialSound::distanceCutoff() const
{
    return m_sound->distanceCutoff();
}

/*!
    \qmlproperty float SpatialSound::manualAttenuation

    Defines a manual attenuation factor if \l distanceModel is set to
    SpatialSound.ManualAttenuation.
 */
void QQuick3DSpatialSound::setManualAttenuation(float attenuation)
{
    m_sound->setManualAttenuation(attenuation);
}

float QQuick3DSpatialSound::manualAttenuation() const
{
    return m_sound->manualAttenuation();
}

/*!
    \qmlproperty float SpatialSound::occlusionIntensity

    Defines how much the object is occluded. 0 implies the object is
    not occluded at all, while a large number implies a large occlusion.

    The default is 0.
 */
void QQuick3DSpatialSound::setOcclusionIntensity(float occlusion)
{
    m_sound->setOcclusionIntensity(occlusion);
}

float QQuick3DSpatialSound::occlusionIntensity() const
{
    return m_sound->occlusionIntensity();
}

/*!
    \qmlproperty float SpatialSound::directivity

    Defines the directivity of the sound source. A value of 0 implies that the sound is
    emitted equally in all directions, while a value of 1 implies that the source mainly
    emits sound in the forward direction.

    Valid values are between 0 and 1, the default is 0.
 */
void QQuick3DSpatialSound::setDirectivity(float alpha)
{
    m_sound->setDirectivity(alpha);
}

float QQuick3DSpatialSound::directivity() const
{
    return m_sound->directivity();
}

/*!
    \qmlproperty float SpatialSound::directivityOrder

    Defines the order of the directivity of the sound source. A higher order
    implies a sharper localization of the sound cone.

    The minimum value and default for this property is 1.
 */
void QQuick3DSpatialSound::setDirectivityOrder(float alpha)
{
    m_sound->setDirectivityOrder(alpha);
}

float QQuick3DSpatialSound::directivityOrder() const
{
    return m_sound->directivityOrder();
}

/*!
    \qmlproperty float SpatialSound::nearFieldGain

    Defines the near field gain for the sound source. Valid values are between 0 and 1.
    A near field gain of 1 will raise the volume of the sound signal by approx 20 dB for
    distances very close to the listener.
 */
void QQuick3DSpatialSound::setNearFieldGain(float gain)
{
    m_sound->setNearFieldGain(gain);
}

float QQuick3DSpatialSound::nearFieldGain() const
{
    return m_sound->nearFieldGain();
}

void QQuick3DSpatialSound::updatePosition()
{
    m_sound->setPosition(scenePosition());
}

void QQuick3DSpatialSound::updateRotation()
{
    m_sound->setRotation(sceneRotation());
}

/*!
   \qmlproperty int SpatialSound::loops

    Determines how often the sound is played before the player stops.
    Set to SpatialSound::Infinite to loop the current sound forever.

    The default value is \c 1.
 */
int QQuick3DSpatialSound::loops() const
{
    return m_sound->loops();
}

void QQuick3DSpatialSound::setLoops(int loops)
{
    m_sound->setLoops(loops);
}

/*!
   \qmlproperty bool SpatialSound::autoPlay

    Determines whether the sound should automatically start playing when a source
    gets specified.

    The default value is \c true.
 */
bool QQuick3DSpatialSound::autoPlay() const
{
    return m_sound->autoPlay();
}

void QQuick3DSpatialSound::setAutoPlay(bool autoPlay)
{
    m_sound->setAutoPlay(autoPlay);
}

/*!
    \qmlmethod SpatialSound::play()

    Starts playing back the sound. Does nothing if the sound is already playing.
 */
void QQuick3DSpatialSound::play()
{
    m_sound->play();
}

/*!
    \qmlmethod SpatialSound::pause()

    Pauses sound playback at the current position. Calling play() will continue playback.
 */
void QQuick3DSpatialSound::pause()
{
    m_sound->pause();
}

/*!
    \qmlmethod SpatialSound::stop()

    Stops sound playback and resets the current position and loop count to 0. Calling play() will
    begin playback at the beginning of the sound file.
 */
void QQuick3DSpatialSound::stop()
{
    m_sound->stop();
}

QT_END_NAMESPACE
