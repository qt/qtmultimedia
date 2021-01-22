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

#ifndef QGSTREAMERAUDIODECODERSERVICEPLUGIN_H
#define QGSTREAMERAUDIODECODERSERVICEPLUGIN_H

#include <qmediaserviceproviderplugin.h>
#include <QtCore/qset.h>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE

class QGstreamerAudioDecoderServicePlugin
        : public QMediaServiceProviderPlugin
        , public QMediaServiceSupportedFormatsInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaServiceSupportedFormatsInterface)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0" FILE "audiodecoder.json")

public:
    QMediaService* create(const QString &key) override;
    void release(QMediaService *service) override;

    QMultimedia::SupportEstimate hasSupport(const QString &mimeType, const QStringList &codecs) const override;
    QStringList supportedMimeTypes() const override;

private:
    void updateSupportedMimeTypes() const;

    mutable QSet<QString> m_supportedMimeTypeSet;
};

QT_END_NAMESPACE

#endif // QGSTREAMERAUDIODECODERSERVICEPLUGIN_H
