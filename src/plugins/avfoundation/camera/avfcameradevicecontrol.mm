/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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
    AVCaptureDevice *device = 0;

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
