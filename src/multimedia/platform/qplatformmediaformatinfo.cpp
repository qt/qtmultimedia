/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
