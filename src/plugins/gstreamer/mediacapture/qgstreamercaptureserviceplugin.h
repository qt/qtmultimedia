/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
    QMediaService* create(QString const& key);
    void release(QMediaService *service);

#if defined(USE_GSTREAMER_CAMERA)
    QMediaServiceProviderHint::Features supportedFeatures(const QByteArray &service) const;

    QByteArray defaultDevice(const QByteArray &service) const;
    QList<QByteArray> devices(const QByteArray &service) const;
    QString deviceDescription(const QByteArray &service, const QByteArray &device);
    QVariant deviceProperty(const QByteArray &service, const QByteArray &device, const QByteArray &property);
#endif

    QMultimedia::SupportEstimate hasSupport(const QString &mimeType, const QStringList& codecs) const;
    QStringList supportedMimeTypes() const;

private:
#if defined(USE_GSTREAMER_CAMERA)
    void updateDevices() const;

    mutable QByteArray m_defaultCameraDevice;
    mutable QList<QByteArray> m_cameraDevices;
    mutable QStringList m_cameraDescriptions;
#endif
    void updateSupportedMimeTypes() const;

    mutable QSet<QString> m_supportedMimeTypeSet; //for fast access
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURESERVICEPLUGIN_H
