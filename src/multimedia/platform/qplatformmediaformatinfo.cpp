// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformmediaformatinfo_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>

#include <set>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcMediaFormatInfo, "qt.multimedia.mediaformatinfo", QtWarningMsg)

QPlatformMediaFormatInfo::QPlatformMediaFormatInfo() = default;

QPlatformMediaFormatInfo::~QPlatformMediaFormatInfo() = default;

QList<QMediaFormat::FileFormat> QPlatformMediaFormatInfo::supportedFileFormats(const QMediaFormat &constraints, QMediaFormat::ConversionMode m) const
{
    std::set<QMediaFormat::FileFormat> formats;

    const auto &codecMap = (m == QMediaFormat::Encode) ? encoders : decoders;
    for (const auto &m : codecMap) {
        if (constraints.audioCodec() != QMediaFormat::AudioCodec::Unspecified && !m.audio.contains(constraints.audioCodec()))
            continue;
        if (constraints.videoCodec() != QMediaFormat::VideoCodec::Unspecified && !m.video.contains(constraints.videoCodec()))
            continue;
        formats.insert(m.format);
    }
    return { formats.begin(), formats.end() };
}

QList<QMediaFormat::AudioCodec> QPlatformMediaFormatInfo::supportedAudioCodecs(const QMediaFormat &constraints, QMediaFormat::ConversionMode m) const
{
    std::set<QMediaFormat::AudioCodec> codecs;

    const auto &codecMap = (m == QMediaFormat::Encode) ? encoders : decoders;
    for (const auto &m : codecMap) {
        if (constraints.fileFormat() != QMediaFormat::UnspecifiedFormat && m.format != constraints.fileFormat())
            continue;
        if (constraints.videoCodec() != QMediaFormat::VideoCodec::Unspecified && !m.video.contains(constraints.videoCodec()))
            continue;
        for (const auto &c : m.audio)
            codecs.insert(c);
    }

    return { codecs.begin(), codecs.end() };
}

QList<QMediaFormat::VideoCodec> QPlatformMediaFormatInfo::supportedVideoCodecs(const QMediaFormat &constraints, QMediaFormat::ConversionMode m) const
{
    std::set<QMediaFormat::VideoCodec> codecs;

    const auto &codecMap = (m == QMediaFormat::Encode) ? encoders : decoders;
    for (const auto &m : codecMap) {
        if (constraints.fileFormat() != QMediaFormat::UnspecifiedFormat && m.format != constraints.fileFormat())
            continue;
        if (constraints.audioCodec() != QMediaFormat::AudioCodec::Unspecified && !m.audio.contains(constraints.audioCodec()))
            continue;
        for (const auto &c : m.video)
            codecs.insert(c);
    }
    return { codecs.begin(), codecs.end() };
}

bool QPlatformMediaFormatInfo::isSupported(const QMediaFormat &format, QMediaFormat::ConversionMode m) const
{
    const auto &codecMap = (m == QMediaFormat::Encode) ? encoders : decoders;

    for (const auto &m : codecMap) {
        if (m.format != format.fileFormat())
            continue;
        if (format.audioCodec() != QMediaFormat::AudioCodec::Unspecified && !m.audio.contains(format.audioCodec()))
            continue;
        if (format.videoCodec() != QMediaFormat::VideoCodec::Unspecified && !m.video.contains(format.videoCodec()))
            continue;
        return true;
    }
    return false;
}

