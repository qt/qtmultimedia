// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#include "qaudioroom_p.h"
#include "qspatialsound_p.h"
#include "qaudiolistener.h"
#include "qaudioengine_p.h"
#include "resonance_audio.h"
#include <qaudiosink.h>
#include <qurl.h>
#include <qdebug.h>
#include <qaudiodecoder.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSpatialSound
    \inmodule QtSpatialAudio
    \ingroup spatialaudio
    \ingroup multimedia_audio

    \brief A sound object in 3D space.

    QSpatialSound represents an audible object in 3D space. You can define
    its position and orientation in space, set the sound it is playing and define a
    volume for the object.

    The object can have different attenuation behavior, emit sound mainly in one direction
    or spherically, and behave as if occluded by some other object.
  */

/*!
    Creates a spatial sound source for \a engine. The object can be placed in
    3D space and will be louder the closer to the listener it is.
 */
QSpatialSound::QSpatialSound(QAudioEngine *engine) : QObject(*new QSpatialSoundPrivate)
{
    setEngine(engine);
}

/*!
    Destroys the sound source.
 */
QSpatialSound::~QSpatialSound()
{
    setEngine(nullptr);
}

/*!
    \property QSpatialSound::position

    Defines the position of the sound source in 3D space. Units are in centimeters
    by default.

    \sa QAudioEngine::distanceScale
 */
void QSpatialSound::setPosition(QVector3D pos)
{
    Q_D(QSpatialSound);
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (!ep)
        return;

    pos *= ep->distanceScale;
    d->pos = pos;
    ep->resonanceAudio->api->SetSourcePosition(d->sourceId, pos.x(), pos.y(), pos.z());
    emit positionChanged();
}

QVector3D QSpatialSound::position() const
{
    Q_D(const QSpatialSound);
    auto *ep = QAudioEnginePrivate::get(d->engine);
    return d->pos/ep->distanceScale;
}

/*!
    \property QSpatialSound::rotation

    Defines the orientation of the sound source in 3D space.
 */
void QSpatialSound::setRotation(const QQuaternion &q)
{
    Q_D(QSpatialSound);

    d->rotation = q;
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->resonanceAudio->api->SetSourceRotation(d->sourceId, q.x(), q.y(), q.z(), q.scalar());
    emit rotationChanged();
}

QQuaternion QSpatialSound::rotation() const
{
    Q_D(const QSpatialSound);
    return d->rotation;
}

/*!
    \property QSpatialSound::volume

    Defines the volume of the sound.

    Values between 0 and 1 will attenuate the sound, while values above 1
    provide an additional gain boost.
 */
void QSpatialSound::setVolume(float volume)
{
    Q_D(QSpatialSound);
    if (d->volume == volume)
        return;
    d->volume = volume;
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->resonanceAudio->api->SetSourceVolume(d->sourceId, d->volume*d->wallDampening);
    emit volumeChanged();
}

float QSpatialSound::volume() const
{
    Q_D(const QSpatialSound);
    return d->volume;
}

/*!
    \enum QSpatialSound::DistanceModel

    Defines how the volume of the sound scales with distance to the listener.

    \value Logarithmic Volume decreases logarithmically with distance.
    \value Linear Volume decreases linearly with distance.
    \value ManualAttenuation Attenuation is defined manually using the
    \l manualAttenuation property.
*/

/*!
    \property QSpatialSound::distanceModel

    Defines distance model for this sound source. The volume starts scaling down
    from \l size to \l distanceCutoff. The volume is constant for distances smaller
    than size and zero for distances larger than the cutoff distance.

    \sa QSpatialSound::DistanceModel
 */
void QSpatialSound::setDistanceModel(DistanceModel model)
{
    Q_D(QSpatialSound);

    if (d->distanceModel == model)
        return;
    d->distanceModel = model;

    d->updateDistanceModel();
    emit distanceModelChanged();
}

void QSpatialSoundPrivate::updateDistanceModel()
{
    if (!engine || sourceId < 0)
        return;
    auto *ep = QAudioEnginePrivate::get(engine);

    vraudio::DistanceRolloffModel dm = vraudio::kLogarithmic;
    switch (distanceModel) {
    case QSpatialSound::DistanceModel::Linear:
        dm = vraudio::kLinear;
        break;
    case QSpatialSound::DistanceModel::ManualAttenuation:
        dm = vraudio::kNone;
        break;
    default:
        break;
    }

    ep->resonanceAudio->api->SetSourceDistanceModel(sourceId, dm, size, distanceCutoff);
}

