// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinformatsinfo_p.h"

#include <QtMultimedia/private/qapple_utils_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/private/qcore_mac_p.h>

#include <AVFoundation/AVFoundation.h>
#include <AudioToolbox/AudioToolbox.h>
#include <VideoToolbox/VideoToolbox.h>

QT_BEGIN_NAMESPACE

namespace {

constexpr struct
{
    const char *__nonnull name;
    QMediaFormat::FileFormat fileFormat;
    // For single-codec containers the codec is implicit in the MIME type itself — no extended
    // MIME type probe needed. Unspecified means probe via audioCodecMap instead.
    QMediaFormat::AudioCodec implicitCodec;
} mediaContainerMap[] = {
    { "video/x-ms-asf", QMediaFormat::WMV, QMediaFormat::AudioCodec::Unspecified },
    { "video/avi", QMediaFormat::AVI, QMediaFormat::AudioCodec::Unspecified },
    { "video/x-matroska", QMediaFormat::Matroska, QMediaFormat::AudioCodec::Unspecified },
    { "video/mp4", QMediaFormat::MPEG4, QMediaFormat::AudioCodec::Unspecified },
    { "video/quicktime", QMediaFormat::QuickTime, QMediaFormat::AudioCodec::Unspecified },
    { "audio/ogg", QMediaFormat::Ogg, QMediaFormat::AudioCodec::Unspecified },
    { "audio/mp4", QMediaFormat::Mpeg4Audio, QMediaFormat::AudioCodec::Unspecified },
    { "audio/aac", QMediaFormat::AAC, QMediaFormat::AudioCodec::AAC },
    { "audio/mp3", QMediaFormat::MP3, QMediaFormat::AudioCodec::MP3 },
    { "audio/flac", QMediaFormat::FLAC, QMediaFormat::AudioCodec::FLAC },
    { "audio/x-wav", QMediaFormat::Wave, QMediaFormat::AudioCodec::Wave },
};

constexpr struct {
    const char *__nonnull name;
    QMediaFormat::VideoCodec codec;
} videoCodecMap[] = {
    // See CMVideoCodecType for the four character code names of codecs
    { "; codecs=\"mp1v\"", QMediaFormat::VideoCodec::MPEG1 },
    { "; codecs=\"mp2v\"", QMediaFormat::VideoCodec::MPEG2 },
    { "; codecs=\"mp4v\"", QMediaFormat::VideoCodec::MPEG4 },
    { "; codecs=\"avc1\"", QMediaFormat::VideoCodec::H264 },
    { "; codecs=\"hvc1\"", QMediaFormat::VideoCodec::H265 },
    { "; codecs=\"vp09\"", QMediaFormat::VideoCodec::VP9 },
    { "; codecs=\"av01\"", QMediaFormat::VideoCodec::AV1 },
    { "; codecs=\"jpeg\"", QMediaFormat::VideoCodec::MotionJPEG },
};

constexpr struct {
    const char *__nonnull name;
    QMediaFormat::AudioCodec codec;
} audioCodecMap[] = {
    // RFC 6381 / ISO 14496-3 codec strings understood by isPlayableExtendedMIMEType
    { "; codecs=\"mp4a.40.2\"", QMediaFormat::AudioCodec::AAC    },  // AAC-LC
    { "; codecs=\"ac-3\"",      QMediaFormat::AudioCodec::AC3    },
    { "; codecs=\"ec-3\"",      QMediaFormat::AudioCodec::EAC3   },
    { "; codecs=\"flac\"",      QMediaFormat::AudioCodec::FLAC   },
    { "; codecs=\"alac\"",      QMediaFormat::AudioCodec::ALAC   },
    { "; codecs=\"opus\"",      QMediaFormat::AudioCodec::Opus   },
    { "; codecs=\"vorbis\"",    QMediaFormat::AudioCodec::Vorbis },
};

bool canEncodeAudioFormat(AudioFormatID formatID)
{
    if (formatID == kAudioFormatLinearPCM)
        return true;
    UInt32 size = 0;
    return AudioFormatGetPropertyInfo(kAudioFormatProperty_Encoders,
                                      sizeof(formatID), &formatID, &size) == noErr
            && size > 0;
}

bool canEncodeDecodeFormat(AudioFormatID formatID)
{
    if (formatID == kAudioFormatLinearPCM)
        return true;
    UInt32 size = 0;
    return AudioFormatGetPropertyInfo(kAudioFormatProperty_Decoders, sizeof(formatID), &formatID,
                                      &size)
            == noErr
            && size > 0;
}

std::set<CMVideoCodecType> availableVTVideoEncoderCodecTypes()
{
    using namespace QtMultimediaPrivate;

    QCFType<CFArrayRef> list;
    if (VTCopyVideoEncoderList(nullptr, &list) != noErr || !list)
        return {};

    auto results =
            CFArrayRange<CFDictionaryRef>(list) | views::transform([](CFDictionaryRef entry) {
        auto *typeRef = CFNumberRef(CFDictionaryGetValue(entry, kVTVideoEncoderList_CodecType));
        CMVideoCodecType type = 0;
        CFNumberGetValue(typeRef, kCFNumberSInt32Type, &type);
        return type;
    });

    return results | ranges::to<std::set<CMVideoCodecType>>();
}

constexpr bool isVideoContainer(QMediaFormat::FileFormat fileFormat)
{
    return fileFormat < QMediaFormat::Mpeg4Audio;
}

bool isPlayableExtendedMIMEType(const char *extendedMimetype)
{
    return [AVURLAsset isPlayableExtendedMIMEType:[NSString stringWithUTF8String:extendedMimetype]];
}

bool isPlayableExtendedMIMEType(const char *containerName, const char *codecName)
{
    const QByteArray extendedMimetype = QByteArrayView(containerName) + codecName;
    return isPlayableExtendedMIMEType(extendedMimetype.constData());
}

constexpr auto recordGetCodec = QtMultimediaPrivate::views::transform([](const auto &record) {
    return record.codec;
});

QList<QPlatformMediaFormatInfo::CodecMap> enumerateDecoders()
{
    namespace views = QtMultimediaPrivate::views;
    namespace ranges = QtMultimediaPrivate::ranges;
    using AudioCodec = QMediaFormat::AudioCodec;
    using VideoCodec = QMediaFormat::VideoCodec;
    using CodecMap = QPlatformMediaFormatInfo::CodecMap;

    const bool canDecodeMp3 = canEncodeDecodeFormat(kAudioFormatMPEGLayer3);

    QList<CodecMap> decoders;
    auto avtypes = [AVURLAsset audiovisualMIMETypes];

    for (AVFileType filetype : avtypes) {
        for (auto [containerName, fileFormat, implicitCodec] : mediaContainerMap) {
            if (strcmp(filetype.UTF8String, containerName))
                continue;

            QList<VideoCodec> video;
            QList<AudioCodec> audio;

            if (implicitCodec != QMediaFormat::AudioCodec::Unspecified) {
                // Single-codec container: the MIME type appearing in audiovisualMIMETypes is
                // sufficient proof of support — no extended MIME type probe needed.
                audio << implicitCodec;
            } else {
                // Multi-codec container: probe each codec via extended MIME type.

                if (isVideoContainer(fileFormat)) {
                    switch (fileFormat) {
                    case QMediaFormat::AVI: {
                        // isPlayableExtendedMIMEType returns false for AVI+video combinations even
                        // when AVFoundation can actually decode them.
                        if (!isPlayableExtendedMIMEType(containerName))
                            break;

                        // AVI is a legacy container format that can carry a wide variety of codecs.
                        // Hardcode the known supported codecs instead of probing via extended MIME type.
                        video << VideoCodec::MPEG4 << VideoCodec::H264 << VideoCodec::MotionJPEG;
                        break;
                    }

                    default: {
                        video = videoCodecMap | views::filter([&](const auto &record) {
                            return isPlayableExtendedMIMEType(containerName, record.name);
                        }) | recordGetCodec
                                | ranges::to<QList<VideoCodec>>();
                    }
                    };
                }

                audio = audioCodecMap | views::filter([&](const auto &record) {
                    return isPlayableExtendedMIMEType(containerName, record.name);
                }) | recordGetCodec
                        | ranges::to<QList<AudioCodec>>();

                // MP3 has no RFC 6381 codec string that isPlayableExtendedMIMEType recognises
                // (neither ".mp3" nor "mp4a.6B" work). Add it manually for containers that
                // are known to carry it: MPEG-4 family and Mpeg4Audio.
                if (canDecodeMp3
                    && (isVideoContainer(fileFormat) || fileFormat == QMediaFormat::Mpeg4Audio))
                    audio << AudioCodec::MP3;
            }

            decoders << CodecMap{
                fileFormat,
                std::move(audio),
                std::move(video),
            };
        }
    }
    return decoders;
}

QList<QPlatformMediaFormatInfo::CodecMap> enumerateEncoders()
{
    namespace views = QtMultimediaPrivate::views;
    namespace ranges = QtMultimediaPrivate::ranges;
    using AudioCodec = QMediaFormat::AudioCodec;
    using VideoCodec = QMediaFormat::VideoCodec;
    using CodecMap = QPlatformMediaFormatInfo::CodecMap;

    static constexpr struct AudioProbeRecord
    {
        AudioFormatID fmtId;
        AudioCodec codec;
    } audioProbes[] = {
        { kAudioFormatMPEG4AAC, AudioCodec::AAC },
        { kAudioFormatMPEGLayer3, AudioCodec::MP3 },
        { kAudioFormatAC3, AudioCodec::AC3 },
        { kAudioFormatEnhancedAC3, AudioCodec::EAC3 },
        { kAudioFormatFLAC, AudioCodec::FLAC },
        { kAudioFormatAppleLossless, AudioCodec::ALAC },
        { kAudioFormatOpus, AudioCodec::Opus },
        { kAudioFormatLinearPCM, AudioCodec::Wave },

        // no API constants, but the fourcc is supported
        { 'vorb', QMediaFormat::AudioCodec::Vorbis },
    };

    static constexpr struct VideoProbeRecord
    {
        CMVideoCodecType type;
        VideoCodec codec;
    } videoProbes[] = {
        { kCMVideoCodecType_H264, VideoCodec::H264 },
        { kCMVideoCodecType_HEVC, VideoCodec::H265 },
        { kCMVideoCodecType_JPEG, VideoCodec::MotionJPEG },
        { kCMVideoCodecType_MPEG4Video, VideoCodec::MPEG4 },
        { kCMVideoCodecType_AV1, VideoCodec::AV1 },

        // no API constants, but the fourcc is supported
        { 'mp1v', VideoCodec::MPEG1 },
        { 'mp2v', VideoCodec::MPEG2 },
    };

    auto filterCodecs = [](const auto &codecsSet) {
        return views::filter([&](auto c) {
            return codecsSet.count(c);
        });
    };

    const std::set<AudioCodec> supportedAudioCodecs = audioProbes
            | views::filter([](const auto &record) {
        return canEncodeAudioFormat(record.fmtId);
    }) | recordGetCodec
            | ranges::to<std::set<AudioCodec>>();

    const std::set<CMVideoCodecType> vtCodecTypes = availableVTVideoEncoderCodecTypes();

    const std::set<VideoCodec> supportedVideoCodecs = videoProbes
            | views::filter([&](const auto &record) {
        return ranges::contains(vtCodecTypes, record.type);
    }) | recordGetCodec
            | ranges::to<std::set<VideoCodec>>();

    auto audioFormats = [&](auto candidates) -> QList<AudioCodec> {
        return candidates | filterCodecs(supportedAudioCodecs) | ranges::to<QList<AudioCodec>>();
    };

    auto videoFormats = [&](auto candidates) -> QList<VideoCodec> {
        return candidates | filterCodecs(supportedVideoCodecs) | ranges::to<QList<VideoCodec>>();
    };

    // Video containers share the same codec candidates; audio candidates differ by container
    // spec (e.g. Mpeg4Audio is audio-only, Wave only accepts LPCM).
    const QList allSupportedVideoCodecs = videoFormats(videoProbes | recordGetCodec);
    const QList allSupportedAudioCodecs = audioFormats(audioProbes | recordGetCodec);

    return QList{
        CodecMap{ QMediaFormat::MPEG4, allSupportedAudioCodecs, allSupportedVideoCodecs },
        CodecMap{ QMediaFormat::QuickTime, allSupportedAudioCodecs, allSupportedVideoCodecs },
        CodecMap{ QMediaFormat::Mpeg4Audio,
                  audioFormats(std::array{ AudioCodec::AAC, AudioCodec::ALAC, AudioCodec::FLAC }),
                  {} },
        CodecMap{ QMediaFormat::Wave, audioFormats(std::array{ AudioCodec::Wave }), {} },
    };
}

} // namespace

