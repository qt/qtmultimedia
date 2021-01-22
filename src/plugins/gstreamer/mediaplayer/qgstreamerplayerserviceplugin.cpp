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
#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#include "qgstreamerplayerserviceplugin.h"

//#define QT_SUPPORTEDMIMETYPES_DEBUG

#include "qgstreamerplayerservice.h"
#include <private/qgstutils_p.h>

QMediaService* QGstreamerPlayerServicePlugin::create(const QString &key)
{
    QGstUtils::initializeGst();

    if (key == QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER))
        return new QGstreamerPlayerService;

    qWarning() << "Gstreamer service plugin: unsupported key:" << key;
    return 0;
}

void QGstreamerPlayerServicePlugin::release(QMediaService *service)
{
    delete service;
}

QMediaServiceProviderHint::Features QGstreamerPlayerServicePlugin::supportedFeatures(
        const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_MEDIAPLAYER)
        return
#if QT_CONFIG(gstreamer_app)
                QMediaServiceProviderHint::StreamPlayback |
#endif
                QMediaServiceProviderHint::VideoSurface;
    else
        return QMediaServiceProviderHint::Features();
}

QMultimedia::SupportEstimate QGstreamerPlayerServicePlugin::hasSupport(const QString &mimeType,
                                                                     const QStringList &codecs) const
{
    if (m_supportedMimeTypeSet.isEmpty())
        updateSupportedMimeTypes();

    return QGstUtils::hasSupport(mimeType, codecs, m_supportedMimeTypeSet);
}

static bool isDecoderOrDemuxer(GstElementFactory *factory)
{
#if GST_CHECK_VERSION(0, 10 ,31)
    return gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DEMUXER)
                || gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER);
#else
    return (factory
            && (qstrcmp(factory->details.klass, "Codec/Decoder/Audio") == 0
                || qstrcmp(factory->details.klass, "Codec/Decoder/Video") == 0
                || qstrcmp(factory->details.klass, "Codec/Demux") == 0 ));
#endif
}

void QGstreamerPlayerServicePlugin::updateSupportedMimeTypes() const
{
     m_supportedMimeTypeSet = QGstUtils::supportedMimeTypes(isDecoderOrDemuxer);
}

QStringList QGstreamerPlayerServicePlugin::supportedMimeTypes() const
{
    return QStringList();
}

