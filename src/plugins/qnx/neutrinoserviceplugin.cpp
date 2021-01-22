/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#include "neutrinoserviceplugin.h"

#include "mmrenderermediaplayerservice.h"

QT_BEGIN_NAMESPACE

NeutrinoServicePlugin::NeutrinoServicePlugin()
{
}

QMediaService *NeutrinoServicePlugin::create(const QString &key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER))
        return new MmRendererMediaPlayerService();

    return 0;
}

void NeutrinoServicePlugin::release(QMediaService *service)
{
    delete service;
}

QMediaServiceProviderHint::Features NeutrinoServicePlugin::supportedFeatures(const QByteArray &service) const
{
    Q_UNUSED(service)
    return QMediaServiceProviderHint::Features();
}

QT_END_NAMESPACE
