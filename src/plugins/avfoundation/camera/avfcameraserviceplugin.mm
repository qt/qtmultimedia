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

#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>

#include "avfcameraserviceplugin.h"
#include "avfcameraservice.h"

#include <qmediaserviceproviderplugin.h>

#import <AVFoundation/AVFoundation.h>


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

QList<QByteArray> AVFServicePlugin::devices(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (m_cameraDevices.isEmpty())
            updateDevices();

        return m_cameraDevices;
    }

    return QList<QByteArray>();
}

QString AVFServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (m_cameraDevices.isEmpty())
            updateDevices();

        return m_cameraDescriptions.value(device);
    }

    return QString();
}

void AVFServicePlugin::updateDevices() const
{
    m_cameraDevices.clear();
    m_cameraDescriptions.clear();

    NSArray *videoDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in videoDevices) {
        QByteArray deviceId([[device uniqueID] UTF8String]);
        m_cameraDevices << deviceId;
        m_cameraDescriptions.insert(deviceId, QString::fromUtf8([[device localizedName] UTF8String]));
    }
}

QT_END_NAMESPACE