void QSpatialSoundPrivate::updateRoomEffects()
{
    if (!engine || sourceId < 0)
        return;
    auto *ep = QAudioEnginePrivate::get(engine);
    if (!ep->currentRoom)
        return;
    auto *rp = QAudioRoomPrivate::get(ep->currentRoom);
    if (!rp)
        return;

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
        ep->resonanceAudio->api->SetSourceRoomEffectsGain(sourceId, 1);
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
        QAudioRoom::Wall walls[3];
        walls[X] = direction.x() > 0 ? QAudioRoom::RightWall : QAudioRoom::LeftWall;
        walls[Y] = direction.y() > 0 ? QAudioRoom::FrontWall : QAudioRoom::BackWall;
        walls[Z] = direction.z() > 0 ? QAudioRoom::Ceiling : QAudioRoom::Floor;
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
        ep->resonanceAudio->api->SetSourceRoomEffectsGain(sourceId, 0);
    }
    ep->resonanceAudio->api->SetSoundObjectOcclusionIntensity(sourceId, occlusionIntensity + wallOcclusion);
    ep->resonanceAudio->api->SetSourceVolume(sourceId, volume*wallDampening);
}

QSpatialSound::DistanceModel QSpatialSound::distanceModel() const
{
    Q_D(const QSpatialSound);
    return d->distanceModel;
}

/*!
    \property QSpatialSound::size

    Defines the size of the sound source. If the listener is closer to the sound
    object than the size, volume will stay constant. The size is also used to for
    occlusion calculations, where large sources can be partially occluded by a wall.
 */
void QSpatialSound::setSize(float size)
{
    Q_D(QSpatialSound);
    auto *ep = QAudioEnginePrivate::get(d->engine);
    size *= ep->distanceScale;
    if (d->size == size)
        return;
    d->size = size;

    d->updateDistanceModel();
    emit sizeChanged();
}

float QSpatialSound::size() const
{
    Q_D(const QSpatialSound);
    auto *ep = QAudioEnginePrivate::get(d->engine);
    return d->size/ep->distanceScale;
}

/*!
    \property QSpatialSound::distanceCutoff

    Defines a distance beyond which sound coming from the source will cutoff.
    If the listener is further away from the sound object than the cutoff
    distance it won't be audible anymore.
 */
void QSpatialSound::setDistanceCutoff(float cutoff)
{
    Q_D(QSpatialSound);
    auto *ep = QAudioEnginePrivate::get(d->engine);
    cutoff *= ep->distanceScale;
    if (d->distanceCutoff == cutoff)
        return;
    d->distanceCutoff = cutoff;

    d->updateDistanceModel();
    emit distanceCutoffChanged();
}

float QSpatialSound::distanceCutoff() const
{
    Q_D(const QSpatialSound);
    auto *ep = QAudioEnginePrivate::get(d->engine);
    return d->distanceCutoff/ep->distanceScale;
}

/*!
    \property QSpatialSound::manualAttenuation

    Defines a manual attenuation factor if \l distanceModel is set to
    QSpatialSound::DistanceModel::ManualAttenuation.
 */
void QSpatialSound::setManualAttenuation(float attenuation)
{
    Q_D(QSpatialSound);
    if (d->manualAttenuation == attenuation)
        return;
    d->manualAttenuation = attenuation;
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->resonanceAudio->api->SetSourceDistanceAttenuation(d->sourceId, d->manualAttenuation);
    emit manualAttenuationChanged();
}

float QSpatialSound::manualAttenuation() const
{
    Q_D(const QSpatialSound);
    return d->manualAttenuation;
}

/*!
    \property QSpatialSound::occlusionIntensity

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
void QSpatialSound::setOcclusionIntensity(float occlusion)
{
    Q_D(QSpatialSound);

    if (d->occlusionIntensity == occlusion)
        return;
    d->occlusionIntensity = occlusion;
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->resonanceAudio->api->SetSoundObjectOcclusionIntensity(d->sourceId, d->occlusionIntensity + d->wallOcclusion);
    emit occlusionIntensityChanged();
}

float QSpatialSound::occlusionIntensity() const
{
    Q_D(const QSpatialSound);

    return d->occlusionIntensity;
}

/*!
    \property QSpatialSound::directivity

    Defines the directivity of the sound source. A value of 0 implies that the sound is
    emitted equally in all directions, while a value of 1 implies that the source mainly
    emits sound in the forward direction.

    Valid values are between 0 and 1, the default is 0.
 */
void QSpatialSound::setDirectivity(float alpha)
{
    Q_D(QSpatialSound);

    alpha = qBound(0., alpha, 1.);
    if (alpha == d->directivity)
        return;
    d->directivity = alpha;

    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->resonanceAudio->api->SetSoundObjectDirectivity(d->sourceId, d->directivity, d->directivityOrder);

    emit directivityChanged();
}

float QSpatialSound::directivity() const
{
    Q_D(const QSpatialSound);
    return d->directivity;
}

/*!
    \property QSpatialSound::directivityOrder

    Defines the order of the directivity of the sound source. A higher order
    implies a sharper localization of the sound cone.

    The minimum value and default for this property is 1.
 */
