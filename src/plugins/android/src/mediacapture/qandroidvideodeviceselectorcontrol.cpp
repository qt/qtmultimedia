/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidvideodeviceselectorcontrol.h"

#include "qandroidcamerasession.h"
#include "jcamera.h"

QT_BEGIN_NAMESPACE

QList<QByteArray> QAndroidVideoDeviceSelectorControl::m_names;
QStringList QAndroidVideoDeviceSelectorControl::m_descriptions;

QAndroidVideoDeviceSelectorControl::QAndroidVideoDeviceSelectorControl(QAndroidCameraSession *session)
    : QVideoDeviceSelectorControl(0)
    , m_selectedDevice(0)
    , m_cameraSession(session)
{
    if (m_names.isEmpty())
        update();
}

QAndroidVideoDeviceSelectorControl::~QAndroidVideoDeviceSelectorControl()
{
}

int QAndroidVideoDeviceSelectorControl::deviceCount() const
{
    return m_names.size();
}

QString QAndroidVideoDeviceSelectorControl::deviceName(int index) const
{
    return m_names.at(index);
}

QString QAndroidVideoDeviceSelectorControl::deviceDescription(int index) const
{
    return m_descriptions.at(index);
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

void QAndroidVideoDeviceSelectorControl::update()
{
    m_names.clear();
    m_descriptions.clear();

    QJNIObjectPrivate cameraInfo("android/hardware/Camera$CameraInfo");
    int numCameras = QJNIObjectPrivate::callStaticMethod<jint>("android/hardware/Camera",
                                                        "getNumberOfCameras");

    for (int i = 0; i < numCameras; ++i) {
        QJNIObjectPrivate::callStaticMethod<void>("android/hardware/Camera",
                                           "getCameraInfo",
                                           "(ILandroid/hardware/Camera$CameraInfo;)V",
                                           i, cameraInfo.object());

        JCamera::CameraFacing facing = JCamera::CameraFacing(cameraInfo.getField<jint>("facing"));

        switch (facing) {
        case JCamera::CameraFacingBack:
            m_names.append("back");
            m_descriptions.append(QStringLiteral("Rear-facing camera"));
            break;
        case JCamera::CameraFacingFront:
            m_names.append("front");
            m_descriptions.append(QStringLiteral("Front-facing camera"));
            break;
        default:
            break;
        }
    }
}

QList<QByteArray> QAndroidVideoDeviceSelectorControl::availableDevices()
{
    if (m_names.isEmpty())
        update();

    return m_names;
}

QString QAndroidVideoDeviceSelectorControl::availableDeviceDescription(const QByteArray &device)
{
    int i = m_names.indexOf(device);
    if (i != -1)
        return m_descriptions.at(i);

    return QString();
}

QT_END_NAMESPACE
