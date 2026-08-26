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

inline bool isOhosPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "ohos"_L1;
}

inline bool isFFMPEGPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "ffmpeg"_L1;
}

inline bool isWMFPlatform()
{
    return QPlatformMediaIntegration::instance()->name() == "windows"_L1;
}

[[nodiscard]] inline bool isMediaBackendPluginLoaded()
{
    return QPlatformMediaIntegration::instance()->name() != "fallback"_L1;
}

inline void initIntegrationTestMain()
{
    QString backend = qEnvironmentVariable("QT_MEDIA_BACKEND");
    if (backend.isEmpty())
        backend = QPlatformMediaIntegration::defaultBackend();
    QPlatformMediaIntegration::setBackend(backend);
}

// Returns true if initialization is successful
[[nodiscard]] inline bool initIntegrationTestCase()
{
    // VERIFY must be inside a void-returning function.
    []() {
        QVERIFY2(
            isMediaBackendPluginLoaded(),
            "No media backend plugin was loaded; the fallback integration was used. "
            "Integration tests require a real backend plugin.");
    }();
    return !QTest::currentTestFailed();
}

inline bool isRhiRenderingSupported()
{
    return QGuiApplicationPrivate::platformIntegration()->hasCapability(
            QPlatformIntegration::RhiBasedRendering);
}

inline bool isOpenGLSupported()
{
    return QGuiApplicationPrivate::platformIntegration()->hasCapability(
            QPlatformIntegration::OpenGL);
}

inline bool isCI()
{
    return qEnvironmentVariable("QTEST_ENVIRONMENT").toLower().split(u' ').contains(u"ci"_s);
}

} // namespace BackendUtilsImpl

using namespace BackendUtilsImpl;

#define QSKIP_IF(checker, defaultMessage, /*messageOpt*/...)                 \
    do {                                                                     \
        if (checker)                                                         \
            QSKIP(strlen(__VA_ARGS__ "") ? __VA_ARGS__ "" : defaultMessage); \
    } while (0)

#define QSKIP_GSTREAMER(/*messageOpt*/...) \
    QSKIP_IF(isGStreamerPlatform(), "The feature is not supported on GStreamer", __VA_ARGS__)

#define QSKIP_IF_NOT_FFMPEG(/*messageOpt*/...) \
    QSKIP_IF(!isFFMPEGPlatform(), "The feature is only supported on FFmpeg", __VA_ARGS__)

#define QSKIP_FFMPEG(/*messageOpt*/...) \
    QSKIP_IF(isFFMPEGPlatform(), "The feature is not supported on FFmpeg", __VA_ARGS__)

#define QSKIP_DARWIN(/*messageOpt*/...) \
    QSKIP_IF(isDarwinPlatform(), "The feature is not supported on Darwin", __VA_ARGS__)

#define QSKIP_OHOS(/*messageOpt*/...) \
    QSKIP_IF(isOhosPlatform(), "The feature is not supported on HarmonyOS", __VA_ARGS__)

#define QSKIP_WMF(/*messageOpt*/...) \
    QSKIP_IF(isWMFPlatform(), "The feature is not supported on Windows MediaEngine", __VA_ARGS__)

#define QEXPECT_FAIL_GSTREAMER(dataIndex, comment, mode) \
  do {                                                   \
    if (isGStreamerPlatform())                           \
      QEXPECT_FAIL(dataIndex, comment, mode);            \
  } while (0)

#endif // MEDIABACKENDUTILS_H
