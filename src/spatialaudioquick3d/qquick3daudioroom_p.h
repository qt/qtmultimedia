// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#ifndef QQUICK3DAUDIOROOM_H
#define QQUICK3DAUDIOROOM_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qquick3dnode_p.h>
#include <QtGui/qvector3d.h>
#include <qaudioroom.h>

QT_BEGIN_NAMESPACE

class QAudioEngine;
class QAudioRoomPrivate;

class QQuick3DAudioRoom : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(QVector3D dimensions READ dimensions WRITE setDimensions NOTIFY dimensionsChanged)
    Q_PROPERTY(QQuaternion rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(Material leftMaterial READ leftMaterial WRITE setLeftMaterial NOTIFY wallsChanged)
    Q_PROPERTY(Material rightMaterial READ rightMaterial WRITE setRightMaterial NOTIFY wallsChanged)
    Q_PROPERTY(Material frontMaterial READ frontMaterial WRITE setFrontMaterial NOTIFY wallsChanged)
    Q_PROPERTY(Material backMaterial READ backMaterial WRITE setBackMaterial NOTIFY wallsChanged)
    Q_PROPERTY(Material floorMaterial READ floorMaterial WRITE setFloorMaterial NOTIFY wallsChanged)
    Q_PROPERTY(Material ceilingMaterial READ ceilingMaterial WRITE setCeilingMaterial NOTIFY wallsChanged)
    Q_PROPERTY(float reflectionGain READ reflectionGain WRITE setReflectionGain NOTIFY reflectionGainChanged)
    Q_PROPERTY(float reverbGain READ reverbGain WRITE setReverbGain NOTIFY reverbGainChanged)
    Q_PROPERTY(float reverbTime READ reverbTime WRITE setReverbTime NOTIFY reverbTimeChanged)
    Q_PROPERTY(float reverbBrightness READ reverbBrightness WRITE setReverbBrightness NOTIFY reverbBrightnessChanged)
    QML_NAMED_ELEMENT(AudioRoom)
public:
    QQuick3DAudioRoom();
    ~QQuick3DAudioRoom() override;

    enum Material {
      Transparent,
      AcousticCeilingTiles,
      BrickBare,
      BrickPainted,
      ConcreteBlockCoarse,
      ConcreteBlockPainted,
      CurtainHeavy,
      FiberGlassInsulation,
      GlassThin,
      GlassThick,
      Grass,
      LinoleumOnConcrete,
      Marble,
      Metal,
      ParquetOnConcrete,
      PlasterRough,
      PlasterSmooth,
      PlywoodPanel,
      PolishedConcreteOrTile,
      Sheetrock,
      WaterOrIceSurface,
      WoodCeiling,
      WoodPanel,
      Uniform,
    };
    Q_ENUM(Material)

    void setDimensions(QVector3D pos);
    QVector3D dimensions() const;

    void setLeftMaterial(Material material);
    Material leftMaterial() const;

    void setRightMaterial(Material material);
    Material rightMaterial() const;

    void setFrontMaterial(Material material);
    Material frontMaterial() const;

    void setBackMaterial(Material material);
    Material backMaterial() const;

    void setFloorMaterial(Material material);
    Material floorMaterial() const;

    void setCeilingMaterial(Material material);
    Material ceilingMaterial() const;

    void setReflectionGain(float factor);
    float reflectionGain() const;

    void setReverbGain(float factor);
    float reverbGain() const;

    void setReverbTime(float factor);
    float reverbTime() const;

    void setReverbBrightness(float factor);
    float reverbBrightness() const;

Q_SIGNALS:
    void positionChanged();
    void dimensionsChanged();
    void rotationChanged();
    void wallsChanged();
    void reflectionGainChanged();
    void reverbGainChanged();
    void reverbTimeChanged();
    void reverbBrightnessChanged();

protected Q_SLOTS:
    void updatePosition();
    void updateRotation();

private:
    QAudioRoom *m_room;
};

QT_END_NAMESPACE

#endif
