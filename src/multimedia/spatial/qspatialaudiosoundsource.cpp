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
#include "qspatialaudiosoundsource_p.h"
#include "qspatialaudiolistener.h"
#include "qspatialaudioengine_p.h"
#include "api/resonance_audio_api.h"
#include <qaudiosink.h>
#include <qurl.h>
#include <qdebug.h>
#include <qaudiodecoder.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSpatialAudioSoundSource

    \brief A sound object in 3D space.

    A QSpatialAudioSoundSource represents an audible object in 3D space. You can define
    it's position and orientation in space, set the sound it is playing and define a
    volume for the object.

    The object can have different attenuation behavior, emit sound mainly in one direction
    or spherically, and behave as if occluded by some other object.
  */

/*!
    Creates a spatial sound source for \a engine. The object can be placed in
    3D space and will be louder the closer to the listener it is.
 */
QSpatialAudioSoundSource::QSpatialAudioSoundSource(QSpatialAudioEngine *engine)
    : d(new QSpatialAudioSoundSourcePrivate(this))
{
    setEngine(engine);
}

/*!
    Destroys the sound source.
 */
QSpatialAudioSoundSource::~QSpatialAudioSoundSource()
{
    setEngine(nullptr);
}

/*!
    \property QSpatialAudioSoundSource::position

    Defines the position of the sound source in 3D space. All units are
    assumed to be in meters.
 */
void QSpatialAudioSoundSource::setPosition(QVector3D pos)
{
    d->pos = pos;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSourcePosition(d->sourceId, pos.x(), pos.y(), pos.z());
    emit positionChanged();
}

QVector3D QSpatialAudioSoundSource::position() const
{
    return d->pos;
}

/*!
    \property QSpatialAudioSoundSource::rotation

    Defines the orientation of the sound source in 3D space.
 */
void QSpatialAudioSoundSource::setRotation(const QQuaternion &q)
{
    d->rotation = q;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSourceRotation(d->sourceId, q.x(), q.y(), q.z(), q.scalar());
    emit rotationChanged();
}

QQuaternion QSpatialAudioSoundSource::rotation() const
{
    return d->rotation;
}

/*!
    \property QSpatialAudioSoundSource::volume

    Defines an overall volume for this sound source.
 */
void QSpatialAudioSoundSource::setVolume(float volume)
{
    if (d->volume == volume)
        return;
    d->volume = volume;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSourceVolume(d->sourceId, d->volume);
    emit volumeChanged();
}

float QSpatialAudioSoundSource::volume() const
{
    return d->volume;
}

/*!
    \enum QSpatialAudioSoundSource::DistanceModel

    Defines how the volume of the sound scales with distance to the listener.

    \value DistanceModel_Logarithmic Volume decreases logarithmically with distance.
    DistanceModel_Linear Volume decreases linearly with distance.
    DistanceModel_ManualAttenutation Attenuation is defined manually using the
    \l manualAttenuation property.
*/

/*!
    \property QSpatialAudioSoundSource::distanceModel

    Defines distance model for this sound source.

    \sa QSpatialAudioSoundSource::DistanceModel
 */
void QSpatialAudioSoundSource::setDistanceModel(DistanceModel model)
{
    if (d->distanceModel == model)
        return;
    d->distanceModel = model;

    d->updateDistanceModel();
    emit distanceModelChanged();
}

void QSpatialAudioSoundSourcePrivate::updateDistanceModel()
{
    auto *ep = QSpatialAudioEnginePrivate::get(engine);
    if (!engine || sourceId < 0)
        return;

    vraudio::DistanceRolloffModel dm = vraudio::kLogarithmic;
    switch (distanceModel) {
    case QSpatialAudioSoundSource::DistanceModel_Linear:
        dm = vraudio::kLinear;
        break;
    case QSpatialAudioSoundSource::DistanceModel_ManualAttenutation:
        dm = vraudio::kNone;
        break;
    default:
        break;
    }

    ep->api->SetSourceDistanceModel(sourceId, dm, minDistance, maxDistance);
}

QSpatialAudioSoundSource::DistanceModel QSpatialAudioSoundSource::distanceModel() const
{
    return d->distanceModel;
}

/*!
    \property QSpatialAudioSoundSource::minimumDistance

    Defines a minimum distance for the sound source. If the listener is closer to the sound
    object than the minimum distance, volume will stay constant and the sound source won't
    be localized in space.
 */
void QSpatialAudioSoundSource::setMinimumDistance(float min)
{
    if (d->minDistance == min)
        return;
    d->minDistance = min;

    d->updateDistanceModel();
    emit minimumDistanceChanged();
}

float QSpatialAudioSoundSource::minimumDistance() const
{
    return d->minDistance;
}

/*!
    \property QSpatialAudioSoundSource::maximumDistance

    Defines a maximum distance for the sound source. If the listener is further away from
    the sound object than the maximum distance it won't be audible anymore.
 */
void QSpatialAudioSoundSource::setMaximumDistance(float max)
{
    if (d->maxDistance == max)
        return;
    d->maxDistance = max;

    d->updateDistanceModel();
    emit maximumDistanceChanged();
}

