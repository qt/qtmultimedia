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

    return 0;
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
        Q_FOREACH (const AVFCameraInfo &info, cameras)
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
