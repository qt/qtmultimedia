/****************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "bbserviceplugin.h"

#ifndef Q_OS_BLACKBERRY_TABLET
#include "bbcameraservice.h"
#include "bbvideodeviceselectorcontrol.h"
#endif
#include "bbmediaplayerservice.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

BbServicePlugin::BbServicePlugin()
{
}

QMediaService *BbServicePlugin::create(const QString &key)
{
#ifndef Q_OS_BLACKBERRY_TABLET
    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA))
        return new BbCameraService();
#endif

    if (key == QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER))
        return new BbMediaPlayerService();

    return 0;
}

void BbServicePlugin::release(QMediaService *service)
{
    delete service;
}

QMediaServiceProviderHint::Features BbServicePlugin::supportedFeatures(const QByteArray &service) const
{
    Q_UNUSED(service)
    return QMediaServiceProviderHint::Features();
}

QList<QByteArray> BbServicePlugin::devices(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (m_cameraDevices.isEmpty())
            updateDevices();

        return m_cameraDevices;
    }

    return QList<QByteArray>();
}

QString BbServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (m_cameraDevices.isEmpty())
            updateDevices();

        for (int i = 0; i < m_cameraDevices.count(); i++)
            if (m_cameraDevices[i] == device)
                return m_cameraDescriptions[i];
    }

    return QString();
}

void BbServicePlugin::updateDevices() const
{
#ifndef Q_OS_BLACKBERRY_TABLET
    BbVideoDeviceSelectorControl::enumerateDevices(&m_cameraDevices, &m_cameraDescriptions);
#endif

    if (m_cameraDevices.isEmpty()) {
        qWarning() << "No camera devices found";
    }
}

QT_END_NAMESPACE
