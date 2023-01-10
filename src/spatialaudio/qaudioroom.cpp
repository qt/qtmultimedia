// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#include <qaudioroom_p.h>

QT_BEGIN_NAMESPACE

namespace {
inline QVector3D toVector(const float *f)
{
    return QVector3D(f[0], f[1], f[2]);
}

inline void toFloats(const QVector3D &v, float *f)
{
    f[0] = v.x();
    f[1] = v.y();
    f[2] = v.z();
}

inline QQuaternion toQuaternion(const float *f)
{
    // resonance audio puts the scalar component last
    return QQuaternion(f[3], f[0], f[1], f[2]);
}

inline void toFloats(const QQuaternion &q, float *f)
{
    f[0] = q.x();
    f[1] = q.y();
    f[2] = q.z();
    f[3] = q.scalar();
}

// Default values for occlusion and dampening of different wall materials.
// These values are used as defaults if a wall is only defined by a material
// and define how sound passes through the wall.
// We define both occlusion and dampening constants to be able to tune the
// sound. Dampening only reduces the level of the sound without affecting its
// tone, while occlusion will dampen higher frequencies more than lower ones
struct {
    float occlusion;
    float dampening;
} occlusionAndDampening[] = {
    { 0.f, 1.f }, // Transparent,
    { 0.f, .1f }, // AcousticCeilingTiles,
    { 2.f, .4f }, // BrickBare,
    { 2.f, .4f }, // BrickPainted,
    { 4.f, 1.f }, // ConcreteBlockCoarse,
    { 4.f, 1.f }, // ConcreteBlockPainted,
    { .7f, .7f }, // CurtainHeavy,
    { .5f, .5f }, // FiberGlassInsulation,
    { .2f, .3f }, // GlassThin,
    { .5f, .2f }, // GlassThick,
    { 7.f, 1.f }, // Grass,
    { 4.f, 1.f }, // LinoleumOnConcrete,
    { 4.f, 1.f }, // Marble,
    { 0.f, .2f }, // Metal,
    { 4.f, 1.f }, // ParquetOnConcrete,
    { 2.f, .4f }, // PlasterRough,
    { 2.f, .4f }, // PlasterSmooth,
    { 1.5f, .2f }, // PlywoodPanel,
    { 4.f, 1.f }, // PolishedConcreteOrTile,
    { 4.f, 1.f }, // Sheetrock,
    { 4.f, 1.f }, // WaterOrIceSurface,
    { 1.f, .3f }, // WoodCeiling,
    { 1.f, .3f }, // WoodPanel,
    { 0.f, .0f }, // UniformMaterial,
};

}

// make sure the wall definitions agree with resonance audio

static_assert(QAudioRoom::LeftWall == 0);
static_assert(QAudioRoom::RightWall == 1);
static_assert(QAudioRoom::Floor == 2);
static_assert(QAudioRoom::Ceiling == 3);
static_assert(QAudioRoom::FrontWall == 4);
static_assert(QAudioRoom::BackWall == 5);

float QAudioRoomPrivate::wallOcclusion(QAudioRoom::Wall wall) const
{
    return m_wallOcclusion[wall] < 0 ? occlusionAndDampening[roomProperties.material_names[wall]].occlusion : m_wallOcclusion[wall];
}

float QAudioRoomPrivate::wallDampening(QAudioRoom::Wall wall) const
{
    return m_wallDampening[wall] < 0 ? occlusionAndDampening[roomProperties.material_names[wall]].dampening : m_wallDampening[wall];
}

void QAudioRoomPrivate::update()
{
    if (!dirty)
        return;
    reflections = vraudio::ComputeReflectionProperties(roomProperties);
    reverb = vraudio::ComputeReverbProperties(roomProperties);
    dirty = false;
}


/*!
    \class QAudioRoom
    \inmodule QtSpatialAudio
    \ingroup spatialaudio
    \ingroup multimedia_audio

    Defines a room for the spatial audio engine.

    If the listener is inside a room, first order sound reflections and reverb
    matching the rooms properties will get applied to the sound field.

    A room is always square and defined by its center position, its orientation and dimensions.
    Each of the 6 walls of the room can be made of different materials that will contribute
    to the computed reflections and reverb that the listener will experience while being inside
    the room.

    If multiple rooms cover the same position, the engine will use the room with the smallest
    volume.
 */

