// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpulsehelpers_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcPulseAudioOut, "qt.multimedia.pulseaudio.output")
Q_LOGGING_CATEGORY(qLcPulseAudioIn, "qt.multimedia.pulseaudio.input")
Q_LOGGING_CATEGORY(qLcPulseAudioEngine, "qt.multimedia.pulseaudio.engine")

namespace QPulseAudioInternal
{
pa_sample_spec audioFormatToSampleSpec(const QAudioFormat &format)
{
    pa_sample_spec  spec;

    spec.rate = format.sampleRate();
    spec.channels = format.channelCount();
    spec.format = PA_SAMPLE_INVALID;
    const bool isBigEndian = QSysInfo::ByteOrder == QSysInfo::BigEndian;

    if (format.sampleFormat() == QAudioFormat::UInt8) {
        spec.format = PA_SAMPLE_U8;
    } else if (format.sampleFormat() == QAudioFormat::Int16) {
        spec.format = isBigEndian ? PA_SAMPLE_S16BE : PA_SAMPLE_S16LE;
    } else if (format.sampleFormat() == QAudioFormat::Int32) {
        spec.format = isBigEndian ? PA_SAMPLE_S32BE : PA_SAMPLE_S32LE;
    } else if (format.sampleFormat() == QAudioFormat::Float) {
        spec.format = isBigEndian ? PA_SAMPLE_FLOAT32BE : PA_SAMPLE_FLOAT32LE;
    }

    return spec;
}

pa_channel_map channelMapForAudioFormat(const QAudioFormat &format)
{
    pa_channel_map map;
    map.channels = 0;

    auto config = format.channelConfig();
    if (config == QAudioFormat::ChannelConfigUnknown)
        config = QAudioFormat::defaultChannelConfigForChannelCount(format.channelCount());

    if (config == QAudioFormat::ChannelConfigMono) {
        map.channels = 1;
        map.map[0] = PA_CHANNEL_POSITION_MONO;
    } else {
        if (config & QAudioFormat::channelConfig(QAudioFormat::FrontLeft))
            map.map[map.channels++] = PA_CHANNEL_POSITION_FRONT_LEFT;
        if (config & QAudioFormat::channelConfig(QAudioFormat::FrontRight))
            map.map[map.channels++] = PA_CHANNEL_POSITION_FRONT_RIGHT;
        if (config & QAudioFormat::channelConfig(QAudioFormat::FrontCenter))
            map.map[map.channels++] = PA_CHANNEL_POSITION_FRONT_CENTER;
        if (config & QAudioFormat::channelConfig(QAudioFormat::LFE))
            map.map[map.channels++] = PA_CHANNEL_POSITION_LFE;
        if (config & QAudioFormat::channelConfig(QAudioFormat::BackLeft))
            map.map[map.channels++] = PA_CHANNEL_POSITION_REAR_LEFT;
        if (config & QAudioFormat::channelConfig(QAudioFormat::BackRight))
            map.map[map.channels++] = PA_CHANNEL_POSITION_REAR_RIGHT;
        if (config & QAudioFormat::channelConfig(QAudioFormat::FrontLeftOfCenter))
            map.map[map.channels++] = PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER;
        if (config & QAudioFormat::channelConfig(QAudioFormat::FrontRightOfCenter))
            map.map[map.channels++] = PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER;
        if (config & QAudioFormat::channelConfig(QAudioFormat::BackCenter))
            map.map[map.channels++] = PA_CHANNEL_POSITION_REAR_CENTER;
        if (config & QAudioFormat::channelConfig(QAudioFormat::LFE2))
            map.map[map.channels++] = PA_CHANNEL_POSITION_LFE;
        if (config & QAudioFormat::channelConfig(QAudioFormat::SideLeft))
            map.map[map.channels++] = PA_CHANNEL_POSITION_SIDE_LEFT;
        if (config & QAudioFormat::channelConfig(QAudioFormat::SideRight))
            map.map[map.channels++] = PA_CHANNEL_POSITION_SIDE_RIGHT;
        if (config & QAudioFormat::channelConfig(QAudioFormat::TopFrontLeft))
            map.map[map.channels++] = PA_CHANNEL_POSITION_TOP_FRONT_LEFT;
        if (config & QAudioFormat::channelConfig(QAudioFormat::TopFrontRight))
            map.map[map.channels++] = PA_CHANNEL_POSITION_TOP_FRONT_RIGHT;
        if (config & QAudioFormat::channelConfig(QAudioFormat::TopFrontCenter))
            map.map[map.channels++] = PA_CHANNEL_POSITION_TOP_FRONT_CENTER;
        if (config & QAudioFormat::channelConfig(QAudioFormat::TopCenter))
            map.map[map.channels++] = PA_CHANNEL_POSITION_TOP_CENTER;
        if (config & QAudioFormat::channelConfig(QAudioFormat::TopBackLeft))
            map.map[map.channels++] = PA_CHANNEL_POSITION_TOP_REAR_LEFT;
        if (config & QAudioFormat::channelConfig(QAudioFormat::TopBackRight))
            map.map[map.channels++] = PA_CHANNEL_POSITION_TOP_REAR_RIGHT;
        if (config & QAudioFormat::channelConfig(QAudioFormat::TopSideLeft))
            map.map[map.channels++] = PA_CHANNEL_POSITION_AUX0;
        if (config & QAudioFormat::channelConfig(QAudioFormat::TopSideRight))
            map.map[map.channels++] = PA_CHANNEL_POSITION_AUX1;
        if (config & QAudioFormat::channelConfig(QAudioFormat::TopBackCenter))
            map.map[map.channels++] = PA_CHANNEL_POSITION_TOP_REAR_CENTER;
        if (config & QAudioFormat::channelConfig(QAudioFormat::BottomFrontCenter))
            map.map[map.channels++] = PA_CHANNEL_POSITION_AUX2;
        if (config & QAudioFormat::channelConfig(QAudioFormat::BottomFrontLeft))
            map.map[map.channels++] = PA_CHANNEL_POSITION_AUX3;
        if (config & QAudioFormat::channelConfig(QAudioFormat::BottomFrontRight))
            map.map[map.channels++] = PA_CHANNEL_POSITION_AUX4;
    }

    Q_ASSERT(qPopulationCount(config) == map.channels);
    return map;
}

QAudioFormat::ChannelConfig channelConfigFromMap(const pa_channel_map &map)
{
    quint32 config = 0;
    for (int i = 0; i < map.channels; ++i) {
        switch (map.map[i]) {
        case PA_CHANNEL_POSITION_MONO:
        case PA_CHANNEL_POSITION_FRONT_CENTER:
            config |= QAudioFormat::channelConfig(QAudioFormat::FrontCenter);
            break;
        case PA_CHANNEL_POSITION_FRONT_LEFT:
            config |= QAudioFormat::channelConfig(QAudioFormat::FrontLeft);
            break;
        case PA_CHANNEL_POSITION_FRONT_RIGHT:
            config |= QAudioFormat::channelConfig(QAudioFormat::FrontRight);
            break;
        case PA_CHANNEL_POSITION_REAR_CENTER:
            config |= QAudioFormat::channelConfig(QAudioFormat::BackCenter);
            break;
        case PA_CHANNEL_POSITION_REAR_LEFT:
            config |= QAudioFormat::channelConfig(QAudioFormat::BackLeft);
            break;
        case PA_CHANNEL_POSITION_REAR_RIGHT:
            config |= QAudioFormat::channelConfig(QAudioFormat::BackRight);
            break;
        case PA_CHANNEL_POSITION_LFE:
            config |= QAudioFormat::channelConfig(QAudioFormat::LFE);
            break;
        case PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER:
            config |= QAudioFormat::channelConfig(QAudioFormat::FrontLeftOfCenter);
            break;
        case PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER:
            config |= QAudioFormat::channelConfig(QAudioFormat::FrontRightOfCenter);
            break;
        case PA_CHANNEL_POSITION_SIDE_LEFT:
            config |= QAudioFormat::channelConfig(QAudioFormat::SideLeft);
            break;
        case PA_CHANNEL_POSITION_SIDE_RIGHT:
            config |= QAudioFormat::channelConfig(QAudioFormat::SideRight);
            break;

        case PA_CHANNEL_POSITION_TOP_CENTER:
            config |= QAudioFormat::channelConfig(QAudioFormat::TopCenter);
            break;
        case PA_CHANNEL_POSITION_TOP_FRONT_LEFT:
            config |= QAudioFormat::channelConfig(QAudioFormat::TopFrontLeft);
            break;
        case PA_CHANNEL_POSITION_TOP_FRONT_RIGHT:
            config |= QAudioFormat::channelConfig(QAudioFormat::TopFrontRight);
            break;
        case PA_CHANNEL_POSITION_TOP_FRONT_CENTER:
            config |= QAudioFormat::channelConfig(QAudioFormat::TopFrontCenter);
            break;
        case PA_CHANNEL_POSITION_TOP_REAR_LEFT:
            config |= QAudioFormat::channelConfig(QAudioFormat::TopBackLeft);
            break;
        case PA_CHANNEL_POSITION_TOP_REAR_RIGHT:
            config |= QAudioFormat::channelConfig(QAudioFormat::TopBackRight);
            break;
        case PA_CHANNEL_POSITION_TOP_REAR_CENTER:
            config |= QAudioFormat::channelConfig(QAudioFormat::TopBackCenter);
            break;
        default:
            break;
        }
    }
    return QAudioFormat::ChannelConfig(config);
}

QAudioFormat sampleSpecToAudioFormat(const pa_sample_spec &spec)
{
    QAudioFormat format;

    format.setSampleRate(spec.rate);
    format.setChannelCount(spec.channels);
    QAudioFormat::SampleFormat sampleFormat;
    switch (spec.format) {
    case PA_SAMPLE_U8:
        sampleFormat = QAudioFormat::UInt8;
        break;
    case PA_SAMPLE_S16LE:
    case PA_SAMPLE_S16BE:
        sampleFormat = QAudioFormat::Int16;
        break;
    case PA_SAMPLE_FLOAT32LE:
    case PA_SAMPLE_FLOAT32BE:
        sampleFormat = QAudioFormat::Float;
        break;
    case PA_SAMPLE_S32LE:
    case PA_SAMPLE_S32BE:
        sampleFormat = QAudioFormat::Int32;
        break;
    default:
        return {};
    }

    format.setSampleFormat(sampleFormat);
    return format;
}

QUtf8StringView currentError(const pa_context *context)
{
    return pa_strerror(pa_context_errno(context));
}

QUtf8StringView currentError(const pa_stream *stream)
{
    return currentError(pa_stream_get_context(stream));
}

} // namespace QPulseAudioInternal

