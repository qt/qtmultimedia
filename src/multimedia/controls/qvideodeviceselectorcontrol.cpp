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

#include "qvideodeviceselectorcontrol.h"

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
    :QMediaControl(parent)
{
}

/*!
    Destroys a video device selector control.
*/
QVideoDeviceSelectorControl::~QVideoDeviceSelectorControl()
{
}

/*!
    \fn QVideoDeviceSelectorControl::deviceCount() const

    Returns the number of available video devices;
*/

/*!
    \fn QVideoDeviceSelectorControl::deviceName(int index) const

    Returns the name of the video device at \a index.
*/

/*!
    \fn QVideoDeviceSelectorControl::deviceDescription(int index) const

    Returns a description of the video device at \a index.
*/

/*!
    \fn QVideoDeviceSelectorControl::defaultDevice() const

    Returns the index of the default video device.
*/

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
