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
#ifndef NEUTRINOSERVICEPLUGIN_H
#define NEUTRINOSERVICEPLUGIN_H

#include <qmediaserviceproviderplugin.h>

QT_BEGIN_NAMESPACE

class NeutrinoServicePlugin
    : public QMediaServiceProviderPlugin,
      public QMediaServiceFeaturesInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaServiceFeaturesInterface)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0" FILE "neutrino_mediaservice.json")
public:
    NeutrinoServicePlugin();

    QMediaService *create(const QString &key) override;
    void release(QMediaService *service) override;
    QMediaServiceProviderHint::Features supportedFeatures(const QByteArray &service) const override;
};

QT_END_NAMESPACE

#endif
