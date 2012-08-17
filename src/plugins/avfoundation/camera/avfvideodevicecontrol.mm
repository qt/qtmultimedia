/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "avfcameradebug.h"
#include "avfvideodevicecontrol.h"
#include "avfcameraservice.h"

QT_USE_NAMESPACE

AVFVideoDeviceControl::AVFVideoDeviceControl(AVFCameraService *service, QObject *parent)
   : QVideoDeviceSelectorControl(parent)
   , m_service(service)
   , m_selectedDevice(0)
   , m_dirty(true)
{
    NSArray *videoDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in videoDevices) {
        m_devices << QString::fromUtf8([[device uniqueID] UTF8String]);
        m_deviceDescriptions << QString::fromUtf8([[device localizedName] UTF8String]);
    }
}

AVFVideoDeviceControl::~AVFVideoDeviceControl()
{
}

int AVFVideoDeviceControl::deviceCount() const
{
    return m_devices.size();
}

QString AVFVideoDeviceControl::deviceName(int index) const
{
    return m_devices[index];
}

QString AVFVideoDeviceControl::deviceDescription(int index) const
{
    return m_deviceDescriptions[index];
}

int AVFVideoDeviceControl::defaultDevice() const
{
    return 0;
}

int AVFVideoDeviceControl::selectedDevice() const
{
    return m_selectedDevice;
}

void AVFVideoDeviceControl::setSelectedDevice(int index)
{
    if (index != m_selectedDevice) {
        m_dirty = true;
        m_selectedDevice = index;
        Q_EMIT selectedDeviceChanged(index);
        Q_EMIT selectedDeviceChanged(m_devices[index]);
    }
}

AVCaptureDevice *AVFVideoDeviceControl::createCaptureDevice()
{
    m_dirty = false;
    AVCaptureDevice *device = 0;

    if (!m_devices.isEmpty()) {
        QString deviceId = m_devices.at(m_selectedDevice);

        device = [AVCaptureDevice deviceWithUniqueID:
                    [NSString stringWithUTF8String:
                        deviceId.toUtf8().constData()]];
    }

    if (!device)
        device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];

    return device;
}

#include "moc_avfvideodevicecontrol.cpp"
