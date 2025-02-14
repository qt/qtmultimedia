// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTVIDEO_H
#define QTVIDEO_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qvideoframe.h>

// The header ensures the code compatibility of qt versions >= 6.7 with 6.6 and 6.5

QT_BEGIN_NAMESPACE

namespace QtVideo
{
Q_NAMESPACE_EXPORT(Q_MULTIMEDIA_EXPORT)

enum class Rotation { None = 0, Clockwise90 = 90, Clockwise180 = 180, Clockwise270 = 270 };
Q_ENUM_NS(Rotation)
}

QT_END_NAMESPACE

#endif // QTVIDEO_H
