// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qsymbolsresolveutils_p.h>

#include <va/va_x11.h>

CHECK_VERSIONS("va-x11", VA_X11_NEEDED_SOVERSION, VA_MAJOR_VERSION + 1);

BEGIN_INIT_FUNCS("va-x11", VA_X11_NEEDED_SOVERSION)
INIT_FUNC(vaGetDisplay)
END_INIT_FUNCS()

DEFINE_FUNC(vaGetDisplay, 1);
