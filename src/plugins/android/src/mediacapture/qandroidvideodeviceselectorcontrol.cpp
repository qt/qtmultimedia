/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qandroidvideodeviceselectorcontrol.h"

#include "qandroidcamerasession.h"
#include "androidcamera.h"

QT_BEGIN_NAMESPACE

QAndroidVideoDeviceSelectorControl::QAndroidVideoDeviceSelectorControl(QAndroidCameraSession *session)
    : QVideoDeviceSelectorControl(0)
    , m_selectedDevice(0)
    , m_cameraSession(session)
{
}

QAndroidVideoDeviceSelectorControl::~QAndroidVideoDeviceSelectorControl()
{
}

int QAndroidVideoDeviceSelectorControl::deviceCount() const
{
    return QAndroidCameraSession::availableCameras().count();
}

QString QAndroidVideoDeviceSelectorControl::deviceName(int index) const
{
    if (index < 0 || index >= QAndroidCameraSession::availableCameras().count())
        return QString();

    return QString::fromLatin1(QAndroidCameraSession::availableCameras().at(index).name);
}

QString QAndroidVideoDeviceSelectorControl::deviceDescription(int index) const
{
    if (index < 0 || index >= QAndroidCameraSession::availableCameras().count())
        return QString();

    return QAndroidCameraSession::availableCameras().at(index).description;
}

int QAndroidVideoDeviceSelectorControl::defaultDevice() const
{
    return 0;
}

int QAndroidVideoDeviceSelectorControl::selectedDevice() const
{
    return m_selectedDevice;
}

void QAndroidVideoDeviceSelectorControl::setSelectedDevice(int index)
{
    if (index != m_selectedDevice) {
        m_selectedDevice = index;
        m_cameraSession->setSelectedCamera(m_selectedDevice);
        emit selectedDeviceChanged(index);
        emit selectedDeviceChanged(deviceName(index));
    }
}

QT_END_NAMESPACE
