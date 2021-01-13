/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qvideodeviceselectorcontrol.h"
#include "private/qmediaplatformdevicemanager_p.h"
#include "private/qmediaplatformintegration_p.h"
#include "qcamerainfo.h"

QT_BEGIN_NAMESPACE

/*!
    \class QVideoDeviceSelectorControl
    \obsolete

    \brief The QVideoDeviceSelectorControl class provides an video device selector media control.
    \inmodule QtMultimedia


    \ingroup multimedia_control

    The QVideoDeviceSelectorControl class provides descriptions of the video devices
    available on a system and allows one to be selected as the  endpoint of a
    media service.

    The interface name of QVideoDeviceSelectorControl is \c org.qt-project.qt.videodeviceselectorcontrol/5.0 as
    defined in QVideoDeviceSelectorControl_iid.
*/

/*!
    \macro QVideoDeviceSelectorControl_iid

    \c org.qt-project.qt.videodeviceselectorcontrol/5.0

    Defines the interface name of the QVideoDeviceSelectorControl class.

    \relates QVideoDeviceSelectorControl
*/

/*!
    Constructs a video device selector control with the given \a parent.
*/
QVideoDeviceSelectorControl::QVideoDeviceSelectorControl(QObject *parent)
    :QObject(parent)
{
    m_deviceManager = QMediaPlatformIntegration::instance()->deviceManager();
}

/*!
    \fn QVideoDeviceSelectorControl::deviceCount() const

    Returns the number of available video devices;
*/
inline int QVideoDeviceSelectorControl::deviceCount() const
{
    return m_deviceManager->videoInputs().size();
}

/*!
    \fn QVideoDeviceSelectorControl::deviceName(int index) const

    Returns the name of the video device at \a index.
*/
QString QVideoDeviceSelectorControl::deviceName(int index) const
{
    auto cameras = m_deviceManager->videoInputs();
    return QString::fromLatin1(cameras.value(index).id());
}

/*!

    \fn QVideoDeviceSelectorControl::deviceDescription(int index) const

    Returns a description of the video device at \a index.
*/
QString QVideoDeviceSelectorControl::deviceDescription(int index) const
{
    auto cameras = m_deviceManager->videoInputs();
    return cameras.value(index).description();
}

/*!

    \fn QVideoDeviceSelectorControl::cameraPosition(int index) const

    Returns the physical position of the camera at \a index.
*/
QCamera::Position QVideoDeviceSelectorControl::cameraPosition(int index) const
{
    auto cameras = m_deviceManager->videoInputs();
    return cameras.value(index).position();
}

/*!

    \fn QVideoDeviceSelectorControl::cameraOrientation(const QString &deviceName) const

    Returns the physical orientation of the sensor for the camera at \a index.

    The value is the orientation angle (clockwise, in steps of 90 degrees) of the camera sensor
    in relation to the display in its natural orientation.
*/
int QVideoDeviceSelectorControl::cameraOrientation(int index) const
{
    auto cameras = m_deviceManager->videoInputs();
    return cameras.value(index).orientation();
}

/*!
    \fn QVideoDeviceSelectorControl::defaultDevice() const

    Returns the index of the default video device.
*/
int QVideoDeviceSelectorControl::defaultDevice() const
{
    return 0;
}

/*!
    \fn QVideoDeviceSelectorControl::selectedDevice() const

    Returns the index of the selected video device.
*/

/*!
    \fn QVideoDeviceSelectorControl::setSelectedDevice(int index)

    Sets the selected video device \a index.
*/

/*!
    \fn QVideoDeviceSelectorControl::devicesChanged()

    Signals that the list of available video devices has changed.
*/

/*!
    \fn QVideoDeviceSelectorControl::selectedDeviceChanged(int index)

    Signals that the selected video device \a index has changed.
*/

/*!
    \fn QVideoDeviceSelectorControl::selectedDeviceChanged(const QString &name)

    Signals that the selected video device \a name has changed.
*/

QT_END_NAMESPACE

#include "moc_qvideodeviceselectorcontrol.cpp"