void QPlatformMediaFormatInfo::fixupCodecMaps()
{
    // Remove codecs that are not supported in the given container format. This is needed because some backends report
    // codecs that are not actually supported in a given container format or that are rather edge cases.

    using FileFormat = QMediaFormat::FileFormat;
    using AudioCodec = QMediaFormat::AudioCodec;
    using VideoCodec = QMediaFormat::VideoCodec;
    namespace ranges = QtMultimediaPrivate::ranges;

    auto fixupMap = [](CodecMap &cm) {
        auto removeInvalidAudio = [&](auto predicate) {
            cm.audio.removeIf([&](AudioCodec c) {
                if (predicate(c)) {
                    qCDebug(lcMediaFormatInfo)
                            << "Removing audio codec" << c << "unsupported in" << cm.format;
                    return true;
                }
                return false;
            });
        };

        auto removeInvalidVideo = [&](auto predicate) {
            cm.video.removeIf([&](VideoCodec c) {
                if (predicate(c)) {
                    qCDebug(lcMediaFormatInfo)
                            << "Removing video codec" << c << "unsupported in" << cm.format;
                    return true;
                }
                return false;
            });
        };

        auto fixupSingleCodecAudioContainer = [&](AudioCodec singleCodec) {
            removeInvalidAudio([&](AudioCodec c) {
                return c != singleCodec;
            });
            removeInvalidVideo([](VideoCodec) {
                return true;
            });
        };

        switch (cm.format) {
        case FileFormat::WebM:
            removeInvalidVideo([](VideoCodec c) {
                return !ranges::contains(
                        std::array{ VideoCodec::VP8, VideoCodec::VP9, VideoCodec::AV1 }, c);
            });
            removeInvalidAudio([](AudioCodec c) {
                return !ranges::contains(std::array{ AudioCodec::Vorbis, AudioCodec::Opus }, c);
            });
            break;
        case FileFormat::Ogg:
            removeInvalidVideo([](VideoCodec c) {
                return c != VideoCodec::Theora;
            });
            removeInvalidAudio([](AudioCodec c) {
                return !ranges::contains(
                        std::array{ AudioCodec::Vorbis, AudioCodec::Opus, AudioCodec::FLAC }, c);
            });
            break;
        case FileFormat::MPEG4:
        case FileFormat::QuickTime:
            removeInvalidVideo([](VideoCodec c) {
                return ranges::contains(std::array{ VideoCodec::VP8, VideoCodec::VP9,
                                                    VideoCodec::Theora, VideoCodec::WMV },
                                        c);
            });
            removeInvalidAudio([](AudioCodec c) {
                return ranges::contains(
                        std::array{ AudioCodec::Vorbis, AudioCodec::Opus, AudioCodec::WMA }, c);
            });
            break;
        case FileFormat::WMV:
            // Caveat: WMV files are ASF Containers with WMV video codec and WMA audio codec.
            removeInvalidVideo([](VideoCodec c) {
                return c != VideoCodec::WMV;
            });
            removeInvalidAudio([](AudioCodec c) {
                return c != AudioCodec::WMA;
            });
            break;
        case FileFormat::Wave:
            fixupSingleCodecAudioContainer(AudioCodec::Wave);
            break;
        case FileFormat::MP3:
            fixupSingleCodecAudioContainer(AudioCodec::MP3);
            break;
        case FileFormat::WMA:
            // Caveat: WMA files are ASF Containers with WMA audio codec.
            fixupSingleCodecAudioContainer(AudioCodec::WMA);
            break;
        case FileFormat::AAC:
            fixupSingleCodecAudioContainer(AudioCodec::AAC);
            break;
        case FileFormat::FLAC:
            fixupSingleCodecAudioContainer(AudioCodec::FLAC);
            break;
        case FileFormat::Mpeg4Audio:
            removeInvalidVideo([](VideoCodec) {
                return true;
            });
            break;
        default:
            break;
        }
    };

    for (auto &cm : encoders)
        fixupMap(cm);
    for (auto &cm : decoders)
        fixupMap(cm);

    auto isEmpty = [](const CodecMap &cm) {
        return cm.audio.isEmpty() && cm.video.isEmpty();
    };
    encoders.removeIf(isEmpty);
    decoders.removeIf(isEmpty);
}

QDebug operator<<(QDebug dbg, const QPlatformMediaFormatInfo::CodecMap &m)
{
    const QDebugStateSaver saver(dbg);
    dbg.nospace() << "CodecMap(" << QMediaFormat::fileFormatName(m.format);
    if (!m.video.isEmpty()) {
        dbg << ", video: [";
        for (int i = 0; i < m.video.size(); ++i) {
            if (i)
                dbg << ", ";
            dbg << QMediaFormat::videoCodecName(m.video[i]);
        }
        dbg << ']';
    }
    if (!m.audio.isEmpty()) {
        dbg << ", audio: [";
        for (int i = 0; i < m.audio.size(); ++i) {
            if (i)
                dbg << ", ";
            dbg << QMediaFormat::audioCodecName(m.audio[i]);
        }
        dbg << ']';
    }
    dbg << ')';
    return dbg;
}

QT_END_NAMESPACE
