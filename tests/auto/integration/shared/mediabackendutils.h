// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MEDIABACKENDUTILS_H
#define MEDIABACKENDUTILS_H

#include <QtTest/qtestcase.h>
#include <private/qplatformmediaintegration_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

inline bool isGStreamerPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "gstreamer";
}

inline bool isQNXPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "qnx";
}

inline bool isDarwinPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "darwin";
}

inline bool isAndroidPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "android";
}

inline bool isFFMPEGPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "ffmpeg";
}

inline bool isWindowsPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "windows";
}

inline bool isRhiRenderingSupported()
{
    return QGuiApplicationPrivate::platformIntegration()->hasCapability(
            QPlatformIntegration::RhiBasedRendering);
}

inline bool isCI()
{
    return qEnvironmentVariable("QTEST_ENVIRONMENT").toLower() == "ci";
}

#define QSKIP_GSTREAMER(message) \
  do {                           \
    if (isGStreamerPlatform())   \
      QSKIP(message);            \
  } while (0)

#define QSKIP_FFMPEG(message) \
  do {                        \
    if (isFFMPEGPlatform())   \
      QSKIP(message);         \
  } while (0)

#define QEXPECT_FAIL_GSTREAMER(dataIndex, comment, mode) \
  do {                                                   \
    if (isGStreamerPlatform())                           \
      QEXPECT_FAIL(dataIndex, comment, mode);            \
  } while (0)

#endif // MEDIABACKENDUTILS_H
