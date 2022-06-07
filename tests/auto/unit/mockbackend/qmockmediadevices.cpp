// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmockmediadevices_p.h"
#include "private/qcameradevice_p.h"

QT_BEGIN_NAMESPACE

QMockMediaDevices::QMockMediaDevices()
    : QPlatformMediaDevices()
{
    setDevices(this);

    QCameraDevicePrivate *info = new QCameraDevicePrivate;
    info->description = QString::fromUtf8("defaultCamera");
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
    info->description = QString::fromUtf8("frontCamera");
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
    info->description = QString::fromUtf8("backCamera");
    info->id = "back";
    info->isDefault = false;
    info->position = QCameraDevice::BackFace;
    m_cameraDevices.append(info->create());

}

QMockMediaDevices::~QMockMediaDevices() = default;

QList<QAudioDevice> QMockMediaDevices::audioInputs() const
{
    return m_inputDevices;
}

QList<QAudioDevice> QMockMediaDevices::audioOutputs() const
{
    return m_outputDevices;
}

QList<QCameraDevice> QMockMediaDevices::videoInputs() const
{
    return m_cameraDevices;
}

QPlatformAudioSource *QMockMediaDevices::createAudioSource(const QAudioDevice &info)
{
    Q_UNUSED(info);
    return nullptr;// ###
}

QPlatformAudioSink *QMockMediaDevices::createAudioSink(const QAudioDevice &info)
{
    Q_UNUSED(info);
    return nullptr; //###
}


QT_END_NAMESPACE
