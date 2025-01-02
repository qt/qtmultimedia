// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERFORMATINFO_H
#define QGSTREAMERFORMATINFO_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformmediaformatinfo_p.h>
#include <qlist.h>
#include <common/qgst_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerFormatInfo : public QPlatformMediaFormatInfo
{
public:
    QGstreamerFormatInfo();
    ~QGstreamerFormatInfo() override;

    QGstCaps formatCaps(const QMediaFormat &f) const;
    QGstCaps audioCaps(const QMediaFormat &f) const;
    QGstCaps videoCaps(const QMediaFormat &f) const;

    static QMediaFormat::AudioCodec audioCodecForCaps(QGstStructureView structure);
    static QMediaFormat::VideoCodec videoCodecForCaps(QGstStructureView structure);
    static QMediaFormat::FileFormat fileFormatForCaps(QGstStructureView structure);
    static QImageCapture::FileFormat imageFormatForCaps(QGstStructureView structure);

private:
    QList<CodecMap> getCodecMaps(QMediaFormat::ConversionMode conversionMode, QList<QMediaFormat::AudioCodec> audioCodecs,
                                 QList<QMediaFormat::VideoCodec> videoCodecs);
};

QT_END_NAMESPACE

#endif
