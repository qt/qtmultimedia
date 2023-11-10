// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qlibrary.h>

#include "qffmpegsymbolsresolveutils_p.h"

#include <QtCore/qglobal.h>
#include <qstringliteral.h>

#include <va/va.h>
#ifdef DYNAMIC_RESOLVE_VA_DRM_SYMBOLS
#include <va/va_drm.h>
#endif
#ifdef DYNAMIC_RESOLVE_VA_X11_SYMBOLS
#include <va/va_x11.h>
#endif
#include <va/va_str.h>

QT_BEGIN_NAMESPACE

static Libs loadLibs()
{
    Libs libs;
    libs.push_back(std::make_unique<QLibrary>("va"));
#ifdef DYNAMIC_RESOLVE_VA_DRM_SYMBOLS
    libs.push_back(std::make_unique<QLibrary>("va-drm"));
#endif

#ifdef DYNAMIC_RESOLVE_VA_X11_SYMBOLS
    libs.push_back(std::make_unique<QLibrary>("va-x11"));
#endif

    if (LibSymbolsResolver::tryLoad(libs))
        return libs;

    return {};
}

constexpr size_t symbolsCount = 38
#if VA_CHECK_VERSION(1, 9, 0)
        + 1
#endif
#ifdef DYNAMIC_RESOLVE_VA_DRM_SYMBOLS
        + 1
#endif
#ifdef DYNAMIC_RESOLVE_VA_X11_SYMBOLS
        + 1
#endif
        ;

Q_GLOBAL_STATIC(LibSymbolsResolver, resolver, "VAAPI", symbolsCount, loadLibs);

void resolveVAAPI()
{
    resolver()->resolve();
}

QT_END_NAMESPACE

QT_USE_NAMESPACE

DEFINE_FUNC(vaInitialize, 3, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaTerminate, 1, VA_STATUS_ERROR_OPERATION_FAILED);

constexpr auto errorStr = "VAAPI is not available";
DEFINE_FUNC(vaErrorStr, 1, errorStr);
DEFINE_FUNC(vaSetErrorCallback, 3);
DEFINE_FUNC(vaSetInfoCallback, 3);

DEFINE_FUNC(vaCreateImage, 5, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaGetImage, 7, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaPutImage, 11, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaDeriveImage, 3, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaDestroyImage, 2, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaQueryImageFormats, 3, VA_STATUS_ERROR_OPERATION_FAILED);

DEFINE_FUNC(vaBeginPicture, 3, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaRenderPicture, 4, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaEndPicture, 2, VA_STATUS_ERROR_OPERATION_FAILED);

DEFINE_FUNC(vaCreateBuffer, 7, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaMapBuffer, 3, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaUnmapBuffer, 2, VA_STATUS_ERROR_OPERATION_FAILED);
#if VA_CHECK_VERSION(1, 9, 0)
DEFINE_FUNC(vaSyncBuffer, 3, VA_STATUS_ERROR_OPERATION_FAILED);
#endif
DEFINE_FUNC(vaDestroyBuffer, 2, VA_STATUS_ERROR_OPERATION_FAILED);

DEFINE_FUNC(vaCreateSurfaces, 8, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaSyncSurface, 2, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaExportSurfaceHandle, 5, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaDestroySurfaces, 3, VA_STATUS_ERROR_OPERATION_FAILED);

DEFINE_FUNC(vaCreateConfig, 6, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaGetConfigAttributes, 5, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaMaxNumProfiles, 1);
DEFINE_FUNC(vaMaxNumImageFormats, 1);
DEFINE_FUNC(vaMaxNumEntrypoints, 1);
DEFINE_FUNC(vaQueryConfigProfiles, 3, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaQueryConfigEntrypoints, 4, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaQuerySurfaceAttributes, 4, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaDestroyConfig, 2, VA_STATUS_ERROR_OPERATION_FAILED);

DEFINE_FUNC(vaCreateContext, 8);
DEFINE_FUNC(vaDestroyContext, 2);

constexpr auto emptyString = "";
DEFINE_FUNC(vaQueryVendorString, 1, emptyString);
DEFINE_FUNC(vaProfileStr, 1, emptyString);
DEFINE_FUNC(vaEntrypointStr, 1, emptyString);

DEFINE_FUNC(vaGetDisplayAttributes, 3, VA_STATUS_ERROR_OPERATION_FAILED);

DEFINE_FUNC(vaSetDriverName, 2, VA_STATUS_ERROR_OPERATION_FAILED);

#ifdef DYNAMIC_RESOLVE_VA_DRM_SYMBOLS
DEFINE_FUNC(vaGetDisplayDRM, 1); // va-drm
#endif

#ifdef DYNAMIC_RESOLVE_VA_X11_SYMBOLS
DEFINE_FUNC(vaGetDisplay, 1); // va-x11
#endif
