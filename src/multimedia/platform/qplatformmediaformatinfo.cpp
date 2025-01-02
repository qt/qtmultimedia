// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
