/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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
****************************************************************************/

#include "avfcameradebug.h"
#include "avfcameradevicecontrol.h"
#include "avfcameraservice.h"
#include "avfcamerasession.h"

QT_USE_NAMESPACE

AVFCameraDeviceControl::AVFCameraDeviceControl(AVFCameraService *service, QObject *parent)
   : QVideoDeviceSelectorControl(parent)
   , m_service(service)
   , m_selectedDevice(0)
   , m_dirty(true)
{
    Q_UNUSED(m_service);
}

AVFCameraDeviceControl::~AVFCameraDeviceControl()
{
}

int AVFCameraDeviceControl::deviceCount() const
{
    return AVFCameraSession::availableCameraDevices().count();
}

QString AVFCameraDeviceControl::deviceName(int index) const
{
    const QList<AVFCameraInfo> &devices = AVFCameraSession::availableCameraDevices();
    if (index < 0 || index >= devices.count())
        return QString();

    return QString::fromUtf8(devices.at(index).deviceId);
}

QString AVFCameraDeviceControl::deviceDescription(int index) const
{
    const QList<AVFCameraInfo> &devices = AVFCameraSession::availableCameraDevices();
    if (index < 0 || index >= devices.count())
        return QString();

    return devices.at(index).description;
}

int AVFCameraDeviceControl::defaultDevice() const
{
    return AVFCameraSession::defaultCameraIndex();
}

int AVFCameraDeviceControl::selectedDevice() const
{
    return m_selectedDevice;
}

void AVFCameraDeviceControl::setSelectedDevice(int index)
{
    if (index >= 0 &&
            index < deviceCount() &&
            index != m_selectedDevice) {
        m_dirty = true;
        m_selectedDevice = index;
        Q_EMIT selectedDeviceChanged(index);
        Q_EMIT selectedDeviceChanged(deviceName(index));
    }
}

AVCaptureDevice *AVFCameraDeviceControl::createCaptureDevice()
{
    m_dirty = false;
    AVCaptureDevice *device = nullptr;

    QString deviceId = deviceName(m_selectedDevice);
    if (!deviceId.isEmpty()) {
        device = [AVCaptureDevice deviceWithUniqueID:
                    [NSString stringWithUTF8String:
                        deviceId.toUtf8().constData()]];
    }

    if (!device)
        device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];

    return device;
}

#include "moc_avfcameradevicecontrol.cpp"
