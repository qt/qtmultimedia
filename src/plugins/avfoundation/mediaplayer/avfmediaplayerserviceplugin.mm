/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfmediaplayerserviceplugin.h"
#include <QtCore/QDebug>

#include "avfmediaplayerservice.h"

#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

AVFMediaPlayerServicePlugin::AVFMediaPlayerServicePlugin()
{
    buildSupportedTypes();
}

QMediaService *AVFMediaPlayerServicePlugin::create(const QString &key)
{
#ifdef QT_DEBUG_AVF
    qDebug() << "AVFMediaPlayerServicePlugin::create" << key;
#endif
    if (key == QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER))
        return new AVFMediaPlayerService;

    qWarning() << "unsupported key: " << key;
    return nullptr;
}

void AVFMediaPlayerServicePlugin::release(QMediaService *service)
{
    delete service;
}

QMediaServiceProviderHint::Features AVFMediaPlayerServicePlugin::supportedFeatures(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_MEDIAPLAYER)
        return QMediaServiceProviderHint::VideoSurface;
    else
        return QMediaServiceProviderHint::Features();
}

QMultimedia::SupportEstimate AVFMediaPlayerServicePlugin::hasSupport(const QString &mimeType, const QStringList &codecs) const
{
    Q_UNUSED(codecs);

    if (m_supportedMimeTypes.contains(mimeType))
        return QMultimedia::ProbablySupported;

    return QMultimedia::MaybeSupported;
}

QStringList AVFMediaPlayerServicePlugin::supportedMimeTypes() const
{
    return m_supportedMimeTypes;
}

void AVFMediaPlayerServicePlugin::buildSupportedTypes()
{
    //Populate m_supportedMimeTypes with mimetypes AVAsset supports
    NSArray *mimeTypes = [AVURLAsset audiovisualMIMETypes];
    for (NSString *mimeType in mimeTypes)
    {
        m_supportedMimeTypes.append(QString::fromUtf8([mimeType UTF8String]));
    }
#ifdef QT_DEBUG_AVF
    qDebug() << "AVFMediaPlayerServicePlugin::buildSupportedTypes";
    qDebug() << "Supported Types: " << m_supportedMimeTypes;
#endif

}
