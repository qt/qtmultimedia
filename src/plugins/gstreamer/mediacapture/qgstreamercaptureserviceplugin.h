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


#ifndef QGSTREAMERCAPTURESERVICEPLUGIN_H
#define QGSTREAMERCAPTURESERVICEPLUGIN_H

#include <qmediaserviceproviderplugin.h>
#include <QtCore/qset.h>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE

class QGstreamerCaptureServicePlugin
    : public QMediaServiceProviderPlugin
#if defined(USE_GSTREAMER_CAMERA)
    , public QMediaServiceSupportedDevicesInterface
    , public QMediaServiceDefaultDeviceInterface
    , public QMediaServiceFeaturesInterface
#endif
    , public QMediaServiceSupportedFormatsInterface
{
    Q_OBJECT
#if defined(USE_GSTREAMER_CAMERA)
    Q_INTERFACES(QMediaServiceSupportedDevicesInterface)
    Q_INTERFACES(QMediaServiceDefaultDeviceInterface)
    Q_INTERFACES(QMediaServiceFeaturesInterface)
#endif
    Q_INTERFACES(QMediaServiceSupportedFormatsInterface)
#if defined(USE_GSTREAMER_CAMERA)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0" FILE "mediacapturecamera.json")
#else
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0" FILE "mediacapture.json")
#endif
public:
    QMediaService* create(const QString &key) override;
    void release(QMediaService *service) override;

#if defined(USE_GSTREAMER_CAMERA)
    QMediaServiceProviderHint::Features supportedFeatures(const QByteArray &service) const override;

    QByteArray defaultDevice(const QByteArray &service) const override;
    QList<QByteArray> devices(const QByteArray &service) const override;
    QString deviceDescription(const QByteArray &service, const QByteArray &device) override;
    QVariant deviceProperty(const QByteArray &service, const QByteArray &device, const QByteArray &property);
#endif

    QMultimedia::SupportEstimate hasSupport(const QString &mimeType, const QStringList &codecs) const override;
    QStringList supportedMimeTypes() const override;

private:
    void updateSupportedMimeTypes() const;

    mutable QSet<QString> m_supportedMimeTypeSet; //for fast access
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURESERVICEPLUGIN_H
