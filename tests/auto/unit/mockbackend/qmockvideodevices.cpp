// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmockvideodevices.h"
#include <private/qcameradevice_p.h>

QT_BEGIN_NAMESPACE

QMockVideoDevices::QMockVideoDevices(QPlatformMediaIntegration *mediaIntegration)
    : QPlatformVideoDevices(mediaIntegration)
{
    QCameraDevicePrivate *info = new QCameraDevicePrivate;
    info->description = QStringLiteral("defaultCamera");
    info->id = "default";
    info->isDefault = true;
    auto *f = new QCameraFormatPrivate{
        QSharedData(),
        QVideoFrameFormat::Format_ARGB8888,
        QSize(640, 480),
        0,
        30
    };
    info->videoFormats << f->create();
    m_cameraDevices.append(info->create());
    info = new QCameraDevicePrivate;
    info->description = QStringLiteral("frontCamera");
    info->id = "front";
    info->isDefault = false;
    info->position = QCameraDevice::FrontFace;
    f = new QCameraFormatPrivate{
        QSharedData(),
        QVideoFrameFormat::Format_XRGB8888,
        QSize(1280, 720),
        0,
        30
    };
    info->videoFormats << f->create();
    m_cameraDevices.append(info->create());
    info = new QCameraDevicePrivate;
    info->description = QStringLiteral("backCamera");
    info->id = "back";
    info->isDefault = false;
    info->position = QCameraDevice::BackFace;
    m_cameraDevices.append(info->create());
}

void QMockVideoDevices::addNewCamera()
{
    auto info = new QCameraDevicePrivate;
    info->description = QLatin1String("newCamera") + QString::number(m_cameraDevices.size());
    info->id =
            QString(QLatin1String("camera") + QString::number(m_cameraDevices.size())).toUtf8();
    info->isDefault = false;
    m_cameraDevices.append(info->create());

    onVideoInputsChanged();
}

QList<QCameraDevice> QMockVideoDevices::findVideoInputs() const
{
    ++m_findVideoInputsInvokeCount;
    return m_cameraDevices;
}

QT_END_NAMESPACE
