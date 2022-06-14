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
