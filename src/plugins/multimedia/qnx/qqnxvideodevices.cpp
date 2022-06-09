/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

QList<QCameraDevice> QQnxVideoDevices::videoDevices() const
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
