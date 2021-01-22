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

#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>

#include "avfcameraserviceplugin.h"
#include "avfcameraservice.h"
#include "avfcamerasession.h"

#include <qmediaserviceproviderplugin.h>

QT_BEGIN_NAMESPACE

AVFServicePlugin::AVFServicePlugin()
{
}

QMediaService* AVFServicePlugin::create(QString const& key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA))
        return new AVFCameraService;
    else
        qWarning() << "unsupported key:" << key;

    return nullptr;
}

void AVFServicePlugin::release(QMediaService *service)
{
    delete service;
}

QByteArray AVFServicePlugin::defaultDevice(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        int i = AVFCameraSession::defaultCameraIndex();
        if (i != -1)
            return AVFCameraSession::availableCameraDevices().at(i).deviceId;
    }

    return QByteArray();
}

QList<QByteArray> AVFServicePlugin::devices(const QByteArray &service) const
{
    QList<QByteArray> devs;

    if (service == Q_MEDIASERVICE_CAMERA) {
        const QList<AVFCameraInfo> &cameras = AVFCameraSession::availableCameraDevices();
        devs.reserve(cameras.size());
        for (const AVFCameraInfo &info : cameras)
            devs.append(info.deviceId);
    }

    return devs;
}

QString AVFServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA)
        return AVFCameraSession::cameraDeviceInfo(device).description;

    return QString();
}

QCamera::Position AVFServicePlugin::cameraPosition(const QByteArray &device) const
{
    return AVFCameraSession::cameraDeviceInfo(device).position;
}

int AVFServicePlugin::cameraOrientation(const QByteArray &device) const
{
    return AVFCameraSession::cameraDeviceInfo(device).orientation;
}

QT_END_NAMESPACE
