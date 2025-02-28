// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qvideotransformation_p.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

QDebug operator<<(QDebug dbg, const VideoTransformation &transform)
{
    dbg << "[ rotation:" << transform.rotation
        << "; mirrored:" << transform.mirroredHorizontallyAfterRotation << "]";
    return dbg;
}

QT_END_NAMESPACE
