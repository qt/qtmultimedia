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

#ifndef WMFSERVICEPLUGIN_H
#define WMFSERVICEPLUGIN_H

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include "qmediaserviceproviderplugin.h"

QT_USE_NAMESPACE

class WMFServicePlugin
    : public QMediaServiceProviderPlugin
    , public QMediaServiceSupportedDevicesInterface
    , public QMediaServiceDefaultDeviceInterface
    , public QMediaServiceFeaturesInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaServiceSupportedDevicesInterface)
    Q_INTERFACES(QMediaServiceDefaultDeviceInterface)
    Q_INTERFACES(QMediaServiceFeaturesInterface)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0" FILE "wmf.json")

public:
    QMediaService* create(QString const& key);
    void release(QMediaService *service);

    QMediaServiceProviderHint::Features supportedFeatures(const QByteArray &service) const;

    QByteArray defaultDevice(const QByteArray &service) const;
    QList<QByteArray> devices(const QByteArray &service) const;
    QString deviceDescription(const QByteArray &service, const QByteArray &device);
};

#endif // WMFSERVICEPLUGIN_H
