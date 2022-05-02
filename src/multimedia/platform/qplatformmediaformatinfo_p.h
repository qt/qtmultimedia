/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QPLATFORMMEDIAFORMATINFO_H
#define QPLATFORMMEDIAFORMATINFO_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtmultimediaglobal_p.h>
#include <qimagecapture.h>
#include <qmediaformat.h>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QPlatformMediaFormatInfo
{
public:
    QPlatformMediaFormatInfo();
    virtual ~QPlatformMediaFormatInfo();

    QList<QMediaFormat::FileFormat> supportedFileFormats(const QMediaFormat &constraints, QMediaFormat::ConversionMode m) const;
    QList<QMediaFormat::AudioCodec> supportedAudioCodecs(const QMediaFormat &constraints, QMediaFormat::ConversionMode m) const;
    QList<QMediaFormat::VideoCodec> supportedVideoCodecs(const QMediaFormat &constraints, QMediaFormat::ConversionMode m) const;

    bool isSupported(const QMediaFormat &format, QMediaFormat::ConversionMode m) const;

    struct CodecMap {
        QMediaFormat::FileFormat format;
        QList<QMediaFormat::AudioCodec> audio;
        QList<QMediaFormat::VideoCodec> video;
    };
    QList<CodecMap> encoders;
    QList<CodecMap> decoders;

    QList<QImageCapture::FileFormat> imageFormats;
};

QT_END_NAMESPACE


#endif // QPLATFORMMEDIAFORMATINFO_H
