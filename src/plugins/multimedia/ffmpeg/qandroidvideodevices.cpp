// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidvideodevices_p.h"
#include "qandroidcameraframe_p.h"

#include <private/qcameradevice_p.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qandroidextras_p.h>
#include <QtCore/qcoreapplication_platform.h>
#include <QtCore/qjnienvironment.h>
#include <jni.h>

static Q_LOGGING_CATEGORY(qLCAndroidVideoDevices, "qt.multimedia.ffmpeg.android.videoDevices")

QCameraFormat createCameraFormat(int width, int height, int fpsMin, int fpsMax)
{
    QCameraFormatPrivate *format = new QCameraFormatPrivate();

    format->resolution = { width, height };

    format->minFrameRate = fpsMin;
    format->maxFrameRate = fpsMax;

    format->pixelFormat = QVideoFrameFormat::PixelFormat::Format_YUV420P;

    return format->create();
}

QList<QCameraDevice> QAndroidVideoDevices::findVideoDevices()
{
    QList<QCameraDevice> devices;

    QJniObject deviceManager(QtJniTypes::Traits<QtJniTypes::QtVideoDeviceManager>::className(),
                             QNativeInterface::QAndroidApplication::context());

    if (!deviceManager.isValid()) {
        qCWarning(qLCAndroidVideoDevices) << "Failed to connect to Qt Video Device Manager.";
        return devices;
    }

    const QJniArray cameraIdList = deviceManager.callMethod<QtJniTypes::String[]>("getCameraIdList");
    if (!cameraIdList.isValid())
        return devices;

    int cameraIndex = 0;
    for (const auto &cameraId : cameraIdList) {
        if (!cameraId.isValid())
            continue;

        QCameraDevicePrivate *info = new QCameraDevicePrivate;
        info->id = cameraId.toString().toUtf8();

        info->orientation = deviceManager.callMethod<jint>("getSensorOrientation", cameraId);

        int facing = deviceManager.callMethod<jint>("getLensFacing", cameraId);

        const int LENS_FACING_FRONT = 0;
        const int LENS_FACING_BACK = 1;
        const int LENS_FACING_EXTERNAL = 2;

        switch (facing) {
        case LENS_FACING_EXTERNAL:
        case LENS_FACING_BACK:
            info->position = QCameraDevice::BackFace;
            info->description = QString("Rear Camera: %1").arg(cameraIndex);
            break;
        case LENS_FACING_FRONT:
            info->position = QCameraDevice::FrontFace;
            info->description = QString("Front Camera: %1").arg(cameraIndex);
            break;
        }
        ++cameraIndex;

        const auto fpsRanges = deviceManager.callMethod<QStringList>("getFpsRange", cameraId);

        int maxFps = 0, minFps = 0;
        for (auto range : fpsRanges) {
            range = range.remove("[");
            range = range.remove("]");

            const auto split = range.split(",");

            int min = split.at(0).toInt();
            int max = split.at(1).toInt();

            if (max > maxFps) {
                maxFps = max;
                minFps = min;
            }
        }

        const static int imageFormat =
                QJniObject::getStaticField<QtJniTypes::AndroidImageFormat, jint>("YUV_420_888");

        const QStringList sizes = deviceManager.callMethod<QStringList>(
                "getStreamConfigurationsSizes", cameraId, imageFormat);

        if (sizes.isEmpty())
            continue;

        for (const auto &sizeString : sizes) {
            const auto split = sizeString.split("x");

            int width = split.at(0).toInt();
            int height = split.at(1).toInt();

            info->videoFormats.append(createCameraFormat(width, height, minFps, maxFps));
        }

        devices.push_back(info->create());
    }

    return devices;
}
