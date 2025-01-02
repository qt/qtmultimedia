// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGSYMBOLSRESOLVE_P_H
#define QFFMPEGSYMBOLSRESOLVE_P_H

#include "qnamespace.h"

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

QT_BEGIN_NAMESPACE

inline void resolveSymbols()
{
#ifdef DYNAMIC_RESOLVE_OPENSSL_SYMBOLS
    extern bool resolveOpenSsl();
    resolveOpenSsl();
#endif

#ifdef DYNAMIC_RESOLVE_VAAPI_SYMBOLS
    extern bool resolveVAAPI();
    resolveVAAPI();
#endif
}

QT_END_NAMESPACE

#endif // QFFMPEGSYMBOLSRESOLVE_P_H
