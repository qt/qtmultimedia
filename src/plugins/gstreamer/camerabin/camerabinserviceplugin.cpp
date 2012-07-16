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
#include <QtCore/QDir>
#include <QtCore/QDebug>

#include "camerabinserviceplugin.h"


#include "camerabinservice.h"
#include <private/qgstutils_p.h>

#include <linux/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <gst/gst.h>

QT_BEGIN_NAMESPACE

QMediaService* CameraBinServicePlugin::create(const QString &key)
{
    QGstUtils::initializeGst();

    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA))
        return new CameraBinService(key);

    qWarning() << "Gstreamer camerabin service plugin: unsupported key:" << key;
    return 0;
}

void CameraBinServicePlugin::release(QMediaService *service)
{
    delete service;
}

QMediaServiceProviderHint::Features CameraBinServicePlugin::supportedFeatures(
        const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA)
        return QMediaServiceProviderHint::VideoSurface;

    return QMediaServiceProviderHint::Features();
}

QList<QByteArray> CameraBinServicePlugin::devices(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (m_cameraDevices.isEmpty())
            updateDevices();

        return m_cameraDevices;
    }

    return QList<QByteArray>();
}

QString CameraBinServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (m_cameraDevices.isEmpty())
            updateDevices();

        for (int i=0; i<m_cameraDevices.count(); i++)
            if (m_cameraDevices[i] == device)
                return m_cameraDescriptions[i];
    }

    return QString();
}

QVariant CameraBinServicePlugin::deviceProperty(const QByteArray &service, const QByteArray &device, const QByteArray &property)
{
    Q_UNUSED(service);
    Q_UNUSED(device);
    Q_UNUSED(property);
    return QVariant();
}

void CameraBinServicePlugin::updateDevices() const
{
    m_cameraDevices.clear();
    m_cameraDescriptions.clear();

    QDir devDir("/dev");
    devDir.setFilter(QDir::System);

    QFileInfoList entries = devDir.entryInfoList(QStringList() << "video*");

    foreach (const QFileInfo &entryInfo, entries) {
        int fd = ::open(entryInfo.filePath().toLatin1().constData(), O_RDWR );
        if (fd == -1)
            continue;

        bool isCamera = false;

        v4l2_input input;
        memset(&input, 0, sizeof(input));
        for (; ::ioctl(fd, VIDIOC_ENUMINPUT, &input) >= 0; ++input.index) {
            if (input.type == V4L2_INPUT_TYPE_CAMERA || input.type == 0) {
                isCamera = ::ioctl(fd, VIDIOC_S_INPUT, input.index) != 0;
                break;
            }
        }

        if (isCamera) {
            // find out its driver "name"
            QString name;
            struct v4l2_capability vcap;
            memset(&vcap, 0, sizeof(struct v4l2_capability));

            if (ioctl(fd, VIDIOC_QUERYCAP, &vcap) != 0)
                name = entryInfo.fileName();
            else
                name = QString((const char*)vcap.card);
            //qDebug() << "found camera: " << name;

            m_cameraDevices.append(entryInfo.filePath().toLocal8Bit());
            m_cameraDescriptions.append(name);
        }
        ::close(fd);
    }
}

QT_END_NAMESPACE
