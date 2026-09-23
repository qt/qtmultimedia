// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlibcamera_videodevices_p.h"
#include "qlibcamera_format_support_p.h"
#include "qlibcamera_instance_p.h"

#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/geometry.h>
#include <libcamera/pixel_format.h>
#include <libcamera/property_ids.h>
#include <libcamera/stream.h>

QT_BEGIN_NAMESPACE

namespace QLibCamera {

using namespace libcamera;

std::unique_ptr<QLibCameraVideoDevices>
QLibCameraVideoDevices::create(QPlatformMediaIntegration *integration)
{
    std::shared_ptr<QLibCameraInstance> instance = QLibCameraInstance::instance();
    if (!instance)
        return nullptr; // CameraManager::start() failed -- caller must fall back.

    return std::unique_ptr<QLibCameraVideoDevices>(
            new QLibCameraVideoDevices(integration, std::move(instance)));
}

QLibCameraVideoDevices::QLibCameraVideoDevices(QPlatformMediaIntegration *integration,
                                               std::shared_ptr<QLibCameraInstance> instance)
    : QPlatformVideoDevices(integration), m_instance(std::move(instance))
{
    connect(m_instance.get(), &QLibCameraInstance::cameraAdded, this,
            &QLibCameraVideoDevices::onVideoInputsChanged);
    connect(m_instance.get(), &QLibCameraInstance::cameraRemoved, this,
            &QLibCameraVideoDevices::onVideoInputsChanged);
}

QLibCameraVideoDevices::~QLibCameraVideoDevices() = default;

static QCameraDevice::Position positionFromLibcamera(const ControlList &properties)
{
    const std::optional<int32_t> location = properties.get(properties::Location);
    if (!location)
        return QCameraDevice::UnspecifiedPosition;

    switch (*location) {
    case properties::CameraLocationFront:
        return QCameraDevice::FrontFace;
    case properties::CameraLocationBack:
        return QCameraDevice::BackFace;
    default:
        return QCameraDevice::UnspecifiedPosition;
    }
}

static void appendFormatsForSize(QList<QCameraFormat> &formats,
                                 QVideoFrameFormat::PixelFormat pixelFormat, const QSize &size,
                                 float minFrameRate, float maxFrameRate)
{
    auto *f = new QCameraFormatPrivate{
        QSharedData(), pixelFormat, size, minFrameRate, maxFrameRate,
    };
    formats << f->create();
}

QList<QCameraDevice> QLibCameraVideoDevices::findVideoInputs() const
{
    using namespace QLibCameraFormatSupport;
    QList<QCameraDevice> devices;

    CameraManager *manager = m_instance->manager();
    if (!manager)
        return devices;

    for (const std::shared_ptr<Camera> &camera : manager->cameras()) {
        auto *info = new QCameraDevicePrivate;
        info->id = QByteArray::fromStdString(camera->id());

        const ControlList &properties = camera->properties();
        if (const std::optional<std::string_view> model = properties.get(properties::Model))
            info->description = QString::fromUtf8(model->data(), qsizetype(model->size()));
        else
            info->description = QString::fromStdString(camera->id());

        info->isDefault = devices.isEmpty();
        info->position = positionFromLibcamera(properties);
        if (const std::optional<int32_t> rotation = properties.get(properties::Rotation))
            info->orientation = *rotation;

        const auto [minFrameRate, maxFrameRate] = frameRateRangeFromControls(camera->controls());

        std::unique_ptr<CameraConfiguration> config =
                camera->generateConfiguration({ StreamRole::VideoRecording });
        if (config && !config->empty()) {
            const StreamConfiguration &streamConfig = config->at(0);
            for (const PixelFormat &pixelFormat : streamConfig.formats().pixelformats()) {
                const std::optional<QVideoFrameFormat::PixelFormat> qtFormat =
                        pixelFormatFromLibcamera(pixelFormat);
                if (!qtFormat)
                    continue; // e.g. Bayer raw formats -- correctly unsupported.

                const std::vector sizes = sizesForPixelFormat(streamConfig.formats(), pixelFormat);
                for (const QSize &size : sizes)
                    appendFormatsForSize(info->videoFormats, *qtFormat, size, minFrameRate,
                                         maxFrameRate);
            }
        }

        devices << info->create();
    }

    return devices;
}

} // namespace QLibCamera

QT_END_NAMESPACE

#include "moc_qlibcamera_videodevices_p.cpp"
