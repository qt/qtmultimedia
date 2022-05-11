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
#include "qspatialaudioroom_p.h"
#include "qspatialaudiosoundsource_p.h"
#include "qspatialaudiolistener.h"
#include "qspatialaudioengine_p.h"
#include "qspatialaudioroom.h"
#include "api/resonance_audio_api.h"
#include <qaudiosink.h>
#include <qurl.h>
#include <qdebug.h>
#include <qaudiodecoder.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSpatialAudioSoundSource
    \inmodule QtMultimedia
    \ingroup multimedia_spatialaudio

    \brief A sound object in 3D space.

    QSpatialAudioSoundSource represents an audible object in 3D space. You can define
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
    d->updateRoomEffects();
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

    Defines the volume of the sound.

    Values between 0 and 1 will attenuate the sound, while values above 1
    provide an additional gain boost.
 */
void QSpatialAudioSoundSource::setVolume(float volume)
{
    if (d->volume == volume)
        return;
    d->volume = volume;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSourceVolume(d->sourceId, d->volume*d->wallDampening);
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
    \value DistanceModel_Linear Volume decreases linearly with distance.
    \value DistanceModel_ManualAttenutation Attenuation is defined manually using the
    \l manualAttenuation property.
*/

/*!
    \property QSpatialAudioSoundSource::distanceModel

    Defines distance model for this sound source. The volume starts scaling down
    from \l size to \l distanceCutoff. The volume is constant for distances smaller
    than size and zero for distances larger than the cutoff distance.

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
    if (!engine || sourceId < 0)
        return;
    auto *ep = QSpatialAudioEnginePrivate::get(engine);

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

    ep->api->SetSourceDistanceModel(sourceId, dm, size, distanceCutoff);
}

void QSpatialAudioSoundSourcePrivate::updateRoomEffects()
{
    if (!engine || sourceId < 0)
        return;
    auto *ep = QSpatialAudioEnginePrivate::get(engine);
    if (!ep->currentRoom)
        return;
    auto *rp = QSpatialAudioRoomPrivate::get(ep->currentRoom);

    QVector3D roomDim2 = ep->currentRoom->dimensions()/2.;
    QVector3D roomPos = ep->currentRoom->position();
    QQuaternion roomRot = ep->currentRoom->rotation();
    QVector3D dist = pos - roomPos;
    // transform into room coordinates
    dist = roomRot.rotatedVector(dist);
    if (qAbs(dist.x()) <= roomDim2.x() &&
        qAbs(dist.y()) <= roomDim2.y() &&
        qAbs(dist.z()) <= roomDim2.z()) {
        // Source is inside room, apply
        ep->api->SetSourceRoomEffectsGain(sourceId, 1);
        wallDampening = 1.;
        wallOcclusion = 0.;
    } else {
        // ### calculate room occlusion and dampening
        // This is a bit of heuristics on top of the heuristic dampening/occlusion numbers for walls
        //
        // We basically cast a ray from the listener through the walls. If walls have different characteristics
        // and we get close to a corner, we try to use some averaging to avoid abrupt changes
        auto relativeListenerPos = ep->listenerPosition() - roomPos;
        relativeListenerPos = roomRot.rotatedVector(relativeListenerPos);

        auto direction = dist.normalized();
        enum {
            X, Y, Z
        };
        // Very rough approximation, use the size of the source plus twice the size of our head.
        // One could probably improve upon this.
        const float transitionDistance = size + 0.4;
        QSpatialAudioRoom::Wall walls[3];
        walls[X] = direction.x() > 0 ? QSpatialAudioRoom::RightWall : QSpatialAudioRoom::LeftWall;
        walls[Y] = direction.y() > 0 ? QSpatialAudioRoom::FrontWall : QSpatialAudioRoom::BackWall;
        walls[Z] = direction.z() > 0 ? QSpatialAudioRoom::Ceiling : QSpatialAudioRoom::Floor;
        float factors[3] = { 0., 0., 0. };
        bool foundWall = false;
        if (direction.x() != 0) {
            float sign = direction.x() > 0 ? 1.f : -1.f;
            float dx = sign * roomDim2.x() - relativeListenerPos.x();
            QVector3D intersection = relativeListenerPos + direction*dx/direction.x();
            float dy = roomDim2.y() - qAbs(intersection.y());
            float dz = roomDim2.z() - qAbs(intersection.z());
            if (dy > 0 && dz > 0) {
//                qDebug() << "Hit with wall X" << walls[0] << dy << dz;
                // Ray is hitting this wall
                factors[Y] = qMax(0.f, 1.f/3.f - dy/transitionDistance);
                factors[Z] = qMax(0.f, 1.f/3.f - dz/transitionDistance);
                factors[X] = 1.f - factors[Y] - factors[Z];
                foundWall = true;
            }
        }
        if (!foundWall && direction.y() != 0) {
            float sign = direction.y() > 0 ? 1.f : -1.f;
            float dy = sign * roomDim2.y() - relativeListenerPos.y();
            QVector3D intersection = relativeListenerPos + direction*dy/direction.y();
            float dx = roomDim2.x() - qAbs(intersection.x());
            float dz = roomDim2.z() - qAbs(intersection.z());
            if (dx > 0 && dz > 0) {
                // Ray is hitting this wall
//                qDebug() << "Hit with wall Y" << walls[1] << dx << dy;
                factors[X] = qMax(0.f, 1.f/3.f - dx/transitionDistance);
                factors[Z] = qMax(0.f, 1.f/3.f - dz/transitionDistance);
                factors[Y] = 1.f - factors[X] - factors[Z];
                foundWall = true;
            }
        }
        if (!foundWall) {
            Q_ASSERT(direction.z() != 0);
            float sign = direction.z() > 0 ? 1.f : -1.f;
            float dz = sign * roomDim2.z() - relativeListenerPos.z();
            QVector3D intersection = relativeListenerPos + direction*dz/direction.z();
            float dx = roomDim2.x() - qAbs(intersection.x());
            float dy = roomDim2.y() - qAbs(intersection.y());
            if (dx > 0 && dy > 0) {
                // Ray is hitting this wall
//                qDebug() << "Hit with wall Z" << walls[2];
                factors[X] = qMax(0.f, 1.f/3.f - dx/transitionDistance);
                factors[Y] = qMax(0.f, 1.f/3.f - dy/transitionDistance);
                factors[Z] = 1.f - factors[X] - factors[Y];
                foundWall = true;
            }
        }
        wallDampening = 0;
        wallOcclusion = 0;
        for (int i = 0; i < 3; ++i) {
            wallDampening += factors[i]*rp->wallDampening(walls[i]);
            wallOcclusion += factors[i]*rp->wallOcclusion(walls[i]);
        }

//        qDebug() << "intersection with wall" << walls[0] << walls[1] << walls[2] << factors[0] << factors[1] << factors[2] << wallDampening << wallOcclusion;
        ep->api->SetSourceRoomEffectsGain(sourceId, 0);
    }
    ep->api->SetSoundObjectOcclusionIntensity(sourceId, occlusionIntensity + wallOcclusion);
    ep->api->SetSourceVolume(sourceId, volume*wallDampening);
}

QSpatialAudioSoundSource::DistanceModel QSpatialAudioSoundSource::distanceModel() const
{
    return d->distanceModel;
}

/*!
    \property QSpatialAudioSoundSource::size

    Defines the size of the sound source. If the listener is closer to the sound
    object than the size, volume will stay constant. The size is also used to for
    occlusion calculations, where large sources can be partially occluded by a wall.
 */
void QSpatialAudioSoundSource::setSize(float min)
{
    if (d->size == min)
        return;
    d->size = min;

    d->updateDistanceModel();
    emit sizeChanged();
}

float QSpatialAudioSoundSource::size() const
{
    return d->size;
}

/*!
    \property QSpatialAudioSoundSource::distanceCutoff

    Defines a distance beyond which sound coming from the source will cutoff.
    If the listener is further away from the sound object than the cutoff
    distance it won't be audible anymore.
 */
void QSpatialAudioSoundSource::setDistanceCutoff(float max)
{
    if (d->distanceCutoff == max)
        return;
    d->distanceCutoff = max;

    d->updateDistanceModel();
    emit distanceCutoffChanged();
}

float QSpatialAudioSoundSource::distanceCutoff() const
{
    return d->distanceCutoff;
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
    not occluded at all, 1 implies the sound source is fully occluded by
    another object.

    A fully occluded object will still be audible, but especially higher
    frequencies will be dampened. In addition, the object will still
    participate in generating reverb and reflections in the room.

    Values larger than 1 are possible to further dampen the direct
    sound coming from the source.

    The default is 0.
 */
void QSpatialAudioSoundSource::setOcclusionIntensity(float occlusion)
{
    if (d->occlusionIntensity == occlusion)
        return;
    d->occlusionIntensity = occlusion;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSoundObjectOcclusionIntensity(d->sourceId, d->occlusionIntensity + d->wallOcclusion);
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
   \property QSpatialAudioSoundSource::loops

    Determines how many times the sound is played before the player stops.
    Set to QSpatialAudioSoundSource::Infinite to play the current sound in a loop forever.

    The default value is \c 1.
 */
int QSpatialAudioSoundSource::loops() const
{
    return d->m_loops.loadRelaxed();
}

void QSpatialAudioSoundSource::setLoops(int loops)
{
    int oldLoops = d->m_loops.fetchAndStoreRelaxed(loops);
    if (oldLoops != loops)
        emit loopsChanged();
}

/*!
   \property QSpatialAudioSoundSource::autoPlay

    Determines whether the sound should automatically start playing when a source
    gets specified.

    The default value is \c true.
 */
bool QSpatialAudioSoundSource::autoPlay() const
{
    return d->m_autoPlay.loadRelaxed();
}

void QSpatialAudioSoundSource::setAutoPlay(bool autoPlay)
{
    bool old = d->m_autoPlay.fetchAndStoreRelaxed(autoPlay);
    if (old != autoPlay)
        emit autoPlayChanged();
}

/*!
    Starts playing back the sound. Does nothing if the sound is already playing.
 */
void QSpatialAudioSoundSource::play()
{
    d->play();
}

/*!
    Pauses sound playback. Calling play() will continue playback.
 */
void QSpatialAudioSoundSource::pause()
{
    d->pause();
}

/*!
    Stops sound playback and resets the current position and current loop count to 0.
    Calling play() will start playback at the beginning of the sound file.
 */
void QSpatialAudioSoundSource::stop()
{
    d->stop();
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
