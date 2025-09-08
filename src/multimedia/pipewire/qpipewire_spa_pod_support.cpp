// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_spa_pod_support_p.h"

#include <QtCore/qdebug.h>

#include <pipewire/version.h>
#include <spa/pod/parser.h>
#include <spa/param/format.h>

#if __has_include(<spa/param/audio/raw-utils.h>)
#  include <spa/param/audio/raw-utils.h>
#else
#  include "qpipewire_spa_compat_p.h"
#endif

#if PW_CHECK_VERSION(0, 3, 44)
#  include <spa/param/audio/iec958.h>
#else
#  include "qpipewire_spa_compat_p.h"
static constexpr spa_format SPA_FORMAT_AUDIO_iec958Codec = spa_format(65542);
static constexpr spa_media_subtype SPA_MEDIA_SUBTYPE_iec958 = spa_media_subtype(3);
#endif

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

namespace {

std::optional<std::variant<spa_audio_format, SpaEnum<spa_audio_format>>>
parseSampleFormat(const spa_pod &pod)
{
    std::optional<spa_audio_format> format = spaParsePodPropertyScalar<spa_audio_format>(
            pod, SPA_TYPE_OBJECT_Format, SPA_FORMAT_AUDIO_format);
    if (format)
        return format;

    std::optional<SpaEnum<spa_audio_format>> choice =
            spaParsePodPropertyChoice<spa_audio_format, SPA_CHOICE_Enum>(
                    pod, SPA_TYPE_OBJECT_Format, SPA_FORMAT_AUDIO_format);
    if (choice)
        return *choice;
    return std::nullopt;
}

std::optional<std::variant<SpaRange<int>, int>> parseSamplingRates(const spa_pod &pod)
{
    std::optional<int> rate =
            spaParsePodPropertyScalar<int>(pod, SPA_TYPE_OBJECT_Format, SPA_FORMAT_AUDIO_format);
    if (rate)
        return rate;

    std::optional<SpaRange<int>> choice = spaParsePodPropertyChoice<int, SPA_CHOICE_Range>(
            pod, SPA_TYPE_OBJECT_Format, SPA_FORMAT_AUDIO_rate);
    if (choice)
        return *choice;
    return std::nullopt;
}

bool isIec958Device(const spa_pod &pod)
{
    return spaParsePodPropertyScalar<spa_media_subtype>(pod, SPA_TYPE_OBJECT_Format,
                                                        SPA_FORMAT_mediaSubtype)
            == SPA_MEDIA_SUBTYPE_iec958;
}

bool isIec958PCMDevice(const spa_pod &pod)
{
    std::optional<spa_audio_iec958_codec> codec = spaParsePodPropertyScalar<spa_audio_iec958_codec>(
            pod, SPA_TYPE_OBJECT_Format, SPA_FORMAT_AUDIO_iec958Codec);
    if (codec)
        return codec == spa_audio_iec958_codec::SPA_AUDIO_IEC958_CODEC_PCM;

    std::optional<SpaEnum<spa_audio_iec958_codec>> choice =
            spaParsePodPropertyChoice<spa_audio_iec958_codec, SPA_CHOICE_Enum>(
                    pod, SPA_TYPE_OBJECT_Format, SPA_FORMAT_AUDIO_iec958Codec);
    if (choice)
        return choice->defaultValue() == spa_audio_iec958_codec::SPA_AUDIO_IEC958_CODEC_PCM;
    return false;
}

} // namespace

std::optional<SpaObjectAudioFormat> SpaObjectAudioFormat::parse(const spa_pod_object *obj)
{
    struct spa_audio_info_raw info = {};
    if (spa_format_audio_raw_parse(&obj->pod, &info) < 0)
        return std::nullopt;

    SpaObjectAudioFormat result;
    result.channelCount = int(info.channels);

    bool isIec958 = isIec958Device(obj->pod);

    if (info.format != spa_audio_format::SPA_AUDIO_FORMAT_UNKNOWN) {
        result.sampleTypes = info.format;
    } else if (isIec958) {
        bool isIec958Pcm = isIec958PCMDevice(obj->pod);
        if (isIec958Pcm) {
            result.channelCount = 2; // IEC958 PCM is always stereo
            result.sampleTypes = spa_audio_iec958_codec::SPA_AUDIO_IEC958_CODEC_PCM;
        } else {
            return std::nullopt;
        }
    } else {
        auto optionalSampleFormat = parseSampleFormat(obj->pod);
        if (!optionalSampleFormat)
            return std::nullopt;

        std::visit([&](auto &&arg) {
            result.sampleTypes = std::forward<decltype(arg)>(arg);
        }, *optionalSampleFormat);
    }

    if (info.rate != 0) {
        result.rates = int(info.rate);
    } else {
        auto optionalSamplingRates = parseSamplingRates(obj->pod);
        if (!optionalSamplingRates)
            return std::nullopt;
        std::visit([&](auto arg) {
            result.rates = arg;
        }, *optionalSamplingRates);
    }

    if (isIec958) {
        // IEC958 PCM is always stereo, and the POD won't contain any information about channel
        // positioning.
    } else if (!SPA_FLAG_IS_SET(info.flags, SPA_AUDIO_FLAG_UNPOSITIONED)) {
        result.channelPositions = QList<spa_audio_channel>();
        for (int channelIndex = 0; channelIndex != result.channelCount; ++channelIndex)
            result.channelPositions->push_back(spa_audio_channel(info.position[channelIndex]));
    } else {
        // unpositionioned
    }

    return result;
}

