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
#include <qquick3dspatialaudioroom_p.h>
#include <qquick3dspatialaudioengine_p.h>
#include <qspatialaudioroom.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SpatialAudioRoom
    \inqmlmodule QtQuick3D.SpatialAudio
    \ingroup quick3d_spatialaudio

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

QQuick3DSpatialAudioRoom::QQuick3DSpatialAudioRoom()
{
    m_room = new QSpatialAudioRoom(QQuick3DSpatialAudioEngine::getEngine());

    connect(this, &QQuick3DNode::scenePositionChanged, this, &QQuick3DSpatialAudioRoom::updatePosition);
    connect(this, &QQuick3DNode::sceneRotationChanged, this, &QQuick3DSpatialAudioRoom::updateRotation);
    connect(m_room, &QSpatialAudioRoom::dimensionsChanged, this, &QQuick3DSpatialAudioRoom::dimensionsChanged);
    connect(m_room, &QSpatialAudioRoom::rotationChanged, this, &QQuick3DSpatialAudioRoom::rotationChanged);
    connect(m_room, &QSpatialAudioRoom::wallsChanged, this, &QQuick3DSpatialAudioRoom::wallsChanged);
    connect(m_room, &QSpatialAudioRoom::reflectionGainChanged, this, &QQuick3DSpatialAudioRoom::reflectionGainChanged);
    connect(m_room, &QSpatialAudioRoom::reverbGainChanged, this, &QQuick3DSpatialAudioRoom::reverbGainChanged);
    connect(m_room, &QSpatialAudioRoom::reverbTimeChanged, this, &QQuick3DSpatialAudioRoom::reverbTimeChanged);
    connect(m_room, &QSpatialAudioRoom::reverbBrightnessChanged, this, &QQuick3DSpatialAudioRoom::reverbBrightnessChanged);
}

QQuick3DSpatialAudioRoom::~QQuick3DSpatialAudioRoom()
{
    delete m_room;
}

/*!
    \qmlproperty vector3D SpatialAudioRoom::dimensions

    Defines the dimensions of the room in 3D space. Units are in centimeters
    by default.

    \sa position, QSpatialAudioEngine::distanceScale
 */
void QQuick3DSpatialAudioRoom::setDimensions(QVector3D dim)
{
    m_room->setDimensions(dim);
}

QVector3D QQuick3DSpatialAudioRoom::dimensions() const
{
    return m_room->dimensions();
}

/*!
    \qmlproperty SpatialAudioRoom::Material SpatialAudioRoom::left
    \qmlproperty SpatialAudioRoom::Material SpatialAudioRoom::right
    \qmlproperty SpatialAudioRoom::Material SpatialAudioRoom::front
    \qmlproperty SpatialAudioRoom::Material SpatialAudioRoom::back
    \qmlproperty SpatialAudioRoom::Material SpatialAudioRoom::floor
    \qmlproperty SpatialAudioRoom::Material SpatialAudioRoom::ceiling

    Sets the material to use for the different sides of the room. Properties correlate to
    coordinates as follows:

    \table
    \header
        \li Property
        \li Coordinate
    \row \li left \li Negative x
    \row \li right \li Positive x
    \row \li back \li Negative z
    \row \li front \li Positive z
    \row \li floor \li Negative y
    \row \li ceiling \li Positive y
    \endtable

    Valid values for the material are:

    \table
    \header
        \li Property value
        \li Description
    \row \li Transparent \li The side of the room is open and won't contribute to reflections or reverb.
    \row \li AcousticCeilingTiles \li Acoustic tiles that suppress most reflections and reverb.
    \row \li BrickBare \li A bare brick wall.
    \row \li BrickPainted \li A painted brick wall.
    \row \li ConcreteBlockCoarse \li A raw concrete wall
    \row \li ConcreteBlockPainted \li A painted concrete wall
    \row \li CurtainHeavy \li A heavy curtain. Will mostly reflect low frequencies
    \row \li FiberGlassInsulation \li Fiber glass insulation. Only reflects very low frequencies
    \row \li GlassThin \li A thin glass wall
    \row \li GlassThick \li A thick glass wall
    \row \li Grass \li Grass
    \row \li LinoleumOnConcrete \li A Linoleum floor
    \row \li Marble \li A marble floor
    \row \li Metal \li Metal
    \row \li ParquetOnConcrete \li Parquet wooden floor on concrete
    \row \li PlasterRough \li Rough plaster
    \row \li PlasterSmooth \li Smooth plaster
    \row \li PlywoodPanel \li Plywodden panel
    \row \li PolishedConcreteOrTile \li Polished concrete or tiles
    \row \li Sheetrock \li Rock
    \row \li WaterOrIceSurface \li Water or ice
    \row \li WoodCeiling \li A wooden ceiling
    \row \li WoodPanel \li Wooden panel
    \row \li Uniform \li Artificial material giving uniform reflections on all frequencies
    \endtable
 */