void QSpatialSound::setDirectivityOrder(float order)
{
    Q_D(QSpatialSound);

    order = qMax(order, 1.);
    if (order == d->directivityOrder)
        return;
    d->directivityOrder = order;

    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->resonanceAudio->api->SetSoundObjectDirectivity(d->sourceId, d->directivity, d->directivityOrder);

    emit directivityChanged();
}

float QSpatialSound::directivityOrder() const
{
    Q_D(const QSpatialSound);

    return d->directivityOrder;
}

/*!
    \property QSpatialSound::nearFieldGain

    Defines the near field gain for the sound source. Valid values are between 0 and 1.
    A near field gain of 1 will raise the volume of the sound signal by approx 20 dB for
    distances very close to the listener.
 */
void QSpatialSound::setNearFieldGain(float gain)
{
    Q_D(QSpatialSound);

    gain = qBound(0., gain, 1.);
    if (gain == d->nearFieldGain)
        return;
    d->nearFieldGain = gain;

    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->resonanceAudio->api->SetSoundObjectNearFieldEffectGain(d->sourceId, d->nearFieldGain*9.f);

    emit nearFieldGainChanged();

}

float QSpatialSound::nearFieldGain() const
{
    Q_D(const QSpatialSound);

    return d->nearFieldGain;
}

/*!
    \property QSpatialSound::source

    The source file for the sound to be played.
 */
void QSpatialSound::setSource(const QUrl &url)
{
    Q_D(QSpatialSound);

    if (d->url == url)
        return;
    d->url = url;

    d->load();
    emit sourceChanged();
}

QUrl QSpatialSound::source() const
{
    Q_D(const QSpatialSound);

    return d->url;
}

/*!
    \enum QSpatialSound::Loops

    Lets you control the sound playback loop using the following values:

    \value Infinite Playback infinitely
    \value Once Playback once
*/
/*!
   \property QSpatialSound::loops

    Determines how many times the sound is played before the player stops.
    Set to QSpatialSound::Infinite to play the current sound in a loop forever.

    The default value is \c 1.
 */
int QSpatialSound::loops() const
{
    Q_D(const QSpatialSound);

    return d->m_loops.loadRelaxed();
}

void QSpatialSound::setLoops(int loops)
{
    Q_D(QSpatialSound);

    int oldLoops = d->m_loops.fetchAndStoreRelaxed(loops);
    if (oldLoops != loops)
        emit loopsChanged();
}

/*!
   \property QSpatialSound::autoPlay

    Determines whether the sound should automatically start playing when a source
    gets specified.

    The default value is \c true.
 */
bool QSpatialSound::autoPlay() const
{
    Q_D(const QSpatialSound);

    return d->m_autoPlay.loadRelaxed();
}

void QSpatialSound::setAutoPlay(bool autoPlay)
{
    Q_D(QSpatialSound);
    bool old = d->m_autoPlay.fetchAndStoreRelaxed(autoPlay);
    if (old != autoPlay)
        emit autoPlayChanged();
}

/*!
    Starts playing back the sound. Does nothing if the sound is already playing.
 */
void QSpatialSound::play()
{
    Q_D(QSpatialSound);

    d->play();
}

/*!
    Pauses sound playback. Calling play() will continue playback.
 */
void QSpatialSound::pause()
{
    Q_D(QSpatialSound);

    d->pause();
}

/*!
    Stops sound playback and resets the current position and current loop count to 0.
    Calling play() will start playback at the beginning of the sound file.
 */
void QSpatialSound::stop()
{
    Q_D(QSpatialSound);

    d->stop();
}

/*!
    \internal
 */
void QSpatialSound::setEngine(QAudioEngine *engine)
{
    Q_D(QSpatialSound);

    if (d->engine == engine)
        return;

    // Remove self from old engine (if necessary)
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->removeSpatialSound(this);

    d->engine = engine;

    // Add self to new engine if necessary
    ep = QAudioEnginePrivate::get(d->engine);
    if (ep) {
        ep->addSpatialSound(this);
        ep->resonanceAudio->api->SetSourcePosition(d->sourceId, d->pos.x(), d->pos.y(), d->pos.z());
        ep->resonanceAudio->api->SetSourceRotation(d->sourceId, d->rotation.x(), d->rotation.y(), d->rotation.z(), d->rotation.scalar());
        ep->resonanceAudio->api->SetSourceVolume(d->sourceId, d->volume);
        ep->resonanceAudio->api->SetSoundObjectDirectivity(d->sourceId, d->directivity, d->directivityOrder);
        ep->resonanceAudio->api->SetSoundObjectNearFieldEffectGain(d->sourceId, d->nearFieldGain);
        d->updateDistanceModel();
    }
}

/*!
    Returns the engine associated with this listener.
 */
QAudioEngine *QSpatialSound::engine() const
{
    Q_D(const QSpatialSound);

    return d->engine;
}

QT_END_NAMESPACE

#include "moc_qspatialsound.cpp"
