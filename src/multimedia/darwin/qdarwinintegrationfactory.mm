// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qdarwinintegrationfactory_p.h>

#include <QtMultimedia/private/qavfvideodevices_p.h>

QT_BEGIN_NAMESPACE

std::unique_ptr<QPlatformVideoDevices> makeQAvfVideoDevices(
    QPlatformMediaIntegration &integration,
    std::function<bool(uint32_t)> &&isCvPixelFormatSupportedDelegate)
{
    return std::make_unique<QAVFVideoDevices>(
        &integration,
        std::move(isCvPixelFormatSupportedDelegate));
}

QT_END_NAMESPACE