QDarwinFormatInfo::QDarwinFormatInfo()
{
    decoders = enumerateDecoders();
    encoders = enumerateEncoders();

    // ###
    imageFormats << QImageCapture::JPEG;

    fixupCodecMaps();
}

QDarwinFormatInfo::~QDarwinFormatInfo() = default;

int QDarwinFormatInfo::audioFormatForCodec(QMediaFormat::AudioCodec codec)
{
    int codecId = kAudioFormatMPEG4AAC;
    switch (codec) {
    case QMediaFormat::AudioCodec::Unspecified:
    case QMediaFormat::AudioCodec::DolbyTrueHD:
    case QMediaFormat::AudioCodec::Vorbis:
    case QMediaFormat::AudioCodec::WMA:
        // Unsupported, shouldn't happen. Fall back to AAC
    case QMediaFormat::AudioCodec::AAC:
        codecId = kAudioFormatMPEG4AAC;
        break;
    case QMediaFormat::AudioCodec::MP3:
        codecId = kAudioFormatMPEGLayer3;
        break;
    case QMediaFormat::AudioCodec::AC3:
        codecId = kAudioFormatAC3;
        break;
    case QMediaFormat::AudioCodec::EAC3:
        codecId = kAudioFormatEnhancedAC3;
        break;
    case QMediaFormat::AudioCodec::FLAC:
        codecId = kAudioFormatFLAC;
        break;
    case QMediaFormat::AudioCodec::ALAC:
        codecId = kAudioFormatAppleLossless;
        break;
    case QMediaFormat::AudioCodec::Opus:
        codecId = kAudioFormatOpus;
        break;
    case QMediaFormat::AudioCodec::Wave:
        codecId = kAudioFormatLinearPCM;
    }
    return codecId;
}

