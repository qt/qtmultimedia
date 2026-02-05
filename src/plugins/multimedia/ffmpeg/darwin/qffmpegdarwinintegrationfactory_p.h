// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGDARWININTEGRATIONFACTORY_P_H
#define QFFMPEGDARWININTEGRATIONFACTORY_P_H

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

#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/private/qplatformcapturablewindows_p.h>
#include <QtMultimedia/private/qplatformimagecapture_p.h>
#include <QtMultimedia/private/qplatformsurfacecapture_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

[[nodiscard]] std::unique_ptr<QPlatformCamera> makeQAvfCamera(QCamera &);
[[nodiscard]] std::unique_ptr<QPlatformImageCapture> makeQAvfImageCapture(QImageCapture &);
[[nodiscard]] std::unique_ptr<QPlatformSurfaceCapture> makeQAvfScreenCapture();
[[nodiscard]] std::unique_ptr<QPlatformCapturableWindows> makeQCgCapturableWindows();
[[nodiscard]] std::unique_ptr<QPlatformSurfaceCapture> makeQCgWindowCapture();

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGDARWININTEGRATIONFACTORY_P_H
