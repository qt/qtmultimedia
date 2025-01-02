// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxformatinfo_p.h"

QT_BEGIN_NAMESPACE

QQnxFormatInfo::QQnxFormatInfo()
{
    // ### This is probably somewhat correct for encoding, but should be checked
    encoders = {
                { QMediaFormat::MPEG4,
                 { QMediaFormat::AudioCodec::AAC },
                 { QMediaFormat::VideoCodec::H264 } },
                { QMediaFormat::Mpeg4Audio,
                 { QMediaFormat::AudioCodec::AAC },
                 {} },
                { QMediaFormat::Wave,
                 { QMediaFormat::AudioCodec::Wave },
                 {} },
                { QMediaFormat::AAC,
                 { QMediaFormat::AudioCodec::AAC },
                 {} },
                };

    // ### There can apparently be more codecs and demuxers installed on the system as plugins
    // Need to find a way to determine the list at compile time or runtime
    decoders = encoders;

    // ###
    imageFormats << QImageCapture::JPEG;
}

QQnxFormatInfo::~QQnxFormatInfo() = default;

QT_END_NAMESPACE
