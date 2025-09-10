// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#include <qquick3daudioroom_p.h>
#include <qquick3daudioengine_p.h>
#include <qaudioroom.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype AudioRoom
    \inqmlmodule QtQuick3D.SpatialAudio
    \ingroup quick3d_spatialaudio
    \ingroup multimedia_audio_qml

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

QQuick3DAudioRoom::QQuick3DAudioRoom()
{
    m_room = new QAudioRoom(QQuick3DAudioEngine::getEngine());

    connect(this, &QQuick3DNode::scenePositionChanged, this, &QQuick3DAudioRoom::updatePosition);
    connect(this, &QQuick3DNode::sceneRotationChanged, this, &QQuick3DAudioRoom::updateRotation);
    connect(m_room, &QAudioRoom::dimensionsChanged, this, &QQuick3DAudioRoom::dimensionsChanged);
    connect(m_room, &QAudioRoom::rotationChanged, this, &QQuick3DAudioRoom::rotationChanged);
    connect(m_room, &QAudioRoom::wallsChanged, this, &QQuick3DAudioRoom::wallsChanged);
    connect(m_room, &QAudioRoom::reflectionGainChanged, this, &QQuick3DAudioRoom::reflectionGainChanged);
    connect(m_room, &QAudioRoom::reverbGainChanged, this, &QQuick3DAudioRoom::reverbGainChanged);
    connect(m_room, &QAudioRoom::reverbTimeChanged, this, &QQuick3DAudioRoom::reverbTimeChanged);
    connect(m_room, &QAudioRoom::reverbBrightnessChanged, this, &QQuick3DAudioRoom::reverbBrightnessChanged);
}

QQuick3DAudioRoom::~QQuick3DAudioRoom()
{
    delete m_room;
}

/*!
    \qmlproperty vector3D AudioRoom::dimensions

    Defines the dimensions of the room in 3D space. Units are in centimeters
    by default.

    \sa QtQuick3D::Node::position
 */
void QQuick3DAudioRoom::setDimensions(QVector3D dim)
{
    m_room->setDimensions(dim);
}

QVector3D QQuick3DAudioRoom::dimensions() const
{
    return m_room->dimensions();
}

/*!
    \qmlproperty AudioRoom::Material AudioRoom::leftMaterial
    \qmlproperty AudioRoom::Material AudioRoom::rightMaterial
    \qmlproperty AudioRoom::Material AudioRoom::frontMaterial
    \qmlproperty AudioRoom::Material AudioRoom::backMaterial
    \qmlproperty AudioRoom::Material AudioRoom::floorMaterial
    \qmlproperty AudioRoom::Material AudioRoom::ceilingMaterial

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
void QQuick3DAudioRoom::setLeftMaterial(Material material)
{
    m_room->setWallMaterial(QAudioRoom::LeftWall, QAudioRoom::Material(material));
}

QQuick3DAudioRoom::Material QQuick3DAudioRoom::leftMaterial() const
{
    return Material(m_room->wallMaterial(QAudioRoom::LeftWall));
}

void QQuick3DAudioRoom::setRightMaterial(Material material)
{
    m_room->setWallMaterial(QAudioRoom::RightWall, QAudioRoom::Material(material));
}

QQuick3DAudioRoom::Material QQuick3DAudioRoom::rightMaterial() const
{
    return Material(m_room->wallMaterial(QAudioRoom::RightWall));
}

void QQuick3DAudioRoom::setFrontMaterial(Material material)
{
    m_room->setWallMaterial(QAudioRoom::FrontWall, QAudioRoom::Material(material));
}

QQuick3DAudioRoom::Material QQuick3DAudioRoom::frontMaterial() const
{
    return Material(m_room->wallMaterial(QAudioRoom::FrontWall));
}

void QQuick3DAudioRoom::setBackMaterial(Material material)
{
    m_room->setWallMaterial(QAudioRoom::BackWall, QAudioRoom::Material(material));
}

QQuick3DAudioRoom::Material QQuick3DAudioRoom::backMaterial() const
{
    return Material(m_room->wallMaterial(QAudioRoom::BackWall));
}

void QQuick3DAudioRoom::setFloorMaterial(Material material)
{
    m_room->setWallMaterial(QAudioRoom::Floor, QAudioRoom::Material(material));
}

QQuick3DAudioRoom::Material QQuick3DAudioRoom::floorMaterial() const
{
    return Material(m_room->wallMaterial(QAudioRoom::Floor));
}

void QQuick3DAudioRoom::setCeilingMaterial(Material material)
{
    m_room->setWallMaterial(QAudioRoom::Ceiling, QAudioRoom::Material(material));
}

QQuick3DAudioRoom::Material QQuick3DAudioRoom::ceilingMaterial() const
{
    return Material(m_room->wallMaterial(QAudioRoom::Ceiling));
}

/*!
    \qmlproperty real AudioRoom::reflectionGain

    A gain factor for reflections generated in this room. A value
    from 0 to 1 will dampen reflections, while a value larger than 1
    will apply a gain to reflections, making them louder.

    The default is 1, a factor of 0 disables reflections. Negative
    values are mapped to 0.
 */
void QQuick3DAudioRoom::setReflectionGain(float factor)
{
    m_room->setReflectionGain(factor);
}

float QQuick3DAudioRoom::reflectionGain() const
{
    return m_room->reflectionGain();
}

/*!
    \qmlproperty real AudioRoom::reverbGain

    A gain factor for reverb generated in this room. A value
    from 0 to 1 will dampen reverb, while a value larger than 1
    will apply a gain to the reverb, making it louder.

    The default is 1, a factor of 0 disables reverb. Negative
    values are mapped to 0.
 */
void QQuick3DAudioRoom::setReverbGain(float factor)
{
    m_room->setReverbGain(factor);
}

float QQuick3DAudioRoom::reverbGain() const
{
    return m_room->reverbGain();
}

/*!
    \qmlproperty real AudioRoom::reverbTime

    A factor to be applies to all reverb timings generated for this room.
    Larger values will lead to longer reverb timings, making the room sound
    larger.

    The default is 1. Negative values are mapped to 0.
 */
void QQuick3DAudioRoom::setReverbTime(float factor)
{
    m_room->setReverbTime(factor);
}

float QQuick3DAudioRoom::reverbTime() const
{
    return m_room->reverbTime();
}

/*!
    \qmlproperty real AudioRoom::reverbBrightness

    A brightness factor to be applied to the generated reverb.
    A positive value will increase reverb for higher frequencies and
    dampen lower frequencies, a negative value does the reverse.

    The default is 0.
 */
void QQuick3DAudioRoom::setReverbBrightness(float factor)
{
    m_room->setReverbBrightness(factor);
}

float QQuick3DAudioRoom::reverbBrightness() const
{
    return m_room->reverbBrightness();
}

void QQuick3DAudioRoom::updatePosition()
{
    m_room->setPosition(scenePosition());
}

void QQuick3DAudioRoom::updateRotation()
{
    m_room->setRotation(sceneRotation());
}

QT_END_NAMESPACE
