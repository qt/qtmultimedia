// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMPTR_P_H
#define QCOMPTR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qt_windows.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

template<typename T, typename... Args>
ComPtr<T> makeComObject(Args &&...args)
{
    ComPtr<T> p;
    // Don't use Attach because of MINGW64 bug
    // #892 Microsoft::WRL::ComPtr::Attach leaks references
    *p.GetAddressOf() = new T(std::forward<Args>(args)...);
    return p;
}

#endif
