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
#ifndef QSPATIALAUDIOSOURCE_H
#define QSPATIALAUDIOSOURCE_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/QObject>
#include <QtGui/qvector3d.h>
#include <QtGui/qquaternion.h>

QT_BEGIN_NAMESPACE

class QSpatialAudioEngine;
class QSpatialAudioSound;

class QSpatialAudioSoundSourcePrivate;
class Q_MULTIMEDIA_EXPORT QSpatialAudioSoundSource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(QQuaternion rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(DistanceModel distanceModel READ distanceModel WRITE setDistanceModel NOTIFY distanceModelChanged)
    Q_PROPERTY(float size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(float distanceCutoff READ distanceCutoff WRITE setDistanceCutoff NOTIFY distanceCutoffChanged)
    Q_PROPERTY(float manualAttenuation READ manualAttenuation WRITE setManualAttenuation NOTIFY manualAttenuationChanged)
    Q_PROPERTY(float occlusionIntensity READ occlusionIntensity WRITE setOcclusionIntensity NOTIFY occlusionIntensityChanged)
    Q_PROPERTY(float directivity READ directivity WRITE setDirectivity NOTIFY directivityChanged)
    Q_PROPERTY(float directivityOrder READ directivityOrder WRITE setDirectivityOrder NOTIFY directivityOrderChanged)
    Q_PROPERTY(float nearFieldGain READ nearFieldGain WRITE setNearFieldGain NOTIFY nearFieldGainChanged)
    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopsChanged)
    Q_PROPERTY(bool autoPlay READ autoPlay WRITE setAutoPlay NOTIFY autoPlayChanged)

public:
    explicit QSpatialAudioSoundSource(QSpatialAudioEngine *engine);
    ~QSpatialAudioSoundSource();

    void setSource(const QUrl &url);
    QUrl source() const;

    enum Loops
    {
        Infinite = -1,
        Once = 1
    };
    Q_ENUM(Loops)

    int loops() const;
    void setLoops(int loops);

    bool autoPlay() const;
    void setAutoPlay(bool autoPlay);

    void setPosition(QVector3D pos);
    QVector3D position() const;

    void setRotation(const QQuaternion &q);
    QQuaternion rotation() const;

    void setVolume(float volume);
    float volume() const;

    enum DistanceModel {
        DistanceModel_Logarithmic,
        DistanceModel_Linear,
        DistanceModel_ManualAttenutation
    };
    Q_ENUM(DistanceModel);

    void setDistanceModel(DistanceModel model);
    DistanceModel distanceModel() const;

    void setSize(float size);
    float size() const;

    void setDistanceCutoff(float cutoff);
    float distanceCutoff() const;

    void setManualAttenuation(float attenuation);
    float manualAttenuation() const;

    void setOcclusionIntensity(float occlusion);
    float occlusionIntensity() const;

    void setDirectivity(float alpha);
    float directivity() const;

    void setDirectivityOrder(float alpha);
    float directivityOrder() const;

    void setNearFieldGain(float gain);
    float nearFieldGain() const;

    QSpatialAudioEngine *engine() const;

Q_SIGNALS:
    void sourceChanged();
    void loopsChanged();
    void autoPlayChanged();
    void positionChanged();
    void rotationChanged();
    void volumeChanged();
    void distanceModelChanged();
    void sizeChanged();
    void distanceCutoffChanged();
    void manualAttenuationChanged();
    void occlusionIntensityChanged();
    void directivityChanged();
    void directivityOrderChanged();
    void nearFieldGainChanged();

public Q_SLOTS:
    void play();
    void pause();
    void stop();

private:
    void setEngine(QSpatialAudioEngine *engine);
    friend class QSpatialAudioSound;
    friend class QSpatialAudioSoundSourcePrivate;
    QSpatialAudioSoundSourcePrivate *d = nullptr;
};

QT_END_NAMESPACE

#endif
