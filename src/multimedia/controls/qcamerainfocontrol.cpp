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

#include "qcamerainfocontrol.h"

QT_BEGIN_NAMESPACE

/*!
    \class QCameraInfoControl
    \obsolete
    \since 5.3

    \brief The QCameraInfoControl class provides a camera info media control.
    \inmodule QtMultimedia

    \ingroup multimedia_control

    The QCameraInfoControl class provides information about the camera devices
    available on the system.

    The interface name of QCameraInfoControl is \c org.qt-project.qt.camerainfocontrol/5.3 as
    defined in QCameraInfoControl_iid.
*/

/*!
    \macro QCameraInfoControl_iid

    \c org.qt-project.qt.camerainfocontrol/5.3

    Defines the interface name of the QCameraInfoControl class.

    \relates QVideoDeviceSelectorControl
*/

/*!
    Constructs a camera info control with the given \a parent.
*/
QCameraInfoControl::QCameraInfoControl(QObject *parent)
    : QMediaControl(parent)
{
}

/*!
    Destroys a camera info control.
*/
QCameraInfoControl::~QCameraInfoControl()
{
}

/*!
    \fn QCameraInfoControl::cameraPosition(const QString &deviceName) const

    Returns the physical position of the camera named \a deviceName on the hardware system.
*/

/*!
    \fn QCameraInfoControl::cameraOrientation(const QString &deviceName) const

    Returns the physical orientation of the sensor for the camera named \a deviceName.

    The value is the orientation angle (clockwise, in steps of 90 degrees) of the camera sensor
    in relation to the display in its natural orientation.
*/

QT_END_NAMESPACE

#include "moc_qcamerainfocontrol.cpp"
