// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWS_SCOPEDPROPVARIANT_P_H
#define QWINDOWS_SCOPEDPROPVARIANT_P_H

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

#include <QtCore/qglobal.h>
#include <propidl.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

// RAII wrapper for PROPVARIANT
struct ScopedPropVariant
{
    PROPVARIANT var;

    ScopedPropVariant() { PropVariantInit(&var); }
    ~ScopedPropVariant() { PropVariantClear(&var); }
    Q_DISABLE_COPY_MOVE(ScopedPropVariant)

    PROPVARIANT *get() { return &var; }
    PROPVARIANT *operator->() { return &var; }
    PROPVARIANT &operator*() { return var; }
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QWINDOWS_SCOPEDPROPVARIANT_P_H
