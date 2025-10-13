// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDARWININTEGRATIONFACTORY_H
#define QDARWININTEGRATIONFACTORY_H

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

#include <QtCore/qtconfigmacros.h>

#include <QtMultimedia/qtmultimediaexports.h>
#include <QtMultimedia/private/qplatformvideodevices_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE

[[nodiscard]] Q_MULTIMEDIA_EXPORT std::unique_ptr<QPlatformVideoDevices> makeQAvfVideoDevices(
    QPlatformMediaIntegration &,
    std::function<bool(uint32_t)> &&isCvPixelFormatSupportedDelegate = nullptr);

QT_END_NAMESPACE

#endif
