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

#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtCore/QFile>

#include "wmfserviceplugin.h"
#include "mfplayerservice.h"
#include "mfdecoderservice.h"

#include <mfapi.h>

namespace
{
static int g_refCount = 0;
void addRefCount()
{
    g_refCount++;
    if (g_refCount == 1) {
        CoInitialize(NULL);
        MFStartup(MF_VERSION);
    }
}

void releaseRefCount()
{
    g_refCount--;
    if (g_refCount == 0) {
        MFShutdown();
        CoUninitialize();
    }
}

}

QMediaService* WMFServicePlugin::create(QString const& key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER)) {
        addRefCount();
        return new MFPlayerService;
    }

    if (key == QLatin1String(Q_MEDIASERVICE_AUDIODECODER)) {
        addRefCount();
        return new MFAudioDecoderService;
    }
    //qDebug() << "unsupported key:" << key;
    return 0;
}

void WMFServicePlugin::release(QMediaService *service)
{
    delete service;
    releaseRefCount();
}

QMediaServiceProviderHint::Features WMFServicePlugin::supportedFeatures(
        const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_MEDIAPLAYER)
        return QMediaServiceProviderHint::StreamPlayback;
    else
        return QMediaServiceProviderHint::Features();
}

QByteArray WMFServicePlugin::defaultDevice(const QByteArray &) const
{
    return QByteArray();
}

QList<QByteArray> WMFServicePlugin::devices(const QByteArray &) const
{
    return QList<QByteArray>();
}

QString WMFServicePlugin::deviceDescription(const QByteArray &, const QByteArray &)
{
    return QString();
}

