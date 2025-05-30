// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#ifndef QAUDIOROOM_P_H
#define QAUDIOROOM_P_H

//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qtspatialaudioglobal_p.h>
#include <qaudioroom.h>
#include <qaudioengine_p.h>
#include <QtGui/qquaternion.h>

#include <resonance_audio.h>
#include "platforms/common/room_effects_utils.h"
#include "platforms/common/room_properties.h"

QT_BEGIN_NAMESPACE

class QAudioRoomPrivate
{
public:
    static QAudioRoomPrivate *get(const QAudioRoom *r) { return r->d; }

    QAudioEngine *engine = nullptr;
    vraudio::RoomProperties roomProperties;
    bool dirty = true;

    vraudio::ReverbProperties reverb;
    vraudio::ReflectionProperties reflections;

    float m_wallOcclusion[6] = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };
    float m_wallDampening[6] = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };

    float wallOcclusion(QAudioRoom::Wall wall) const;
    float wallDampening(QAudioRoom::Wall wall) const;

    void update();
};

QT_END_NAMESPACE

#endif
