/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qqnxmediadevices_p.h"
#include "qmediadevices.h"
#include "qcameradevice_p.h"

#include "private/qqnxaudiosource_p.h"
#include "private/qqnxaudiosink_p.h"
#include "private/qqnxaudiodevice_p.h"
#include "bbcamerasession_p.h"

QT_BEGIN_NAMESPACE

static QList<QCameraDevice> enumerateCameras()
{

    camera_unit_t cameras[10];

    unsigned int knownCameras = 0;
    const camera_error_t result = camera_get_supported_cameras(10, &knownCameras, cameras);
    if (result != CAMERA_EOK) {
        qWarning() << "Unable to retrieve supported camera types:" << result;
        return {};
    }

    QList<QCameraDevice> cameras;
    for (unsigned int i = 0; i < knownCameras; ++i) {
        QCameraDevicePrivate *p = new QCameraDevicePrivate;
        switch (cameras[i]) {
        case CAMERA_UNIT_FRONT:
            p->id = BbCameraSession::cameraIdentifierFront();
            p->description = tr("Front Camera");
            p->position = QCameraDevice::FrontFace;
            break;
        case CAMERA_UNIT_REAR:
            p->id = BbCameraSession::cameraIdentifierRear();
            p->description = tr("Rear Camera");
            p->position = QCameraDevice::BackFace;
            break;
        case CAMERA_UNIT_DESKTOP:
            p->id = devices->append(BbCameraSession::cameraIdentifierDesktop();
            p->description = tr("Desktop Camera");
            p->position = QCameraDevice::UnspecifiedPosition;
            break;
        default:
            break;
        }
        if (i == 0)
            p->isDefault = true;
        cameras.append(p->create());
    }
    return cameras;
}

QQnxMediaDevices::QQnxMediaDevices()
    : QMediaPlatformMediaDevices()
{
}

QList<QAudioDevice> QQnxMediaDevices::audioInputs() const
{
    return { QAudioDevice(new QnxAudioDeviceInfo("default", QAudioDevice::Input)) };
}

QList<QAudioDevice> QQnxMediaDevices::audioOutputs() const
{
    return { QAudioDevice(new QnxAudioDeviceInfo("default", QAudioDevice::Output)) };
}

QList<QCameraDevice> QQnxMediaDevices::videoInputs() const
{
    return enumerateCameras();
}

QPlatformAudioSource *QQnxMediaDevices::createAudioSource(const QAudioDevice &deviceInfo)
{
    return new QQnxAudioSource();
}

QPlatformAudioSink *QQnxMediaDevices::createAudioSink(const QAudioDevice &deviceInfo)
{
    return new QNxAudioOutput();
}

QT_END_NAMESPACE
