// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosvideodevices_p.h"

#include "qohosglobal_p.h"

#include <private/qcameradevice_p.h>

#include <QtMultimedia/qcameradevice.h>
#include <QtMultimedia/qvideoframeformat.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

#include <ohcamera/camera.h>
#include <ohcamera/camera_manager.h>

#include <info/application_target_sdk_version.h>

QT_BEGIN_NAMESPACE

namespace {

QCameraDevice::Position positionFor(Camera_Position position)
{
    switch (position) {
    case CAMERA_POSITION_FRONT:
        return QCameraDevice::FrontFace;
    case CAMERA_POSITION_BACK:
        return QCameraDevice::BackFace;
    case CAMERA_POSITION_UNSPECIFIED:
        break;
    }
    return QCameraDevice::UnspecifiedPosition;
}

QString descriptionFor(const Camera_Device &device)
{
    QString position;
    switch (device.cameraPosition) {
    case CAMERA_POSITION_FRONT:
        position = QStringLiteral("Front");
        break;
    case CAMERA_POSITION_BACK:
        position = QStringLiteral("Back");
        break;
    default:
        position = QStringLiteral("Camera");
        break;
    }
    QString type;
    switch (device.cameraType) {
    case CAMERA_TYPE_WIDE_ANGLE:
        type = QStringLiteral("Wide");
        break;
    case CAMERA_TYPE_ULTRA_WIDE:
        type = QStringLiteral("UltraWide");
        break;
    case CAMERA_TYPE_TELEPHOTO:
        type = QStringLiteral("Telephoto");
        break;
    case CAMERA_TYPE_TRUE_DEPTH:
        type = QStringLiteral("TrueDepth");
        break;
    case CAMERA_TYPE_DEFAULT:
        break;
    }
    return type.isEmpty() ? position : QStringLiteral("%1 (%2)").arg(position, type);
}

QVideoFrameFormat::PixelFormat pixelFormatFor(Camera_Format format)
{
    switch (format) {
    case CAMERA_FORMAT_RGBA_8888:
        return QVideoFrameFormat::Format_RGBA8888;
    case CAMERA_FORMAT_YUV_420_SP:
        return QVideoFrameFormat::Format_NV12;
    case CAMERA_FORMAT_JPEG:
        return QVideoFrameFormat::Format_Jpeg;
    case CAMERA_FORMAT_YCBCR_P010:
    case CAMERA_FORMAT_YCRCB_P010:
#if OH_CURRENT_API_VERSION >= 23
    case CAMERA_FORMAT_HEIC:
#endif
        break;
    }
    return QVideoFrameFormat::Format_Invalid;
}

QList<QCameraFormat> collectVideoFormats(Camera_Manager *manager, const Camera_Device &device)
{
    QList<QCameraFormat> formats;
    Camera_OutputCapability *caps = nullptr;
    if (OH_CameraManager_GetSupportedCameraOutputCapability(manager,
                                                            const_cast<Camera_Device *>(&device),
                                                            &caps)
                != CAMERA_OK
        || !caps) {
        return formats;
    }

    for (uint32_t i = 0; i < caps->videoProfilesSize; ++i) {
        Camera_VideoProfile *profile = caps->videoProfiles[i];
        if (!profile)
            continue;
        const auto pixelFormat = pixelFormatFor(profile->format);
        if (pixelFormat == QVideoFrameFormat::Format_Invalid)
            continue;
        auto *priv = new QCameraFormatPrivate;
        priv->pixelFormat = pixelFormat;
        priv->resolution = QSize{ int(profile->size.width), int(profile->size.height) };
        priv->minFrameRate = float(profile->range.min);
        priv->maxFrameRate = float(profile->range.max);
        formats.append(priv->create());
    }

    OH_CameraManager_DeleteSupportedCameraOutputCapability(manager, caps);
    return formats;
}

} // namespace

QOhosVideoDevices::QOhosVideoDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration)
{
}

QList<QCameraDevice> QOhosVideoDevices::findVideoInputs() const
{
    Camera_Manager *manager = nullptr;
    if (OH_Camera_GetCameraManager(&manager) != CAMERA_OK || !manager)
        return {};

    Camera_Device *devices = nullptr;
    uint32_t count = 0;
    if (OH_CameraManager_GetSupportedCameras(manager, &devices, &count) != CAMERA_OK || !devices) {
        OH_Camera_DeleteCameraManager(manager);
        return {};
    }

    QList<QCameraDevice> result;
    result.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        const Camera_Device &dev = devices[i];
        auto *priv = new QCameraDevicePrivate;
        priv->id = QByteArray{ dev.cameraId };
        priv->description = descriptionFor(dev);
        priv->isDefault = (i == 0);
        priv->position = positionFor(dev.cameraPosition);
        priv->videoFormats = collectVideoFormats(manager, dev);
        result.append(priv->create());
    }

    OH_CameraManager_DeleteSupportedCameras(manager, devices, count);
    OH_Camera_DeleteCameraManager(manager);
    return result;
}

QT_END_NAMESPACE

#include "moc_qohosvideodevices_p.cpp"
