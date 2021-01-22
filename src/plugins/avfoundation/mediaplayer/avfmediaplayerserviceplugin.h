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


#ifndef AVFMEDIAPLAYERSERVICEPLUGIN_H
#define AVFMEDIAPLAYERSERVICEPLUGIN_H

#include <QtCore/qglobal.h>
#include <QtMultimedia/qmediaserviceproviderplugin.h>

QT_BEGIN_NAMESPACE

class AVFMediaPlayerServicePlugin
    : public QMediaServiceProviderPlugin
    , public QMediaServiceSupportedFormatsInterface
    , public QMediaServiceFeaturesInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaServiceSupportedFormatsInterface)
    Q_INTERFACES(QMediaServiceFeaturesInterface)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0" FILE "avfmediaplayer.json")

public:
    explicit AVFMediaPlayerServicePlugin();

    QMediaService* create(QString const& key) override;
    void release(QMediaService *service) override;

    QMediaServiceProviderHint::Features supportedFeatures(const QByteArray &service) const override;
    QMultimedia::SupportEstimate hasSupport(const QString &mimeType, const QStringList& codecs) const override;
    QStringList supportedMimeTypes() const override;

private:
    void buildSupportedTypes();

    QStringList m_supportedMimeTypes;
};

QT_END_NAMESPACE

#endif // AVFMEDIAPLAYERSERVICEPLUGIN_H
