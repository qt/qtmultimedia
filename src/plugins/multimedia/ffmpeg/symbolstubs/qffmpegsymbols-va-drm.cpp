// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qsymbolsresolveutils_p.h>

#include <va/va_drm.h>

CHECK_VERSIONS("va-drm", VA_DRM_NEEDED_SOVERSION, VA_MAJOR_VERSION + 1);

BEGIN_INIT_FUNCS("va-drm", VA_DRM_NEEDED_SOVERSION)
INIT_FUNC(vaGetDisplayDRM)
END_INIT_FUNCS()

DEFINE_FUNC(vaGetDisplayDRM, 1);
