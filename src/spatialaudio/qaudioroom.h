/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#ifndef QAUDIOROOM_H
#define QAUDIOROOM_H

#include <QtSpatialAudio/qtspatialaudioglobal.h>
#include <QtCore/qobject.h>
#include <QtGui/qvector3d.h>

QT_BEGIN_NAMESPACE

class QAudioEngine;
class QAudioRoomPrivate;

class Q_SPATIALAUDIO_EXPORT QAudioRoom : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(QVector3D dimensions READ dimensions WRITE setDimensions NOTIFY dimensionsChanged)
    Q_PROPERTY(QQuaternion rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(float reflectionGain READ reflectionGain WRITE setReflectionGain NOTIFY reflectionGainChanged)
    Q_PROPERTY(float reverbGain READ reverbGain WRITE setReverbGain NOTIFY reverbGainChanged)
    Q_PROPERTY(float reverbTime READ reverbTime WRITE setReverbTime NOTIFY reverbTimeChanged)
    Q_PROPERTY(float reverbBrightness READ reverbBrightness WRITE setReverbBrightness NOTIFY reverbBrightnessChanged)
public:
    QAudioRoom(QAudioEngine *engine);
    ~QAudioRoom();

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
      UniformMaterial,
    };

    enum Wall {
        LeftWall,
        RightWall,
        Floor,
        Ceiling,
        FrontWall,
        BackWall
    };

    void setPosition(QVector3D pos);
    QVector3D position() const;

    void setDimensions(QVector3D pos);
    QVector3D dimensions() const;

    void setRotation(const QQuaternion &q);
    QQuaternion rotation() const;

    void setWallMaterial(Wall wall, Material material);
    Material wallMaterial(Wall wall) const;

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

private:
    friend class QAudioRoomPrivate;
    QAudioRoomPrivate *d;
};

QT_END_NAMESPACE

#endif
