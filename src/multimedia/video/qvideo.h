// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVIDEO_H
#define QVIDEO_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

namespace QtVideo
{
Q_NAMESPACE_EXPORT(Q_MULTIMEDIA_EXPORT)

enum class Rotation { None = 0, Clockwise90 = 90, Clockwise180 = 180, Clockwise270 = 270 };
Q_ENUM_NS(Rotation)
}

QT_END_NAMESPACE

#endif // QVIDEO_H
