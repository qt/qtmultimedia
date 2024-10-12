// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "formatutils_p.h"

QT_BEGIN_NAMESPACE

std::set<QMediaFormat::VideoCodec> allVideoCodecs(bool includeUnspecified)
{
    using VideoCodec = QMediaFormat::VideoCodec;
    std::set<VideoCodec> codecs;

    const int firstCodec = qToUnderlying(VideoCodec::Unspecified) + (includeUnspecified ? 0 : 1);
    constexpr int lastCodec = qToUnderlying(VideoCodec::LastVideoCodec);

    for (int i = firstCodec; i <= lastCodec; ++i)
        codecs.insert(static_cast<VideoCodec>(i));

    return codecs;
}

std::set<QMediaFormat::AudioCodec> allAudioCodecs(bool includeUnspecified)
{
    using AudioCodec = QMediaFormat::AudioCodec;
    std::set<AudioCodec> codecs;

    const int firstCodec = qToUnderlying(AudioCodec::Unspecified) + (includeUnspecified ? 0 : 1);
    constexpr int lastCodec = qToUnderlying(AudioCodec::LastAudioCodec);

    for (int i = firstCodec; i <= lastCodec; ++i)
        codecs.insert(static_cast<AudioCodec>(i));

    return codecs;
}

std::set<QMediaFormat::FileFormat> allFileFormats(bool includeUnspecified)
{
    using FileFormat = QMediaFormat::FileFormat;

    std::set<FileFormat> videoFormats;
    const int firstFormat = FileFormat::UnspecifiedFormat + (includeUnspecified ? 0 : 1);
    for (int i = firstFormat; i <= FileFormat::LastFileFormat; ++i) {
        const FileFormat format = static_cast<FileFormat>(i);
        videoFormats.insert(format);
    }
    return videoFormats;
}

std::vector<QMediaFormat> allMediaFormats(bool includeUnspecified)
{
    std::vector<QMediaFormat> formats;
    for (const auto &fileFormat : allFileFormats(includeUnspecified)) {
        for (const auto &audioCodec : allAudioCodecs(includeUnspecified)) {
            for (const auto &videoCodec : allVideoCodecs(includeUnspecified)) {
                QMediaFormat format{ fileFormat };
                format.setAudioCodec(audioCodec);
                format.setVideoCodec(videoCodec);
                formats.push_back(format);
            }
        }
    }
    return formats;
}


QT_END_NAMESPACE
