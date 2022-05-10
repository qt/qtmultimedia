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
#include <qspatialaudioroom_p.h>

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
}

void QSpatialAudioRoomPrivate::update()
{
    if (!dirty)
        return;
    reflections = vraudio::ComputeReflectionProperties(roomProperties);
    reverb = vraudio::ComputeReverbProperties(roomProperties);
    dirty = false;
}


/*!
    \class QSpatialAudioRoom
    \inmodule QtMultimedia
    \ingroup multimedia_spatialaudio

    Defines a room for the spatial audio engine.

    If the listener is inside a room, first order sound reflections and reverb
    matching the rooms properties will get applied to the sound field.

    A room is always square and defined by it's center position, it's orientation and dimensions.
    Each of the 6 walls of the room can be made of different materials that will contribute
    to the computed reflections and reverb that the listener will experience while being inside
    the room.

    If multiple rooms cover the same position, the engine will use the room with the smallest
    volume.
 */

/*!
    Constructs a QSpatialAudioRoom for \a engine.
 */
QSpatialAudioRoom::QSpatialAudioRoom(QSpatialAudioEngine *engine)
    : d(new QSpatialAudioRoomPrivate)
{
    d->engine = engine;
    auto *ep = QSpatialAudioEnginePrivate::get(engine);
    ep->addRoom(this);
}

/*!
    Destroys the room.
 */
QSpatialAudioRoom::~QSpatialAudioRoom()
{
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->removeRoom(this);
    delete d;
}

/*!
    \enum QSpatialAudioRoom::Material

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
    \enum QSpatialAudioRoom::Wall

    An enum defining the 6 walls of the room

    \value LeftWall Left wall (negative x)
    \value RightWall Right wall (positive x)
    \value BackWall Back wall (negative z)
    \value FrontWall Front wall (positive z)
    \value Floor Bottom wall (negative y)
    \value Ceiling Top wall (positive y)
*/


/*!
    \property QSpatialAudioRoom::position

    Defines the position of the center of the room in 3D space. All units are
    assumed to be in meters.
 */
void QSpatialAudioRoom::setPosition(QVector3D pos)
{
    if (toVector(d->roomProperties.position) == pos)
        return;
    toFloats(pos, d->roomProperties.position);
    d->dirty = true;
    emit positionChanged();
}

QVector3D QSpatialAudioRoom::position() const
{
    return toVector(d->roomProperties.position);
}

/*!
    \property QSpatialAudioRoom::dimensions

    Defines the dimensions of the room in 3D space. All units are
    assumed to be in meters.
 */
void QSpatialAudioRoom::setDimensions(QVector3D dim)
{
    if (toVector(d->roomProperties.dimensions) == dim)
        return;
    toFloats(dim, d->roomProperties.dimensions);
    d->dirty = true;
    emit dimensionsChanged();
}

QVector3D QSpatialAudioRoom::dimensions() const
{
    return toVector(d->roomProperties.dimensions);
}

/*!
    \property QSpatialAudioRoom::rotation

    Defines the orientation of the room in 3D space.
 */
void QSpatialAudioRoom::setRotation(const QQuaternion &q)
{
    if (toQuaternion(d->roomProperties.rotation) == q)
        return;
    toFloats(q, d->roomProperties.rotation);
    d->dirty = true;
    emit rotationChanged();
}

QQuaternion QSpatialAudioRoom::rotation() const
{
    return toQuaternion(d->roomProperties.rotation);
}

/*!
    Sets \a wall to \a material.

    Different wall materials have different reflection and reverb properties
    that influence the sound of the room.

    \sa wallMaterial(), Material, QSpatialAudioRoom::Wall
 */
void QSpatialAudioRoom::setWallMaterial(Wall wall, Material material)
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

    \sa setWallMaterial(), Material, QSpatialAudioRoom::Wall
 */
QSpatialAudioRoom::Material QSpatialAudioRoom::wallMaterial(Wall wall) const
{
    return Material(d->roomProperties.material_names[int(wall)]);
}

/*!
    \property QSpatialAudioRoom::reflectionGain

    A gain factor for reflections generated in this room. A value
    from 0 to 1 will dampen reflections, while a value larger than 1
    will apply a gain to reflections, making them louder.

    The default is 1, a factor of 0 disables reflections. Negative
    values are mapped to 0.
 */
void QSpatialAudioRoom::setReflectionGain(float factor)
{
    if (factor < 0.)
        factor = 0.;
    if (d->roomProperties.reflection_scalar == factor)
        return;
    d->roomProperties.reflection_scalar = factor;
    d->dirty = true;
    reflectionGainChanged();
}

float QSpatialAudioRoom::reflectionGain() const
{
    return d->roomProperties.reflection_scalar;
}

/*!
    \property QSpatialAudioRoom::reverbGain

    A gain factor for reverb generated in this room. A value
    from 0 to 1 will dampen reverb, while a value larger than 1
    will apply a gain to the reverb, making it louder.

    The default is 1, a factor of 0 disables reverb. Negative
    values are mapped to 0.
 */
void QSpatialAudioRoom::setReverbGain(float factor)
{
    if (factor < 0)
        factor = 0;
    if (d->roomProperties.reverb_gain == factor)
        return;
    d->roomProperties.reverb_gain = factor;
    d->dirty = true;
    reverbGainChanged();
}

float QSpatialAudioRoom::reverbGain() const
{
    return d->roomProperties.reverb_gain;
}

/*!
    \property QSpatialAudioRoom::reverbTime

    A factor to be applies to all reverb timings generated for this room.
    Larger values will lead to longer reverb timings, making the room sound
    larger.

    The default is 1. Negative values are mapped to 0.
 */
void QSpatialAudioRoom::setReverbTime(float factor)
{
    if (factor < 0)
        factor = 0;
    if (d->roomProperties.reverb_time == factor)
        return;
    d->roomProperties.reverb_time = factor;
    d->dirty = true;
    reverbTimeChanged();
}

float QSpatialAudioRoom::reverbTime() const
{
    return d->roomProperties.reverb_time;
}

/*!
    \property QSpatialAudioRoom::reverbBrightness

    A brightness factor to be applied to the generated reverb.
    A positive value will increase reverb for higher frequencies and
    dampen lower frequencies, a negative value does the reverse.

    The default is 0.
 */
void QSpatialAudioRoom::setReverbBrightness(float factor)
{
    if (d->roomProperties.reverb_brightness == factor)
        return;
    d->roomProperties.reverb_brightness = factor;
    d->dirty = true;
    reverbBrightnessChanged();
}

float QSpatialAudioRoom::reverbBrightness() const
{
    return d->roomProperties.reverb_brightness;
}

QT_END_NAMESPACE

#include "moc_qspatialaudioroom.cpp"
