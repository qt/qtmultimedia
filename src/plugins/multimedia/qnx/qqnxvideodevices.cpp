// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxvideodevices_p.h"
#include "qqnxcamera_p.h"
#include "private/qcameradevice_p.h"
#include "qcameradevice.h"

#include <qdir.h>
#include <qdebug.h>

#include <optional>

QT_BEGIN_NAMESPACE

static QVideoFrameFormat::PixelFormat fromCameraFrametype(camera_frametype_t type)
{
    switch (type) {
    default:
    case CAMERA_FRAMETYPE_UNSPECIFIED:
        return QVideoFrameFormat::Format_Invalid;
    case CAMERA_FRAMETYPE_NV12:
        return QVideoFrameFormat::Format_NV12;
    case CAMERA_FRAMETYPE_RGB8888:
        return QVideoFrameFormat::Format_ARGB8888;
    case CAMERA_FRAMETYPE_JPEG:
         return QVideoFrameFormat::Format_Jpeg;
    case CAMERA_FRAMETYPE_GRAY8:
        return QVideoFrameFormat::Format_Y8;
    case CAMERA_FRAMETYPE_CBYCRY:
        return QVideoFrameFormat::Format_UYVY;
    case CAMERA_FRAMETYPE_YCBCR420P:
        return QVideoFrameFormat::Format_YUV420P;
    case CAMERA_FRAMETYPE_YCBYCR:
        return QVideoFrameFormat::Format_YUYV;
    }
}

static std::optional<QCameraDevice> createCameraDevice(camera_unit_t unit, bool isDefault)
{
    const QQnxCamera camera(unit);

    if (!camera.isValid()) {
        qWarning() << "Invalid camera unit:" << unit;
        return {};
    }

    auto *p = new QCameraDevicePrivate;

    p->id = QByteArray::number(camera.unit());
    p->description = camera.name();
    p->isDefault = isDefault;

    const QList<camera_frametype_t> frameTypes = camera.supportedVfFrameTypes();

    for (camera_res_t res : camera.supportedVfResolutions()) {
        const QSize resolution(res.width, res.height);

        p->photoResolutions.append(resolution);

        for (camera_frametype_t frameType : camera.supportedVfFrameTypes()) {
            const QVideoFrameFormat::PixelFormat pixelFormat = fromCameraFrametype(frameType);

            if (pixelFormat == QVideoFrameFormat::Format_Invalid)
                continue;

            auto *f = new QCameraFormatPrivate;
            p->videoFormats.append(f->create());

            f->resolution = resolution;
            f->pixelFormat = pixelFormat;
            f->minFrameRate = 1.e10;

            for (double fr : camera.specifiedVfFrameRates(frameType, res)) {
                if (fr < f->minFrameRate)
                    f->minFrameRate = fr;
                if (fr > f->maxFrameRate)
                    f->maxFrameRate = fr;
            }
        }
    }

    return p->create();
}

QQnxVideoDevices::QQnxVideoDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration)
{
}

QList<QCameraDevice> QQnxVideoDevices::findVideoInputs() const
{
    QList<QCameraDevice> cameras;

    bool isDefault = true;

    for (const camera_unit_t cameraUnit : QQnxCamera::supportedUnits()) {
        const std::optional<QCameraDevice> cameraDevice = createCameraDevice(cameraUnit, isDefault);

        if (!cameraDevice)
            continue;

        cameras.append(*cameraDevice);

        isDefault = false;
    }

    return cameras;
}

QT_END_NAMESPACE
