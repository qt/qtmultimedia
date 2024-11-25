// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MEDIABACKENDUTILS_H
#define MEDIABACKENDUTILS_H

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

#include <QtTest/qtestcase.h>
#include <private/qplatformmediaintegration_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

namespace BackendUtilsImpl {

using namespace Qt::Literals;

inline bool isGStreamerPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "gstreamer"_L1;
}

inline bool isQNXPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "qnx"_L1;
}

inline bool isDarwinPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "darwin"_L1;
}

inline bool isAndroidPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "android"_L1;
}

inline bool isFFMPEGPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "ffmpeg"_L1;
}

inline bool isWindowsPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "windows"_L1;
}

inline bool isRhiRenderingSupported()
{
    return QGuiApplicationPrivate::platformIntegration()->hasCapability(
            QPlatformIntegration::RhiBasedRendering);
}

inline bool isCI()
{
    return qEnvironmentVariable("QTEST_ENVIRONMENT").toLower().split(u' ').contains(u"ci"_s);
}

} // namespace BackendUtilsImpl

using namespace BackendUtilsImpl;

#define QSKIP_GSTREAMER(message) \
  do {                           \
    if (isGStreamerPlatform())   \
      QSKIP(message);            \
  } while (0)

#define QSKIP_IF_NOT_FFMPEG()                             \
    do {                                                  \
        if (!isFFMPEGPlatform())                          \
            QSKIP("Feature is only supported on FFmpeg"); \
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
