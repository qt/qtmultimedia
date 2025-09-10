// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#ifndef QQUICK3DSPATIALSOUND_H
#define QQUICK3DSPATIALSOUND_H

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
#include <QUrl>
#include <qvector3d.h>
#include <qspatialsound.h>

QT_BEGIN_NAMESPACE

class QQuick3DSpatialSound : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
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
    QML_NAMED_ELEMENT(SpatialSound)

public:
    QQuick3DSpatialSound();
    ~QQuick3DSpatialSound() override;

    void setSource(QUrl source);
    QUrl source() const;

    void setVolume(float volume);
    float volume() const;

    enum DistanceModel {
        Logarithmic,
        Linear,
        ManualAttenuation
    };
    Q_ENUM(DistanceModel);

    void setDistanceModel(DistanceModel model);
    DistanceModel distanceModel() const;

    void setSize(float min);
    float size() const;

    void setDistanceCutoff(float max);
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

public Q_SLOTS:
    void play();
    void pause();
    void stop();

Q_SIGNALS:
    void sourceChanged();
    void volumeChanged();
    void distanceModelChanged();
    void sizeChanged();
    void distanceCutoffChanged();
    void manualAttenuationChanged();
    void occlusionIntensityChanged();
    void directivityChanged();
    void directivityOrderChanged();
    void nearFieldGainChanged();
    void loopsChanged();
    void autoPlayChanged();

private Q_SLOTS:
    void updatePosition();
    void updateRotation();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *) override { return nullptr; }

private:
    QSpatialSound *m_sound = nullptr;
};

QT_END_NAMESPACE

#endif
