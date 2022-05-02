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

#include "qplatformmediaformatinfo_p.h"
#include <qset.h>

QT_BEGIN_NAMESPACE

QPlatformMediaFormatInfo::QPlatformMediaFormatInfo() = default;

QPlatformMediaFormatInfo::~QPlatformMediaFormatInfo() = default;

QList<QMediaFormat::FileFormat> QPlatformMediaFormatInfo::supportedFileFormats(const QMediaFormat &constraints, QMediaFormat::ConversionMode m) const
{
    QSet<QMediaFormat::FileFormat> formats;

    const auto &codecMap = (m == QMediaFormat::Encode) ? encoders : decoders;
    for (const auto &m : codecMap) {
        if (constraints.audioCodec() != QMediaFormat::AudioCodec::Unspecified && !m.audio.contains(constraints.audioCodec()))
            continue;
        if (constraints.videoCodec() != QMediaFormat::VideoCodec::Unspecified && !m.video.contains(constraints.videoCodec()))
            continue;
        formats.insert(m.format);
    }
    return formats.values();
}

QList<QMediaFormat::AudioCodec> QPlatformMediaFormatInfo::supportedAudioCodecs(const QMediaFormat &constraints, QMediaFormat::ConversionMode m) const
{
    QSet<QMediaFormat::AudioCodec> codecs;

    const auto &codecMap = (m == QMediaFormat::Encode) ? encoders : decoders;
    for (const auto &m : codecMap) {
        if (constraints.fileFormat() != QMediaFormat::UnspecifiedFormat && m.format != constraints.fileFormat())
            continue;
        if (constraints.videoCodec() != QMediaFormat::VideoCodec::Unspecified && !m.video.contains(constraints.videoCodec()))
            continue;
        for (const auto &c : m.audio)
            codecs.insert(c);
    }
    return codecs.values();
}

QList<QMediaFormat::VideoCodec> QPlatformMediaFormatInfo::supportedVideoCodecs(const QMediaFormat &constraints, QMediaFormat::ConversionMode m) const
{
    QSet<QMediaFormat::VideoCodec> codecs;

    const auto &codecMap = (m == QMediaFormat::Encode) ? encoders : decoders;
    for (const auto &m : codecMap) {
        if (constraints.fileFormat() != QMediaFormat::UnspecifiedFormat && m.format != constraints.fileFormat())
            continue;
        if (constraints.audioCodec() != QMediaFormat::AudioCodec::Unspecified && !m.audio.contains(constraints.audioCodec()))
            continue;
        for (const auto &c : m.video)
            codecs.insert(c);
    }
    return codecs.values();
}

bool QPlatformMediaFormatInfo::isSupported(const QMediaFormat &format, QMediaFormat::ConversionMode m) const
{
    const auto &codecMap = (m == QMediaFormat::Encode) ? encoders : decoders;

    for (const auto &m : codecMap) {
        if (m.format != format.fileFormat())
            continue;
        if (!m.audio.contains(format.audioCodec()))
            continue;
        if (format.videoCodec() != QMediaFormat::VideoCodec::Unspecified && !m.video.contains(format.videoCodec()))
            continue;
        return true;
    }
    return false;
}

QT_END_NAMESPACE
