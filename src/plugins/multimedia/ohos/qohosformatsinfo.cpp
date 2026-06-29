// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosformatsinfo_p.h"

#include <QtMultimedia/qimagecapture.h>

#include <multimedia/player_framework/native_avcapability.h>
#include <multimedia/player_framework/native_avcodec_base.h>

QT_BEGIN_NAMESPACE

namespace {

bool codecAvailable(const char *mime, bool encoder)
{
    return mime && OH_AVCodec_GetCapability(mime, encoder) != nullptr;
}

QMediaFormat::AudioCodec audioCodecIfAvailable(QMediaFormat::AudioCodec codec, bool encoder)
{
    const char *mime = nullptr;
    switch (codec) {
    case QMediaFormat::AudioCodec::AAC:    mime = OH_AVCODEC_MIMETYPE_AUDIO_AAC; break;
    case QMediaFormat::AudioCodec::MP3:    mime = OH_AVCODEC_MIMETYPE_AUDIO_MPEG; break;
    case QMediaFormat::AudioCodec::FLAC:   mime = OH_AVCODEC_MIMETYPE_AUDIO_FLAC; break;
    case QMediaFormat::AudioCodec::Vorbis: mime = OH_AVCODEC_MIMETYPE_AUDIO_VORBIS; break;
    case QMediaFormat::AudioCodec::Opus:   mime = OH_AVCODEC_MIMETYPE_AUDIO_OPUS; break;
    case QMediaFormat::AudioCodec::Wave:
        // WAVE / PCM is always supported by the muxer/demuxer; no codec query needed.
        return codec;
    default:
        break;
    }
    return codecAvailable(mime, encoder) ? codec : QMediaFormat::AudioCodec::Unspecified;
}

QMediaFormat::VideoCodec videoCodecIfAvailable(QMediaFormat::VideoCodec codec, bool encoder)
{
    const char *mime = nullptr;
    switch (codec) {
    case QMediaFormat::VideoCodec::H264:  mime = OH_AVCODEC_MIMETYPE_VIDEO_AVC; break;
    case QMediaFormat::VideoCodec::H265:  mime = OH_AVCODEC_MIMETYPE_VIDEO_HEVC; break;
    case QMediaFormat::VideoCodec::MPEG4: mime = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4; break;
    default:
        break;
    }
    return codecAvailable(mime, encoder) ? codec : QMediaFormat::VideoCodec::Unspecified;
}

void pruneUnspecified(QList<QPlatformMediaFormatInfo::CodecMap> &maps)
{
    for (auto &m : maps) {
        m.audio.removeAll(QMediaFormat::AudioCodec::Unspecified);
        m.video.removeAll(QMediaFormat::VideoCodec::Unspecified);
    }
    erase_if(maps, [](const QPlatformMediaFormatInfo::CodecMap &m) {
        return m.audio.isEmpty() && m.video.isEmpty();
    });
}

} // namespace

QOhosFormatsInfo::QOhosFormatsInfo()
{
    // Decoders — what we can play back via OH_AVPlayer.
    {
        const auto aac    = audioCodecIfAvailable(QMediaFormat::AudioCodec::AAC,    false);
        const auto mp3    = audioCodecIfAvailable(QMediaFormat::AudioCodec::MP3,    false);
        const auto flac   = audioCodecIfAvailable(QMediaFormat::AudioCodec::FLAC,   false);
        const auto vorbis = audioCodecIfAvailable(QMediaFormat::AudioCodec::Vorbis, false);
        const auto opus   = audioCodecIfAvailable(QMediaFormat::AudioCodec::Opus,   false);
        const auto wav    = audioCodecIfAvailable(QMediaFormat::AudioCodec::Wave,   false);

        const auto h264  = videoCodecIfAvailable(QMediaFormat::VideoCodec::H264,  false);
        const auto h265  = videoCodecIfAvailable(QMediaFormat::VideoCodec::H265,  false);
        const auto mpeg4 = videoCodecIfAvailable(QMediaFormat::VideoCodec::MPEG4, false);

        decoders = {
            { QMediaFormat::AAC,        { aac }, {} },
            { QMediaFormat::MP3,        { mp3 }, {} },
            { QMediaFormat::FLAC,       { flac }, {} },
            { QMediaFormat::Wave,       { wav }, {} },
            { QMediaFormat::Mpeg4Audio, { mp3, aac, flac, vorbis }, {} },
            { QMediaFormat::MPEG4,      { mp3, aac, flac, vorbis }, { h264, h265, mpeg4 } },
            { QMediaFormat::Ogg,        { opus, vorbis, flac }, {} },
        };
        pruneUnspecified(decoders);
    }

    // Encoders — what OH_AVRecorder will accept.
    {
        const auto aac = audioCodecIfAvailable(QMediaFormat::AudioCodec::AAC, true);
        const auto mp3 = audioCodecIfAvailable(QMediaFormat::AudioCodec::MP3, true);
        const auto wav = audioCodecIfAvailable(QMediaFormat::AudioCodec::Wave, true);

        const auto h264  = videoCodecIfAvailable(QMediaFormat::VideoCodec::H264,  true);
        const auto h265  = videoCodecIfAvailable(QMediaFormat::VideoCodec::H265,  true);
        const auto mpeg4 = videoCodecIfAvailable(QMediaFormat::VideoCodec::MPEG4, true);

        encoders = {
            { QMediaFormat::AAC,        { aac }, {} },
            { QMediaFormat::MP3,        { mp3 }, {} },
            { QMediaFormat::Wave,       { wav }, {} },
            { QMediaFormat::Mpeg4Audio, { aac }, {} },
            { QMediaFormat::MPEG4,      { aac, mp3 }, { h264, h265, mpeg4 } },
        };
        pruneUnspecified(encoders);
    }

    imageFormats << QImageCapture::JPEG;

    fixupCodecMaps();
}

QOhosFormatsInfo::~QOhosFormatsInfo() = default;

QT_END_NAMESPACE
