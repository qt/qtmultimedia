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

#include "bbcamerainfocontrol.h"

#include "bbcamerasession.h"

QT_BEGIN_NAMESPACE

BbCameraInfoControl::BbCameraInfoControl(QObject *parent)
    : QCameraInfoControl(parent)
{
}

QCamera::Position BbCameraInfoControl::position(const QString &deviceName)
{
    if (deviceName == QString::fromUtf8(BbCameraSession::cameraIdentifierFront()))
        return QCamera::FrontFace;
    else if (deviceName == QString::fromUtf8(BbCameraSession::cameraIdentifierRear()))
        return QCamera::BackFace;
    else
        return QCamera::UnspecifiedPosition;
}

int BbCameraInfoControl::orientation(const QString &deviceName)
{
    // The camera sensor orientation could be retrieved with camera_get_native_orientation()
    // but since the sensor angular offset is compensated with camera_set_videovf_property() and
    // camera_set_photovf_property() we should always return 0 here.
    Q_UNUSED(deviceName);
    return 0;
}

QCamera::Position BbCameraInfoControl::cameraPosition(const QString &deviceName) const
{
    return position(deviceName);
}

int BbCameraInfoControl::cameraOrientation(const QString &deviceName) const
{
    return orientation(deviceName);
}

QT_END_NAMESPACE

