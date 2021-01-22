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


#ifndef CAMERABINSERVICEPLUGIN_H
#define CAMERABINSERVICEPLUGIN_H

#include <qmediaserviceproviderplugin.h>
#include <private/qgstreamervideoinputdevicecontrol_p.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class CameraBinServicePlugin
    : public QMediaServiceProviderPlugin
    , public QMediaServiceSupportedDevicesInterface
    , public QMediaServiceDefaultDeviceInterface
    , public QMediaServiceFeaturesInterface
    , public QMediaServiceCameraInfoInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaServiceSupportedDevicesInterface)
    Q_INTERFACES(QMediaServiceDefaultDeviceInterface)
    Q_INTERFACES(QMediaServiceFeaturesInterface)
    Q_INTERFACES(QMediaServiceCameraInfoInterface)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0" FILE "camerabin.json")
public:
    CameraBinServicePlugin();
    ~CameraBinServicePlugin();

    QMediaService* create(const QString &key) override;
    void release(QMediaService *service) override;

    QMediaServiceProviderHint::Features supportedFeatures(const QByteArray &service) const override;

    QByteArray defaultDevice(const QByteArray &service) const override;
    QList<QByteArray> devices(const QByteArray &service) const override;
    QString deviceDescription(const QByteArray &service, const QByteArray &device) override;
    QVariant deviceProperty(const QByteArray &service, const QByteArray &device, const QByteArray &property);

    QCamera::Position cameraPosition(const QByteArray &device) const override;
    int cameraOrientation(const QByteArray &device) const override;

private:
    GstElementFactory *sourceFactory() const;

    mutable GstElementFactory *m_sourceFactory;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURESERVICEPLUGIN_H
