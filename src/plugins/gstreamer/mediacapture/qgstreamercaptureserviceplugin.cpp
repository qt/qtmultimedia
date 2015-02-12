/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#include "qgstreamercaptureserviceplugin.h"

//#define QT_SUPPORTEDMIMETYPES_DEBUG

#include "qgstreamercaptureservice.h"
#include <private/qgstutils_p.h>

QMediaService* QGstreamerCaptureServicePlugin::create(const QString &key)
{
    QGstUtils::initializeGst();

    if (key == QLatin1String(Q_MEDIASERVICE_AUDIOSOURCE))
        return new QGstreamerCaptureService(key);

#if defined(USE_GSTREAMER_CAMERA)
    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA))
        return new QGstreamerCaptureService(key);
#endif

    qWarning() << "Gstreamer capture service plugin: unsupported key:" << key;
    return 0;
}

void QGstreamerCaptureServicePlugin::release(QMediaService *service)
{
    delete service;
}

#if defined(USE_GSTREAMER_CAMERA)
QMediaServiceProviderHint::Features QGstreamerCaptureServicePlugin::supportedFeatures(
        const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA)
        return QMediaServiceProviderHint::VideoSurface;

    return QMediaServiceProviderHint::Features();
}

QByteArray QGstreamerCaptureServicePlugin::defaultDevice(const QByteArray &service) const
{
    return service == Q_MEDIASERVICE_CAMERA
            ? QGstUtils::enumerateCameras().value(0).name.toUtf8()
            : QByteArray();
}

QList<QByteArray> QGstreamerCaptureServicePlugin::devices(const QByteArray &service) const
{
    return service == Q_MEDIASERVICE_CAMERA ? QGstUtils::cameraDevices() : QList<QByteArray>();
}

QString QGstreamerCaptureServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    return service == Q_MEDIASERVICE_CAMERA ? QGstUtils::cameraDescription(deviceName) : QString();
}

QVariant QGstreamerCaptureServicePlugin::deviceProperty(const QByteArray &service, const QByteArray &device, const QByteArray &property)
{
    Q_UNUSED(service);
    Q_UNUSED(device);
    Q_UNUSED(property);
    return QVariant();
}

#endif

QMultimedia::SupportEstimate QGstreamerCaptureServicePlugin::hasSupport(const QString &mimeType,
                                                                     const QStringList& codecs) const
{
    if (m_supportedMimeTypeSet.isEmpty())
        updateSupportedMimeTypes();

    return QGstUtils::hasSupport(mimeType, codecs, m_supportedMimeTypeSet);
}


static bool isEncoderOrMuxer(GstElementFactory *factory)
{
    return gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_MUXER)
                || gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_ENCODER);
}

void QGstreamerCaptureServicePlugin::updateSupportedMimeTypes() const
{
    m_supportedMimeTypeSet = QGstUtils::supportedMimeTypes(isEncoderOrMuxer);
}

QStringList QGstreamerCaptureServicePlugin::supportedMimeTypes() const
{
    return QStringList();
}

