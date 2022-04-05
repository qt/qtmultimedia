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

#include "qqnxmediadevices_p.h"
#include "qmediadevices.h"
#include "private/qcameradevice_p.h"
#include "qcameradevice.h"

#include "qqnxaudiosource_p.h"
#include "qqnxaudiosink_p.h"
#include "qqnxaudiodevice_p.h"

#include <camera/camera_api.h>

#include <qdir.h>
#include <qdebug.h>

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

static QList<QCameraDevice> enumerateCameras()
{

    camera_unit_t cameraUnits[64];

    unsigned int knownCameras = 0;
    const camera_error_t result = camera_get_supported_cameras(64, &knownCameras, cameraUnits);
    if (result != CAMERA_EOK) {
        qWarning() << "Unable to retrieve supported camera types:" << result;
        return {};
    }

    QList<QCameraDevice> cameras;
    for (unsigned int i = 0; i < knownCameras; ++i) {
        QCameraDevicePrivate *p = new QCameraDevicePrivate;
        p->id = QByteArray::number(cameraUnits[i]);

        char name[CAMERA_LOCATION_NAMELEN];
        camera_get_location_property(cameraUnits[i], CAMERA_LOCATION_NAME, &name, CAMERA_LOCATION_END);
        p->description = QString::fromUtf8(name);

        if (i == 0)
            p->isDefault = true;

        camera_handle_t handle;
        if (camera_open(cameraUnits[i], CAMERA_MODE_PREAD, &handle) == CAMERA_EOK) {
            // query camera properties

            uint32_t nResolutions = 0;
            camera_get_supported_vf_resolutions(handle, 0, &nResolutions, nullptr);
            QVarLengthArray<camera_res_t> resolutions(nResolutions);
            camera_get_supported_vf_resolutions(handle, nResolutions, &nResolutions, resolutions.data());

            uint32_t nFrameTypes;
            camera_get_supported_vf_frame_types(handle, 0, &nFrameTypes, nullptr);
            QVarLengthArray<camera_frametype_t> frameTypes(nFrameTypes);
            camera_get_supported_vf_frame_types(handle, nFrameTypes, &nFrameTypes, frameTypes.data());

            for (auto res : resolutions) {
                QSize resolution(res.width, res.height);
                p->photoResolutions.append(resolution);

                for (auto frameType : frameTypes) {
                    auto pixelFormat = fromCameraFrametype(frameType);
                    if (pixelFormat == QVideoFrameFormat::Format_Invalid)
                        continue;

                    uint32_t nFrameRates;
                    camera_get_specified_vf_framerates(handle, frameType, res, 0, &nFrameRates, nullptr, nullptr);
                    QVarLengthArray<double> frameRates(nFrameRates);
                    bool continuous = false;
                    camera_get_specified_vf_framerates(handle, frameType, res, nFrameRates, &nFrameRates, frameRates.data(), &continuous);

                    QCameraFormatPrivate *f = new QCameraFormatPrivate;
                    f->resolution = resolution;
                    f->pixelFormat = pixelFormat;
                    f->minFrameRate = 1.e10;
                    for (auto fr : frameRates) {
                        if (fr < f->minFrameRate)
                            f->minFrameRate = fr;
                        if (fr > f->maxFrameRate)
                            f->maxFrameRate = fr;
                    }
                    p->videoFormats.append(f->create());
                }
            }
        }

        cameras.append(p->create());
    }
    return cameras;
}

static QList<QAudioDevice> enumeratePcmDevices(QAudioDevice::Mode mode)
{
    if (mode == QAudioDevice::Null)
        return {};

    QDir dir(QStringLiteral("/dev/snd"));

    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name);

    // QNX PCM devices names start with the pcm prefix and end either with the
    // 'p' (playback) or 'c' (capture) suffix

    const char modeSuffix = mode == QAudioDevice::Input ? 'c' : 'p';

    QList<QAudioDevice> devices;

    for (const QString &entry : dir.entryList()) {
        if (entry.startsWith(QStringLiteral("pcm")) && entry.back() == modeSuffix)
            devices << (new QnxAudioDeviceInfo(entry.toUtf8(), mode))->create();
    }

    return devices;
}

QQnxMediaDevices::QQnxMediaDevices(QPlatformMediaIntegration *integration)
    : QPlatformMediaDevices(integration)
{
}

QList<QAudioDevice> QQnxMediaDevices::audioInputs() const
{
    return ::enumeratePcmDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QQnxMediaDevices::audioOutputs() const
{
    return ::enumeratePcmDevices(QAudioDevice::Output);
}

QList<QCameraDevice> QQnxMediaDevices::videoInputs() const
{
    if (!camerasChecked) {
        camerasChecked = true;
        cameras = enumerateCameras();
    }
    return cameras;
}

QPlatformAudioSource *QQnxMediaDevices::createAudioSource(const QAudioDevice &/*deviceInfo*/)
{
    return new QQnxAudioSource();
}

QPlatformAudioSink *QQnxMediaDevices::createAudioSink(const QAudioDevice &deviceInfo)
{
    return new QQnxAudioSink(deviceInfo);
}

QT_END_NAMESPACE
