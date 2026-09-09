// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_multimediaquick_qml_cameraformathelper.h"

#include <QtCore/qsize.h>

#include <QtMultimedia/private/qcameradevice_p.h>

QT_USE_NAMESPACE

QCameraFormat CameraFormatHelper::createCameraFormat(
    QSize resolution,
    QVideoFrameFormat::PixelFormat format,
    float minFrameRate,
    float maxFrameRate)
{
    auto *formatPrivate = new QCameraFormatPrivate;
    formatPrivate->pixelFormat = format;
    formatPrivate->resolution = resolution;
    formatPrivate->minFrameRate = minFrameRate;
    formatPrivate->maxFrameRate = maxFrameRate;
    return formatPrivate->create();
}

bool CameraFormatHelper::implicitlyConvertibleToQCameraFormat(const QCameraFormat &)
{
    // If we resolve correctly to this function, it means our QML cameraFormat
    // was implicitly convertible to QCameraFormat.
    return true;
}
