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
#include <qhash.h>
#include <qlist.h>
#include <qgstutils_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerFormatInfo : public QPlatformMediaFormatInfo
{
public:
    QGstreamerFormatInfo();
    ~QGstreamerFormatInfo();

    QGstCaps formatCaps(const QMediaFormat &f) const;
    QGstCaps audioCaps(const QMediaFormat &f) const;
    QGstCaps videoCaps(const QMediaFormat &f) const;

    static QMediaFormat::AudioCodec audioCodecForCaps(QGstStructure structure);
    static QMediaFormat::VideoCodec videoCodecForCaps(QGstStructure structure);
    static QMediaFormat::FileFormat fileFormatForCaps(QGstStructure structure);
    static QImageCapture::FileFormat imageFormatForCaps(QGstStructure structure);

    QList<CodecMap> getMuxerList(bool demuxer, QList<QMediaFormat::AudioCodec> audioCodecs, QList<QMediaFormat::VideoCodec> videoCodecs);
};

QT_END_NAMESPACE

#endif