NSString *QDarwinFormatInfo::videoFormatForCodec(QMediaFormat::VideoCodec codec)
{
    switch (codec) {
    case QMediaFormat::VideoCodec::MPEG1:
        return @"mp1v";
        break;
    case QMediaFormat::VideoCodec::MPEG2:
        return @"mp2v";
        break;
    case QMediaFormat::VideoCodec::MPEG4:
        return @"mp4v";
        break;
    case QMediaFormat::VideoCodec::H264:
        return @"avc1";
        break;
    case QMediaFormat::VideoCodec::VP9:
        return @"vp09";
        break;
    case QMediaFormat::VideoCodec::MotionJPEG:
        return @"jpeg";
    case QMediaFormat::VideoCodec::H265:
        return @"hvc1";
    case QMediaFormat::VideoCodec::AV1:
        return @"av01";
    case QMediaFormat::VideoCodec::Unspecified:
    case QMediaFormat::VideoCodec::VP8:
    case QMediaFormat::VideoCodec::Theora:
    case QMediaFormat::VideoCodec::WMV:
    default:
        qDebug() << "Unsupported video codec" << codec << ", using H.265 as fallback";
        return @"hvc1";
    }
}

NSString *QDarwinFormatInfo::avFileTypeForContainerFormat(QMediaFormat::FileFormat container)
{
    switch (container) {
    case QMediaFormat::MPEG4:
        return AVFileTypeMPEG4;
    case QMediaFormat::QuickTime:
        return AVFileTypeQuickTimeMovie;
    case QMediaFormat::MP3:
        return AVFileTypeMPEGLayer3;
    case QMediaFormat::Mpeg4Audio:
        return AVFileTypeAppleM4A;
    case QMediaFormat::Wave:
        return AVFileTypeWAVE;
    default:
        return AVFileTypeQuickTimeMovie;
    }
}

QT_END_NAMESPACE
