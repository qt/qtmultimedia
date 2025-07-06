// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_spa_pod_support_p.h"

#include "qpipewire_support_p.h"

#include <QtCore/qdebug.h>

#include <pipewire/version.h>
#include <spa/pod/parser.h>
#include <spa/param/format.h>

#if __has_include(<spa/param/audio/raw-utils.h>)
#  include <spa/param/audio/raw-utils.h>
#else
#  include "qpipewire_spa_compat_p.h"
#endif

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

namespace {

std::optional<std::variant<spa_audio_format, SpaEnum<spa_audio_format>>>
parseSampleFormat(const spa_pod &pod)
{
    const spa_pod *format_pod = nullptr;
    int res = spa_pod_parse_object(&pod, SPA_TYPE_OBJECT_Format, nullptr, SPA_FORMAT_AUDIO_format,
                                   SPA_POD_OPT_PodChoice(&format_pod));
    if (res < 0)
        return std::nullopt;

    if (!format_pod) {
        qWarning() << "parseSampleFormat: parse error" << pod;
        return std::nullopt;
    }

    if (spa_pod_is_choice(format_pod)) {
        switch (SPA_POD_CHOICE_TYPE(format_pod)) {
        case SPA_CHOICE_Enum: {
            std::optional<SpaEnum<spa_audio_format>> formats =
                    SpaEnum<spa_audio_format>::parse(format_pod);
            if (formats)
                return *formats;

            qWarning() << "cannot parse audio format";
            return std::nullopt;
        }

        default:
            // TODO: can we obtain the full list of supported types?
            qWarning() << "unhandled type" << SPA_POD_CHOICE_TYPE(format_pod);
            return std::nullopt;
        }
    }

    return std::nullopt;
}

std::optional<std::variant<int, std::vector<int>, SpaRange<int>>>
parseSamplingRates(const spa_pod &pod)
{
    const spa_pod *rate_pod = nullptr;
    int res = spa_pod_parse_object(&pod, SPA_TYPE_OBJECT_Format, nullptr, SPA_FORMAT_AUDIO_rate,
                                   SPA_POD_OPT_PodChoice(&rate_pod));
    if (res < 0)
        return std::nullopt;

    if (!rate_pod) {
        qWarning() << "parseSamplingRates: parse error" << pod;
        return std::nullopt;
    }

    if (spa_pod_is_choice(rate_pod)) {
        switch (SPA_POD_CHOICE_TYPE(rate_pod)) {
        case SPA_CHOICE_Range: {
            std::optional<SpaRange<int>> range = SpaRange<int>::parse(rate_pod);
            if (range)
                return *range;

            qWarning() << "cannot parse sampling rates";
            return std::nullopt;
        }

        default:
            // TODO: can we obtain the full list of supported rates?
            qWarning() << "unhandled type" << SPA_POD_CHOICE_TYPE(rate_pod);
            return std::nullopt;
        }
    }

    return std::nullopt;
}

} // namespace

std::optional<SpaObjectAudioFormat> SpaObjectAudioFormat::parse(const spa_pod_object *obj)
{
    struct spa_audio_info_raw info = {};
    if (spa_format_audio_raw_parse(&obj->pod, &info) < 0)
        return std::nullopt;

    SpaObjectAudioFormat result;
    result.channelCount = int(info.channels);

    if (info.format != spa_audio_format::SPA_AUDIO_FORMAT_UNKNOWN) {
        result.sampleTypes = info.format;
    } else {
        auto optionalSampleFormat = parseSampleFormat(obj->pod);
        if (!optionalSampleFormat)
            return std::nullopt;

        result.sampleTypes = std::move(*optionalSampleFormat);
    }

    if (info.rate != 0) {
        result.rates = int(info.rate);
    } else {
        auto optionalSamplingRates = parseSamplingRates(obj->pod);
        if (!optionalSamplingRates)
            return std::nullopt;
        result.rates = std::move(*optionalSamplingRates);
    }

    for (int channelIndex = 0; channelIndex != result.channelCount; ++channelIndex)
        result.channelPositions.push_back(spa_audio_channel(info.position[channelIndex]));

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
