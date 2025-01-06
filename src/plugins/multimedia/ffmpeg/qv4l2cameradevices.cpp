// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4l2cameradevices_p.h"
#include "qv4l2filedescriptor_p.h"
#include "qv4l2camera_p.h"

#include <private/qcameradevice_p.h>
#include <private/qcore_unix_p.h>

#include <qdir.h>
#include <qfile.h>
#include <qdebug.h>
#include <qloggingcategory.h>

#include <linux/videodev2.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcV4L2CameraDevices, "qt.multimedia.ffmpeg.v4l2cameradevices");

static bool areCamerasEqual(QList<QCameraDevice> a, QList<QCameraDevice> b)
{
    auto areCamerasDataEqual = [](const QCameraDevice &a, const QCameraDevice &b) {
        Q_ASSERT(QCameraDevicePrivate::handle(a));
        Q_ASSERT(QCameraDevicePrivate::handle(b));
        return *QCameraDevicePrivate::handle(a) == *QCameraDevicePrivate::handle(b);
    };

    return std::equal(a.cbegin(), a.cend(), b.cbegin(), b.cend(), areCamerasDataEqual);
}

QV4L2CameraDevices::QV4L2CameraDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration)
{
    m_deviceWatcher.addPath(QLatin1String("/dev"));
    connect(&m_deviceWatcher, &QFileSystemWatcher::directoryChanged, this,
            &QV4L2CameraDevices::checkCameras);
    doCheckCameras();
}

QList<QCameraDevice> QV4L2CameraDevices::findVideoInputs() const
{
    return m_cameras;
}

void QV4L2CameraDevices::checkCameras()
{
    if (doCheckCameras())
        onVideoInputsChanged();
}

bool QV4L2CameraDevices::doCheckCameras()
{
    QList<QCameraDevice> newCameras;

    QDir dir(QLatin1String("/dev"));
    const auto devices = dir.entryList(QDir::System);

    bool first = true;

    for (auto device : devices) {
        //        qCDebug(qLcV4L2Camera) << "device:" << device;
        if (!device.startsWith(QLatin1String("video")))
            continue;

        QByteArray file = QFile::encodeName(dir.filePath(device));
        const int fd = open(file.constData(), O_RDONLY);
        if (fd < 0)
            continue;

        auto fileCloseGuard = qScopeGuard([fd]() { close(fd); });

        v4l2_fmtdesc formatDesc = {};

        struct v4l2_capability cap;
        if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
            continue;

        if (cap.device_caps & V4L2_CAP_META_CAPTURE)
            continue;
        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
            continue;
        if (!(cap.capabilities & V4L2_CAP_STREAMING))
            continue;

        auto camera = std::make_unique<QCameraDevicePrivate>();

        camera->id = file;
        camera->description = QString::fromUtf8((const char *)cap.card);
        qCDebug(qLcV4L2CameraDevices) << "found camera" << camera->id << camera->description;

        formatDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        while (!xioctl(fd, VIDIOC_ENUM_FMT, &formatDesc)) {
            auto pixelFmt = formatForV4L2Format(formatDesc.pixelformat);
            qCDebug(qLcV4L2CameraDevices) << "    " << pixelFmt;

            if (pixelFmt == QVideoFrameFormat::Format_Invalid) {
                ++formatDesc.index;
                continue;
            }

            qCDebug(qLcV4L2CameraDevices) << "frame sizes:";
            v4l2_frmsizeenum frameSize = {};
            frameSize.pixel_format = formatDesc.pixelformat;

            while (!xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frameSize)) {
                QList<QSize> resolutions;
                if (frameSize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                    resolutions.append(QSize(frameSize.discrete.width,
                                             frameSize.discrete.height));
                } else {
                    resolutions.append(QSize(frameSize.stepwise.max_width,
                                             frameSize.stepwise.max_height));
                    resolutions.append(QSize(frameSize.stepwise.min_width,
                                             frameSize.stepwise.min_height));
                }

                for (auto resolution : resolutions) {
                    float min = 1e10;
                    float max = 0;
                    auto updateMaxMinFrameRate = [&max, &min](auto discreteFrameRate) {
                        const float rate = float(discreteFrameRate.denominator)
                                           / float(discreteFrameRate.numerator);
                        if (rate > max)
                            max = rate;
                        if (rate < min)
                            min = rate;
                    };

                    v4l2_frmivalenum frameInterval = {};
                    frameInterval.pixel_format = formatDesc.pixelformat;
                    frameInterval.width = resolution.width();
                    frameInterval.height = resolution.height();

                    while (!xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frameInterval)) {
                        if (frameInterval.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                            updateMaxMinFrameRate(frameInterval.discrete);
                        } else {
                            updateMaxMinFrameRate(frameInterval.stepwise.max);
                            updateMaxMinFrameRate(frameInterval.stepwise.min);
                        }
                        ++frameInterval.index;
                    }

                    qCDebug(qLcV4L2CameraDevices) << "    " << resolution << min << max;

                    if (min <= max) {
                        auto fmt = std::make_unique<QCameraFormatPrivate>();
                        fmt->pixelFormat = pixelFmt;
                        fmt->resolution = resolution;
                        fmt->minFrameRate = min;
                        fmt->maxFrameRate = max;
                        camera->videoFormats.append(fmt.release()->create());
                        camera->photoResolutions.append(resolution);
                    }
                }
                ++frameSize.index;
            }
            ++formatDesc.index;
        }

        if (camera->videoFormats.empty())
            continue;

        // first camera is default
        camera->isDefault = std::exchange(first, false);

        newCameras.append(camera.release()->create());
    }

    if (areCamerasEqual(m_cameras, newCameras))
        return false;

    m_cameras = std::move(newCameras);
    return true;
}

QT_END_NAMESPACE

#include "moc_qv4l2cameradevices_p.cpp"
