// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVIDEO_H
#define QVIDEO_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

namespace QVideo
{
Q_NAMESPACE_EXPORT(Q_MULTIMEDIA_EXPORT)

enum RotationAngle { Rotation0 = 0, Rotation90 = 90, Rotation180 = 180, Rotation270 = 270 };
Q_ENUM_NS(RotationAngle)
}

QT_END_NAMESPACE

#endif // QVIDEO_H