std::optional<SpaObjectAudioFormat> SpaObjectAudioFormat::parse(const spa_pod *pod)
{
    if (spa_pod_is_object_type(pod, SPA_TYPE_OBJECT_Format)) {
        const spa_pod_object *obj = reinterpret_cast<const spa_pod_object *>(pod);
        return parse(obj);
    }
    return std::nullopt;
}

namespace {

spa_audio_format toSpaAudioFormat(QAudioFormat::SampleFormat fmt)
{
    switch (fmt) {
    case QAudioFormat::Int16:
        return SPA_AUDIO_FORMAT_S16;
    case QAudioFormat::Int32:
        return SPA_AUDIO_FORMAT_S32;
    case QAudioFormat::UInt8:
        return SPA_AUDIO_FORMAT_U8;
    case QAudioFormat::Float:
        return SPA_AUDIO_FORMAT_F32;
    default:
        return SPA_AUDIO_FORMAT_UNKNOWN;
    }
}

void initializeChannelPositions(spa_audio_info_raw &info, const QAudioFormat &fmt)
{
    using ChannelConfig = QAudioFormat::ChannelConfig;
    const ChannelConfig cfg = fmt.channelConfig();

    auto fillPositions = [&](QSpan<const spa_audio_channel> positions) {
        std::copy(positions.begin(), positions.end(), std::begin(info.position));
    };

    switch (cfg) {
    case ChannelConfig::ChannelConfigMono:
        return fillPositions(channelPositionsMono);

    case ChannelConfig::ChannelConfigStereo:
        return fillPositions(channelPositionsStereo);
    case ChannelConfig::ChannelConfig2Dot1:
        return fillPositions(channelPositions2Dot1);
    case ChannelConfig::ChannelConfig3Dot0:
        return fillPositions(channelPositions3Dot0);
    case ChannelConfig::ChannelConfig3Dot1:
        return fillPositions(channelPositions3Dot1);
    case ChannelConfig::ChannelConfigSurround5Dot0:
        return fillPositions(channelPositions5Dot0);
    case ChannelConfig::ChannelConfigSurround5Dot1:
        return fillPositions(channelPositions5Dot1);
    case ChannelConfig::ChannelConfigSurround7Dot0:
        return fillPositions(channelPositions7Dot0);
    case ChannelConfig::ChannelConfigSurround7Dot1:
        return fillPositions(channelPositions7Dot1);
    case ChannelConfig::ChannelConfigUnknown:
    default: {
#if !PW_CHECK_VERSION(0, 3, 33)
        uint32_t SPA_AUDIO_CHANNEL_START_Aux = 0x1000;
#endif

        // now we're in speculative territory: ChannelConfig is a bitmask and isn't
        // able to represent arbitrary channel configurations.
        //
        // as a "best effort", we can try to populate all channels as "Aux" channels
        // depending on the channel count
        std::iota(info.position, info.position + fmt.channelCount(),
                  uint32_t(SPA_AUDIO_CHANNEL_START_Aux));
        return;
    }
    }
}

} // namespace

spa_audio_info_raw asSpaAudioInfoRaw(const QAudioFormat &fmt)
{
    spa_audio_info_raw ret{
        .format = toSpaAudioFormat(fmt.sampleFormat()),
        .flags = {},
        .rate = uint32_t(fmt.sampleRate()),
        .channels = uint32_t(fmt.channelCount()),
        .position = {},
    };

    initializeChannelPositions(ret, fmt);

    return ret;
}

} // namespace QtPipeWire

QT_END_NAMESPACE