void QQuick3DSpatialAudioRoom::setLeft(Material material)
{
    m_room->setWallMaterial(QSpatialAudioRoom::LeftWall, QSpatialAudioRoom::Material(material));
}

QQuick3DSpatialAudioRoom::Material QQuick3DSpatialAudioRoom::left() const
{
    return Material(m_room->wallMaterial(QSpatialAudioRoom::LeftWall));
}

void QQuick3DSpatialAudioRoom::setRight(Material material)
{
    m_room->setWallMaterial(QSpatialAudioRoom::RightWall, QSpatialAudioRoom::Material(material));
}

QQuick3DSpatialAudioRoom::Material QQuick3DSpatialAudioRoom::right() const
{
    return Material(m_room->wallMaterial(QSpatialAudioRoom::RightWall));
}

void QQuick3DSpatialAudioRoom::setFront(Material material)
{
    m_room->setWallMaterial(QSpatialAudioRoom::FrontWall, QSpatialAudioRoom::Material(material));
}

QQuick3DSpatialAudioRoom::Material QQuick3DSpatialAudioRoom::front() const
{
    return Material(m_room->wallMaterial(QSpatialAudioRoom::FrontWall));
}

void QQuick3DSpatialAudioRoom::setBack(Material material)
{
    m_room->setWallMaterial(QSpatialAudioRoom::BackWall, QSpatialAudioRoom::Material(material));
}

QQuick3DSpatialAudioRoom::Material QQuick3DSpatialAudioRoom::back() const
{
    return Material(m_room->wallMaterial(QSpatialAudioRoom::BackWall));
}

void QQuick3DSpatialAudioRoom::setFloor(Material material)
{
    m_room->setWallMaterial(QSpatialAudioRoom::Floor, QSpatialAudioRoom::Material(material));
}

QQuick3DSpatialAudioRoom::Material QQuick3DSpatialAudioRoom::floor() const
{
    return Material(m_room->wallMaterial(QSpatialAudioRoom::Floor));
}

void QQuick3DSpatialAudioRoom::setCeiling(Material material)
{
    m_room->setWallMaterial(QSpatialAudioRoom::Ceiling, QSpatialAudioRoom::Material(material));
}

QQuick3DSpatialAudioRoom::Material QQuick3DSpatialAudioRoom::ceiling() const
{
    return Material(m_room->wallMaterial(QSpatialAudioRoom::Ceiling));
}

/*!
    \qmlproperty float SpatialAudioRoom::reflectionGain

    A gain factor for reflections generated in this room. A value
    from 0 to 1 will dampen reflections, while a value larger than 1
    will apply a gain to reflections, making them louder.

    The default is 1, a factor of 0 disables reflections. Negative
    values are mapped to 0.
 */
void QQuick3DSpatialAudioRoom::setReflectionGain(float factor)
{
    m_room->setReflectionGain(factor);
}

float QQuick3DSpatialAudioRoom::reflectionGain() const
{
    return m_room->reflectionGain();
}

/*!
    \qmlproperty float SpatialAudioRoom::reverbGain

    A gain factor for reverb generated in this room. A value
    from 0 to 1 will dampen reverb, while a value larger than 1
    will apply a gain to the reverb, making it louder.

    The default is 1, a factor of 0 disables reverb. Negative
    values are mapped to 0.
 */
void QQuick3DSpatialAudioRoom::setReverbGain(float factor)
{
    m_room->setReverbGain(factor);
}

float QQuick3DSpatialAudioRoom::reverbGain() const
{
    return m_room->reverbGain();
}

/*!
    \qmlproperty float SpatialAudioRoom::reverbTime

    A factor to be applies to all reverb timings generated for this room.
    Larger values will lead to longer reverb timings, making the room sound
    larger.

    The default is 1. Negative values are mapped to 0.
 */
void QQuick3DSpatialAudioRoom::setReverbTime(float factor)
{
    m_room->setReverbTime(factor);
}

float QQuick3DSpatialAudioRoom::reverbTime() const
{
    return m_room->reverbTime();
}

/*!
    \qmlproperty float SpatialAudioRoom::reverbBrightness

    A brightness factor to be applied to the generated reverb.
    A positive value will increase reverb for higher frequencies and
    dampen lower frequencies, a negative value does the reverse.

    The default is 0.
 */
void QQuick3DSpatialAudioRoom::setReverbBrightness(float factor)
{
    m_room->setReverbBrightness(factor);
}

float QQuick3DSpatialAudioRoom::reverbBrightness() const
{
    return m_room->reverbBrightness();
}

void QQuick3DSpatialAudioRoom::updatePosition()
{
    m_room->setPosition(scenePosition());
}

void QQuick3DSpatialAudioRoom::updateRotation()
{
    m_room->setRotation(sceneRotation());
}

QT_END_NAMESPACE
