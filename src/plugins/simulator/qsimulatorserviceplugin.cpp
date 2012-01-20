/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#include "qsimulatorserviceplugin.h"
#include <mobilityconnection_p.h>

#include "simulatorcameraservice.h"

#include <qmediaserviceprovider.h>

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

QTM_USE_NAMESPACE

Simulator::MultimediaConnection *QSimulatorServicePlugin::mMultimediaConnection = 0;

QSimulatorServicePlugin::QSimulatorServicePlugin()
{
    ensureSimulatorConnection();
}

QStringList QSimulatorServicePlugin::keys() const
{
    QStringList retList;
    retList << QLatin1String(Q_MEDIASERVICE_CAMERA);
    return retList;
}

QMediaService* QSimulatorServicePlugin::create(const QString &key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA))
        return new SimulatorCameraService(key, mMultimediaConnection);

    qWarning() << "Simulator service plugin: unsupported key:" << key;
    return 0;
}

void QSimulatorServicePlugin::release(QMediaService *service)
{
    delete service;
}

QList<QByteArray> QSimulatorServicePlugin::devices(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        QCameraData cams = get_qtCameraData();
        QList<QByteArray> returnList;
        foreach(const QString &key, cams.cameras.keys())
            returnList.append(key.toAscii());
        return returnList;
    }

    return QList<QByteArray>();
}

QString QSimulatorServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        QCameraData cams = get_qtCameraData();
        return cams.cameras.value(device).description;
    }

    return QString();
}

void QSimulatorServicePlugin::ensureSimulatorConnection()
{
    using namespace QtMobility::Simulator;

    static bool connected = false;
    if (connected)
        return;

    connected = true;
    MobilityConnection *connection = MobilityConnection::instance();
    mMultimediaConnection = new MultimediaConnection(connection);
    mMultimediaConnection->getInitialData();
}

Q_EXPORT_PLUGIN2(qtmedia_simulatorengine, QSimulatorServicePlugin);
