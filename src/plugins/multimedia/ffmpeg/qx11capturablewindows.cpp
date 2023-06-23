// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qx11capturablewindows_p.h"

QT_BEGIN_NAMESPACE

QList<QCapturableWindow> QX11CapturableWindows::windows() const
{
    // to be imlemented
    return {};
}

bool QX11CapturableWindows::isWindowValid(const QCapturableWindowPrivate &) const
{
    // to be imlemented
    return false;
}

QT_END_NAMESPACE