static QLatin1StringView stateToQStringView(pa_stream_state_t state)
{
    using namespace Qt::StringLiterals;
    switch (state)
    {
    case PA_STREAM_UNCONNECTED: return "Unconnected"_L1;
    case PA_STREAM_CREATING:    return "Creating"_L1;
    case PA_STREAM_READY:       return "Ready"_L1;
    case PA_STREAM_FAILED:      return "Failed"_L1;
    case PA_STREAM_TERMINATED:  return "Terminated"_L1;
    default:                    Q_UNREACHABLE_RETURN("Unknown stream state"_L1);
    }
}

static QLatin1StringView sampleFormatToQStringView(pa_sample_format format)
{
    using namespace Qt::StringLiterals;
    switch (format)
    {
    case PA_SAMPLE_U8:          return "Unsigned 8 Bit PCM."_L1;
    case PA_SAMPLE_ALAW:        return "8 Bit a-Law "_L1;
    case PA_SAMPLE_ULAW:        return "8 Bit mu-Law"_L1;
    case PA_SAMPLE_S16LE:       return "Signed 16 Bit PCM, little endian (PC)."_L1;
    case PA_SAMPLE_S16BE:       return "Signed 16 Bit PCM, big endian."_L1;
    case PA_SAMPLE_FLOAT32LE:   return "32 Bit IEEE floating point, little endian (PC), range -1.0 to 1.0"_L1;
    case PA_SAMPLE_FLOAT32BE:   return "32 Bit IEEE floating point, big endian, range -1.0 to 1.0"_L1;
    case PA_SAMPLE_S32LE:       return "Signed 32 Bit PCM, little endian (PC)."_L1;
    case PA_SAMPLE_S32BE:       return "Signed 32 Bit PCM, big endian."_L1;
    case PA_SAMPLE_S24LE:       return "Signed 24 Bit PCM packed, little endian (PC)."_L1;
    case PA_SAMPLE_S24BE:       return "Signed 24 Bit PCM packed, big endian."_L1;
    case PA_SAMPLE_S24_32LE:    return "Signed 24 Bit PCM in LSB of 32 Bit words, little endian (PC)."_L1;
    case PA_SAMPLE_S24_32BE:    return "Signed 24 Bit PCM in LSB of 32 Bit words, big endian."_L1;
    case PA_SAMPLE_MAX:         return "Upper limit of valid sample types."_L1;
    case PA_SAMPLE_INVALID:     return "Invalid sample format"_L1;
    default:                    Q_UNREACHABLE_RETURN("Unknown sample format"_L1);
    }
}

static QLatin1StringView stateToQStringView(pa_context_state_t state)
{
    using namespace Qt::StringLiterals;
    switch (state)
    {
    case PA_CONTEXT_UNCONNECTED:  return "Unconnected"_L1;
    case PA_CONTEXT_CONNECTING:   return "Connecting"_L1;
    case PA_CONTEXT_AUTHORIZING:  return "Authorizing"_L1;
    case PA_CONTEXT_SETTING_NAME: return "Setting Name"_L1;
    case PA_CONTEXT_READY:        return "Ready"_L1;
    case PA_CONTEXT_FAILED:       return "Failed"_L1;
    case PA_CONTEXT_TERMINATED:   return "Terminated"_L1;
    default:                      Q_UNREACHABLE_RETURN("Unknown context state"_L1);
    }
}


QDebug operator<<(QDebug dbg, pa_stream_state_t state)
{
    return dbg << stateToQStringView(state);
}

QDebug operator<<(QDebug dbg, pa_sample_format format)
{
    return dbg << sampleFormatToQStringView(format);
}

QDebug operator<<(QDebug dbg, pa_context_state_t state)
{
    return dbg << stateToQStringView(state);
}

QT_END_NAMESPACE
