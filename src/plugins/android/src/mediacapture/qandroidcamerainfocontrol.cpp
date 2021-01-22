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

#include "qandroidcamerainfocontrol.h"

#include "qandroidcamerasession.h"

QT_BEGIN_NAMESPACE

QCamera::Position QAndroidCameraInfoControl::position(const QString &deviceName)
{
    const QList<AndroidCameraInfo> &cameras = QAndroidCameraSession::availableCameras();
    for (int i = 0; i < cameras.count(); ++i) {
        const AndroidCameraInfo &info = cameras.at(i);
        if (QString::fromLatin1(info.name) == deviceName)
            return info.position;
    }

    return QCamera::UnspecifiedPosition;
}

int QAndroidCameraInfoControl::orientation(const QString &deviceName)
{
    const QList<AndroidCameraInfo> &cameras = QAndroidCameraSession::availableCameras();
    for (int i = 0; i < cameras.count(); ++i) {
        const AndroidCameraInfo &info = cameras.at(i);
        if (QString::fromLatin1(info.name) == deviceName)
            return info.orientation;
    }

    return 0;
}

QCamera::Position QAndroidCameraInfoControl::cameraPosition(const QString &deviceName) const
{
    return position(deviceName);
}

int QAndroidCameraInfoControl::cameraOrientation(const QString &deviceName) const
{
    return orientation(deviceName);
}

QT_END_NAMESPACE