float QSpatialAudioSoundSource::maximumDistance() const
{
    return d->maxDistance;
}

/*!
    \property QSpatialAudioSoundSource::manualAttenuation

    Defines a manual attenuation factor if \l distanceModel is set to
    QSpatialAudioSoundSource::DistanceModel_ManualAttenutation.
 */
void QSpatialAudioSoundSource::setManualAttenuation(float attenuation)
{
    if (d->manualAttenuation == attenuation)
        return;
    d->manualAttenuation = attenuation;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSourceDistanceAttenuation(d->sourceId, d->manualAttenuation);
    emit manualAttenuationChanged();
}

float QSpatialAudioSoundSource::manualAttenuation() const
{
    return d->manualAttenuation;
}

/*!
    \property QSpatialAudioSoundSource::occlusionIntensity

    Defines how much the object is occluded. 0 implies the object is
    not occluded at all, while a large number implies a large occlusion.

    The default is 0.
 */
void QSpatialAudioSoundSource::setOcclusionIntensity(float occlusion)
{
    if (d->occlusionIntensity == occlusion)
        return;
    d->occlusionIntensity = occlusion;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSoundObjectOcclusionIntensity(d->sourceId, d->occlusionIntensity);
    emit occlusionIntensityChanged();
}

float QSpatialAudioSoundSource::occlusionIntensity() const
{
    return d->occlusionIntensity;
}

/*!
    \property QSpatialAudioSoundSource::directivity

    Defines the directivity of the sound source. A value of 0 implies that the sound is
    emitted equally in all directions, while a value of 1 implies that the source mainly
    emits sound in the forward direction.

    Valid values are between 0 and 1, the default is 0.
 */
void QSpatialAudioSoundSource::setDirectivity(float alpha)
{
    alpha = qBound(0., alpha, 1.);
    if (alpha == d->directivity)
        return;
    d->directivity = alpha;

    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSoundObjectDirectivity(d->sourceId, d->directivity, d->directivityOrder);

    emit directivityChanged();
}

float QSpatialAudioSoundSource::directivity() const
{
    return d->directivity;
}

/*!
    \property QSpatialAudioSoundSource::directivityOrder

    Defines the order of the directivity of the sound source. A higher order
    implies a sharper localization of the sound cone.

    The minimum value and default for this property is 1.
 */
void QSpatialAudioSoundSource::setDirectivityOrder(float order)
{
    order = qMax(order, 1.);
    if (order == d->directivityOrder)
        return;
    d->directivityOrder = order;

    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSoundObjectDirectivity(d->sourceId, d->directivity, d->directivityOrder);

    emit directivityChanged();
}

float QSpatialAudioSoundSource::directivityOrder() const
{
    return d->directivityOrder;
}

/*!
    \property QSpatialAudioSoundSource::nearFieldGain

    Defines the near field gain for the sound source. Valid values are between 0 and 1.
    A near field gain of 1 will raise the volume of the sound signal by approx 20 dB for
    distances very close to the listener.
 */
void QSpatialAudioSoundSource::setNearFieldGain(float gain)
{
    gain = qBound(0., gain, 1.);
    if (gain == d->nearFieldGain)
        return;
    d->nearFieldGain = gain;

    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSoundObjectNearFieldEffectGain(d->sourceId, d->nearFieldGain/9.);

    emit nearFieldGainChanged();

}

float QSpatialAudioSoundSource::nearFieldGain() const
{
    return d->nearFieldGain;
}

/*!
    \property QSpatialAudioSoundSource::source

    The source file for the sound to be played.
 */
void QSpatialAudioSoundSource::setSource(const QUrl &url)
{
    if (d->url == url)
        return;
    d->url = url;

    d->load();
    emit sourceChanged();
}

QUrl QSpatialAudioSoundSource::source() const
{
    return d->url;
}

/*!
    \internal
 */
void QSpatialAudioSoundSource::setEngine(QSpatialAudioEngine *engine)
{
    if (d->engine == engine)
        return;
    auto *ep = QSpatialAudioEnginePrivate::get(engine);

    if (ep)
        ep->removeSpatialSound(this);
    d->engine = engine;

    ep = QSpatialAudioEnginePrivate::get(engine);
    if (ep) {
        ep->addSpatialSound(this);
        ep->api->SetSourcePosition(d->sourceId, d->pos.x(), d->pos.y(), d->pos.z());
        ep->api->SetSourceRotation(d->sourceId, d->rotation.x(), d->rotation.y(), d->rotation.z(), d->rotation.scalar());
        ep->api->SetSourceVolume(d->sourceId, d->volume);
        ep->api->SetSoundObjectDirectivity(d->sourceId, d->directivity, d->directivityOrder);
        ep->api->SetSoundObjectNearFieldEffectGain(d->sourceId, d->nearFieldGain);
        d->updateDistanceModel();
    }
}

/*!
    Returns the engine associated with this listener.
 */
QSpatialAudioEngine *QSpatialAudioSoundSource::engine() const
{
    return d->engine;
}

QT_END_NAMESPACE

#include "moc_qspatialaudiosoundsource.cpp"