/*!
    Constructs a QAudioRoom for \a engine.
 */
QAudioRoom::QAudioRoom(QAudioEngine *engine)
    : d(new QAudioRoomPrivate)
{
    Q_ASSERT(engine);
    d->engine = engine;
    auto *ep = QAudioEnginePrivate::get(engine);
    ep->addRoom(this);
}

/*!
    Destroys the room.
 */
QAudioRoom::~QAudioRoom()
{
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->removeRoom(this);
    delete d;
}

/*!
    \enum QAudioRoom::Material

    Defines different materials that can be applied to the different walls of the room.

    \value Transparent The side of the room is open and won't contribute to reflections or reverb.
    \value AcousticCeilingTiles Acoustic tiles that suppress most reflections and reverb.
    \value BrickBare Bare brick wall.
    \value BrickPainted Painted brick wall.
    \value ConcreteBlockCoarse Raw concrete wall
    \value ConcreteBlockPainted Painted concrete wall
    \value CurtainHeavy Heavy curtain. Will mostly reflect low frequencies
    \value FiberGlassInsulation Fiber glass insulation. Only reflects very low frequencies
    \value GlassThin Thin glass wall
    \value GlassThick Thick glass wall
    \value Grass Grass
    \value LinoleumOnConcrete Linoleum floor
    \value Marble Marble floor
    \value Metal Metal
    \value ParquetOnConcrete Parquet wooden floor on concrete
    \value PlasterRough Rough plaster
    \value PlasterSmooth Smooth plaster
    \value PlywoodPanel Plywodden panel
    \value PolishedConcreteOrTile Polished concrete or tiles
    \value Sheetrock Rock
    \value WaterOrIceSurface Water or ice
    \value WoodCeiling Wooden ceiling
    \value WoodPanel Wooden panel
    \value UniformMaterial Artificial material giving uniform reflections on all frequencies
*/

/*!
    \enum QAudioRoom::Wall

    An enum defining the 6 walls of the room

    \value LeftWall Left wall (negative x)
    \value RightWall Right wall (positive x)
    \value Floor Bottom wall (negative y)
    \value Ceiling Top wall (positive y)
    \value FrontWall Front wall (negative z)
    \value BackWall Back wall (positive z)
*/


/*!
    \property QAudioRoom::position

    Defines the position of the center of the room in 3D space. Units are in centimeters
    by default.

    \sa dimensions, QAudioEngine::distanceScale
 */
void QAudioRoom::setPosition(QVector3D pos)
{
    auto *ep = QAudioEnginePrivate::get(d->engine);
    pos *= ep->distanceScale;
    if (toVector(d->roomProperties.position) == pos)
        return;
    toFloats(pos, d->roomProperties.position);
    d->dirty = true;
    emit positionChanged();
}

QVector3D QAudioRoom::position() const
{
    auto *ep = QAudioEnginePrivate::get(d->engine);
    auto pos = toVector(d->roomProperties.position);
    pos /= ep->distanceScale;
    return pos;
}

/*!
    \property QAudioRoom::dimensions

    Defines the dimensions of the room in 3D space. Units are in centimeters
    by default.

    \sa position, QAudioEngine::distanceScale
 */
void QAudioRoom::setDimensions(QVector3D dim)
{
    auto *ep = QAudioEnginePrivate::get(d->engine);
    dim *= ep->distanceScale;
    if (toVector(d->roomProperties.dimensions) == dim)
        return;
    toFloats(dim, d->roomProperties.dimensions);
    d->dirty = true;
    emit dimensionsChanged();
}

QVector3D QAudioRoom::dimensions() const
{
    auto *ep = QAudioEnginePrivate::get(d->engine);
    auto dim = toVector(d->roomProperties.dimensions);
    dim /= ep->distanceScale;
    return dim;
}

/*!
    \property QAudioRoom::rotation

    Defines the orientation of the room in 3D space.
 */
