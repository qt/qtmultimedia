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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <dshow.h>

#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtCore/QFile>

#include "directshowglobal.h"
#include "dsserviceplugin.h"

#include "dsvideodevicecontrol.h"
#include <dshow.h>
#include "dscameraservice.h"

#include "directshowplayerservice.h"

#include <qmediaserviceproviderplugin.h>
#include "directshowutils.h"

extern const CLSID CLSID_VideoInputDeviceCategory;


#ifndef _STRSAFE_H_INCLUDED_
#include <tchar.h>
#endif
#include <dshow.h>
#include <objbase.h>
#include <initguid.h>
#ifdef Q_CC_MSVC
#  pragma comment(lib, "strmiids.lib")
#  pragma comment(lib, "ole32.lib")
#endif // Q_CC_MSVC
#include <windows.h>
#include <ocidl.h>

QT_BEGIN_NAMESPACE
Q_LOGGING_CATEGORY(qtDirectShowPlugin, "qt.multimedia.plugins.directshow")

QMediaService* DSServicePlugin::create(QString const& key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA)) {
        DirectShowUtils::CoInitializeIfNeeded();
        return new DSCameraService;
    }

    if (key == QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER)) {
        DirectShowUtils::CoInitializeIfNeeded();
        return new DirectShowPlayerService;
    }

    return nullptr;
}

void DSServicePlugin::release(QMediaService *service)
{
    delete service;
    DirectShowUtils::CoUninitializeIfNeeded();
}

QMediaServiceProviderHint::Features DSServicePlugin::supportedFeatures(
        const QByteArray &service) const
{
    return service == Q_MEDIASERVICE_MEDIAPLAYER
        ? (QMediaServiceProviderHint::StreamPlayback | QMediaServiceProviderHint::VideoSurface)
        : QMediaServiceProviderHint::Features();
}

QByteArray DSServicePlugin::defaultDevice(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        DirectShowUtils::CoInitializeIfNeeded();
        const QList<DSVideoDeviceInfo> &devs = DSVideoDeviceControl::availableDevices();
        DirectShowUtils::CoUninitializeIfNeeded();
        if (!devs.isEmpty())
            return devs.first().first;
    }
    return QByteArray();
}

QList<QByteArray> DSServicePlugin::devices(const QByteArray &service) const
{
    QList<QByteArray> result;

    if (service == Q_MEDIASERVICE_CAMERA) {
        DirectShowUtils::CoInitializeIfNeeded();
        const QList<DSVideoDeviceInfo> &devs = DSVideoDeviceControl::availableDevices();
        DirectShowUtils::CoUninitializeIfNeeded();
        for (const DSVideoDeviceInfo &info : devs)
            result.append(info.first);
    }

    return result;
}

QString DSServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        DirectShowUtils::CoInitializeIfNeeded();
        const QList<DSVideoDeviceInfo> &devs = DSVideoDeviceControl::availableDevices();
        DirectShowUtils::CoUninitializeIfNeeded();
        for (const DSVideoDeviceInfo &info : devs) {
            if (info.first == device)
                return info.second;
        }
    }
    return QString();
}

QT_END_NAMESPACE
