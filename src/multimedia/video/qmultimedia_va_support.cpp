// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmultimedia_va_support_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

QDebug operator<<(QDebug debug, VAStatus status)
{
    const QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "VAStatus(0x" << Qt::hex << qToUnderlying(status) << Qt::dec
                              << ", " << vaErrorStr(int(status)) << ')';
    return debug;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