void QAudioRoom::setRotation(const QQuaternion &q)
{
    if (toQuaternion(d->roomProperties.rotation) == q)
        return;
    toFloats(q, d->roomProperties.rotation);
    d->dirty = true;
    emit rotationChanged();
}

QQuaternion QAudioRoom::rotation() const
{
    return toQuaternion(d->roomProperties.rotation);
}

/*!
    \fn void QAudioRoom::wallsChanged()

    Signals when the wall material changes.
*/
/*!
    Sets \a wall to \a material.

    Different wall materials have different reflection and reverb properties
    that influence the sound of the room.

    \sa wallMaterial(), Material, QAudioRoom::Wall
 */
void QAudioRoom::setWallMaterial(Wall wall, Material material)
{
    static_assert(vraudio::kUniform == int(UniformMaterial));
    static_assert(vraudio::kTransparent == int(Transparent));

    if (d->roomProperties.material_names[int(wall)] == int(material))
        return;
    d->roomProperties.material_names[int(wall)] = vraudio::MaterialName(int(material));
    d->dirty = true;
    emit wallsChanged();
}

/*!
    returns the material being used for \a wall.

    \sa setWallMaterial(), Material, QAudioRoom::Wall
 */
QAudioRoom::Material QAudioRoom::wallMaterial(Wall wall) const
{
    return Material(d->roomProperties.material_names[int(wall)]);
}

/*!
    \property QAudioRoom::reflectionGain

    A gain factor for reflections generated in this room. A value
    from 0 to 1 will dampen reflections, while a value larger than 1
    will apply a gain to reflections, making them louder.

    The default is 1, a factor of 0 disables reflections. Negative
    values are mapped to 0.
 */
void QAudioRoom::setReflectionGain(float factor)
{
    if (factor < 0.)
        factor = 0.;
    if (d->roomProperties.reflection_scalar == factor)
        return;
    d->roomProperties.reflection_scalar = factor;
    d->dirty = true;
    emit reflectionGainChanged();
}

float QAudioRoom::reflectionGain() const
{
    return d->roomProperties.reflection_scalar;
}

/*!
    \property QAudioRoom::reverbGain

    A gain factor for reverb generated in this room. A value
    from 0 to 1 will dampen reverb, while a value larger than 1
    will apply a gain to the reverb, making it louder.

    The default is 1, a factor of 0 disables reverb. Negative
    values are mapped to 0.
 */
void QAudioRoom::setReverbGain(float factor)
{
    if (factor < 0)
        factor = 0;
    if (d->roomProperties.reverb_gain == factor)
        return;
    d->roomProperties.reverb_gain = factor;
    d->dirty = true;
    emit reverbGainChanged();
}

float QAudioRoom::reverbGain() const
{
    return d->roomProperties.reverb_gain;
}

/*!
    \property QAudioRoom::reverbTime

    A factor to be applies to all reverb timings generated for this room.
    Larger values will lead to longer reverb timings, making the room sound
    larger.

    The default is 1. Negative values are mapped to 0.
 */
void QAudioRoom::setReverbTime(float factor)
{
    if (factor < 0)
        factor = 0;
    if (d->roomProperties.reverb_time == factor)
        return;
    d->roomProperties.reverb_time = factor;
    d->dirty = true;
    emit reverbTimeChanged();
}

float QAudioRoom::reverbTime() const
{
    return d->roomProperties.reverb_time;
}

/*!
    \property QAudioRoom::reverbBrightness

    A brightness factor to be applied to the generated reverb.
    A positive value will increase reverb for higher frequencies and
    dampen lower frequencies, a negative value does the reverse.

    The default is 0.
 */
void QAudioRoom::setReverbBrightness(float factor)
{
    if (d->roomProperties.reverb_brightness == factor)
        return;
    d->roomProperties.reverb_brightness = factor;
    d->dirty = true;
    emit reverbBrightnessChanged();
}

float QAudioRoom::reverbBrightness() const
{
    return d->roomProperties.reverb_brightness;
}

QT_END_NAMESPACE

#include "moc_qaudioroom.cpp"
