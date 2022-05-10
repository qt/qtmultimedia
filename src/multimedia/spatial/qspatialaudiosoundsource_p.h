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

#include <qspatialaudiosoundsource.h>
#include <qspatialaudioengine_p.h>
#include <qurl.h>
#include <qvector3d.h>
#include <qquaternion.h>
#include <qaudiobuffer.h>
#include <qaudiodevice.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

class QAudioDecoder;
class QSpatialAudioEnginePrivate;

class QSpatialAudioSoundSourcePrivate : public QSpatialAudioSound
{
public:
    QSpatialAudioSoundSourcePrivate(QObject *parent)
        : QSpatialAudioSound(parent, 1)
    {}

    static QSpatialAudioSoundSourcePrivate *get(QSpatialAudioSoundSource *soundSource)
    { return soundSource ? soundSource->d : nullptr; }

    QVector3D pos;
    QQuaternion rotation;
    QSpatialAudioSoundSource::DistanceModel distanceModel = QSpatialAudioSoundSource::DistanceModel_Logarithmic;
    float minDistance = .1;
    float maxDistance = 50.;
    float manualAttenuation = 0;
    float occlusionIntensity = 0.;
    float directivity = 0.;
    float directivityOrder = 1.;
    float nearFieldGain = 0.;
    float wallDampening = 1.;
    float wallOcclusion = 0.;

    void updateDistanceModel();
    void updateRoomEffects();
};

QT_END_NAMESPACE

#endif
