// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qsymbolsresolveutils_p.h>

#include <va/va.h>
#include <va/va_str.h>

// VAAPI generated the actual *.so name due to the rule:
// https://github.com/intel/libva/blob/master/configure.ac
//
// The library name is generated libva.<x>.<y>.0 where
// <x> = VA-API major version + 1
// <y> = 100 * VA-API minor version + VA-API micro version
CHECK_VERSIONS("va", VA_NEEDED_SOVERSION, VA_MAJOR_VERSION + 1);

#ifdef Q_FFMPEG_PLUGIN_STUBS_ONLY
constexpr const char *loggingName = "va(in plugin)";
#else
constexpr const char *loggingName = nullptr;
#endif

BEGIN_INIT_FUNCS("va", VA_NEEDED_SOVERSION, loggingName)


INIT_FUNC(vaExportSurfaceHandle);
INIT_FUNC(vaSyncSurface);
INIT_FUNC(vaQueryVendorString);

#ifndef Q_FFMPEG_PLUGIN_STUBS_ONLY

INIT_FUNC(vaInitialize);
INIT_FUNC(vaTerminate);
INIT_FUNC(vaErrorStr);
INIT_FUNC(vaSetErrorCallback);
INIT_FUNC(vaSetInfoCallback);

INIT_FUNC(vaCreateImage);
INIT_FUNC(vaGetImage);
INIT_FUNC(vaPutImage);
INIT_FUNC(vaDeriveImage);
INIT_FUNC(vaDestroyImage);
INIT_FUNC(vaQueryImageFormats);

INIT_FUNC(vaBeginPicture);
INIT_FUNC(vaRenderPicture);
INIT_FUNC(vaEndPicture);

INIT_FUNC(vaCreateBuffer);
INIT_FUNC(vaMapBuffer);
INIT_FUNC(vaUnmapBuffer);
#if VA_CHECK_VERSION(1, 9, 0)
INIT_FUNC(vaSyncBuffer);
#endif
INIT_FUNC(vaDestroyBuffer);

INIT_FUNC(vaCreateSurfaces);
INIT_FUNC(vaDestroySurfaces);

INIT_FUNC(vaCreateConfig);
INIT_FUNC(vaGetConfigAttributes);
INIT_FUNC(vaMaxNumProfiles);
INIT_FUNC(vaMaxNumImageFormats);
INIT_FUNC(vaMaxNumEntrypoints);
INIT_FUNC(vaQueryConfigProfiles);
INIT_FUNC(vaQueryConfigEntrypoints);
INIT_FUNC(vaQuerySurfaceAttributes);
INIT_FUNC(vaDestroyConfig);

INIT_FUNC(vaCreateContext);
INIT_FUNC(vaDestroyContext);

INIT_FUNC(vaProfileStr);
INIT_FUNC(vaEntrypointStr);

INIT_FUNC(vaGetDisplayAttributes);

INIT_FUNC(vaSetDriverName);

INIT_FUNC(vaAcquireBufferHandle);
INIT_FUNC(vaReleaseBufferHandle);

#endif

END_INIT_FUNCS()

constexpr auto emptyString = "";

DEFINE_FUNC(vaExportSurfaceHandle, 5, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaSyncSurface, 2, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaQueryVendorString, 1, emptyString);

#ifndef Q_FFMPEG_PLUGIN_STUBS_ONLY

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


DEFINE_FUNC(vaProfileStr, 1, emptyString);
DEFINE_FUNC(vaEntrypointStr, 1, emptyString);

DEFINE_FUNC(vaGetDisplayAttributes, 3, VA_STATUS_ERROR_OPERATION_FAILED);

DEFINE_FUNC(vaSetDriverName, 2, VA_STATUS_ERROR_OPERATION_FAILED);

DEFINE_FUNC(vaAcquireBufferHandle, 3, VA_STATUS_ERROR_OPERATION_FAILED);
DEFINE_FUNC(vaReleaseBufferHandle, 2, VA_STATUS_ERROR_OPERATION_FAILED);

#endif

