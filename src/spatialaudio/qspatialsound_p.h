// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#ifndef QSPATIALAUDIOSOUNDSOURCE_P_H
#define QSPATIALAUDIOSOUNDSOURCE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qspatialsound.h>
#include <qambientsound_p.h>
#include <qaudioengine_p.h>
#include <qurl.h>
#include <qvector3d.h>
#include <qquaternion.h>
#include <qaudiobuffer.h>
#include <qaudiodevice.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

class QAudioDecoder;
class QAudioEnginePrivate;

class QSpatialSoundPrivate : public QAmbientSoundPrivate
{
public:
    QSpatialSoundPrivate(QObject *parent)
        : QAmbientSoundPrivate(parent, 1)
    {}

    static QSpatialSoundPrivate *get(QSpatialSound *soundSource)
    { return soundSource ? soundSource->d : nullptr; }

    QVector3D pos;
    QQuaternion rotation;
    QSpatialSound::DistanceModel distanceModel = QSpatialSound::DistanceModel::Logarithmic;
    float size = .1f;
    float distanceCutoff = 50.f;
    float manualAttenuation = 0.f;
    float occlusionIntensity = 0.f;
    float directivity = 0.f;
    float directivityOrder = 1.f;
    float nearFieldGain = 0.f;
    float wallDampening = 1.f;
    float wallOcclusion = 0.f;

    void updateDistanceModel();
    void updateRoomEffects();
};

QT_END_NAMESPACE

#endif
