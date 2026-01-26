// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiodevice_p.h"

#include <QtCore/qdebug.h>
#include <QtMultimedia/private/qaudioformat_p.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

namespace {

QAudioFormat::SampleFormat toSampleFormat(spa_audio_format fmt)
{
    switch (fmt) {
    case SPA_AUDIO_FORMAT_S16:
        return QAudioFormat::Int16;
    case SPA_AUDIO_FORMAT_S32:
        return QAudioFormat::Int32;
    case SPA_AUDIO_FORMAT_U8:
        return QAudioFormat::UInt8;
    case SPA_AUDIO_FORMAT_F32:
        return QAudioFormat::Float;
    default:
        return QAudioFormat::Unknown;
    }
}

QByteArray inferDeviceId(const PwPropertyDict &properties)
{
    auto nodeName = getNodeName(properties);
    Q_ASSERT(nodeName);
    if (nodeName)
        return QByteArray{ *nodeName };
    return {};
}

template <typename Lhs, typename Rhs>
bool channelPositionsEqual(const Lhs &lhs, const Rhs &rhs)
{
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

QAudioDevicePrivate::AudioDeviceFormat toAudioDeviceFormat(const SpaObjectAudioFormat &formats)
{
    QAudioDevicePrivate::AudioDeviceFormat format;

    static const QList allSampleFormats = {
        QAudioFormat::SampleFormat::UInt8,
        QAudioFormat::SampleFormat::Int16,
        QAudioFormat::SampleFormat::Int32,
        QAudioFormat::SampleFormat::Float,
    };

    format.supportedSampleFormats = allSampleFormats;

    format.minimumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.front();
    format.maximumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.back();

    // Set preferred sample rate
    constexpr int defaultPipewireSamplingRate = 48000;
    if (formats.rates) {
        std::visit([&](const auto &arg) {
            if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, int>) {
                format.preferredFormat.setSampleRate(arg);
            } else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, QSpan<const int>>) {
                format.preferredFormat.setSampleRate(QtMultimediaPrivate::findClosestSamplingRate(
                        defaultPipewireSamplingRate, arg));
            } else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, SpaRange<int>>) {
                format.preferredFormat.setSampleRate(arg.defaultValue);
            }
        }, *formats.rates);
    } else {
        format.preferredFormat.setSampleRate(defaultPipewireSamplingRate);
    }

    // Set preferred sample format
    std::visit([&](const auto &arg) {
        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, spa_audio_format>) {
            QAudioFormat::SampleFormat fmt = toSampleFormat(arg);
            if (fmt != QAudioFormat::Unknown) {
                format.preferredFormat.setSampleFormat(fmt);
            } else {
                // fallback to float
                format.preferredFormat.setSampleFormat(QAudioFormat::Float);
            }
        } else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, spa_audio_iec958_codec>) {
            Q_ASSERT(arg == SPA_AUDIO_IEC958_CODEC_PCM);
            format.preferredFormat.setSampleFormat(QAudioFormat::Float);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(arg)>,
                                            SpaEnum<spa_audio_format>>) {
            QAudioFormat::SampleFormat sampleFormat = toSampleFormat(arg.defaultValue());
            if (sampleFormat != QAudioFormat::Unknown) {
                format.preferredFormat.setSampleFormat(sampleFormat);
            } else {
                if (!format.supportedSampleFormats.empty())
                    format.preferredFormat.setSampleFormat(QAudioFormat::Float);
            }
        }
    }, formats.sampleTypes);

    format.minimumChannelCount = 1;
    format.maximumChannelCount = formats.channelCount;

    // Set channel configuration
    if (formats.channelPositions) {
        if (channelPositionsEqual(*formats.channelPositions, channelPositionsMono)) {
            format.channelConfiguration = QAudioFormat::ChannelConfigMono;
        } else if (channelPositionsEqual(*formats.channelPositions, channelPositionsStereo)) {
            format.channelConfiguration = QAudioFormat::ChannelConfigStereo;
        } else if (channelPositionsEqual(*formats.channelPositions, channelPositions2Dot1)) {
            format.channelConfiguration = QAudioFormat::ChannelConfig2Dot1;
        } else if (channelPositionsEqual(*formats.channelPositions, channelPositions3Dot0)) {
            format.channelConfiguration = QAudioFormat::ChannelConfig3Dot0;
        } else if (channelPositionsEqual(*formats.channelPositions, channelPositions3Dot1)) {
            format.channelConfiguration = QAudioFormat::ChannelConfig3Dot1;
        } else if (channelPositionsEqual(*formats.channelPositions, channelPositions5Dot0)) {
            format.channelConfiguration = QAudioFormat::ChannelConfigSurround5Dot0;
        } else if (channelPositionsEqual(*formats.channelPositions, channelPositions5Dot1)) {
            format.channelConfiguration = QAudioFormat::ChannelConfigSurround5Dot1;
        } else if (channelPositionsEqual(*formats.channelPositions, channelPositions7Dot0)) {
            format.channelConfiguration = QAudioFormat::ChannelConfigSurround7Dot0;
        } else if (channelPositionsEqual(*formats.channelPositions, channelPositions7Dot1)) {
            format.channelConfiguration = QAudioFormat::ChannelConfigSurround7Dot1;
        } else {
            format.channelConfiguration =
                    QAudioFormat::defaultChannelConfigForChannelCount(formats.channelCount);
        }
    } else {
        format.channelConfiguration =
                QAudioFormat::defaultChannelConfigForChannelCount(formats.channelCount);
    }

    format.preferredFormat.setChannelCount(formats.channelCount);
    format.preferredFormat.setChannelConfig(format.channelConfiguration);

    return format;
}

} // namespace

QPipewireAudioDevicePrivate::QPipewireAudioDevicePrivate(const PwPropertyDict &nodeProperties,
                                                         std::optional<QByteArray> sysfsPath,
                                                         const SpaObjectAudioFormat &formats,
                                                         QAudioDevice::Mode mode, bool isDefault)
    : QAudioDevicePrivate{
          inferDeviceId(nodeProperties),
          mode,
          QString::fromUtf8(getNodeDescription(nodeProperties).value_or("")),
          isDefault,
          toAudioDeviceFormat(formats),
      }
{
    if (sysfsPath)
        m_sysfsPath = std::move(sysfsPath);

    if (auto nodeName = getNodeName(nodeProperties))
        m_nodeName.assign(*nodeName);

    m_channelPositions = formats.channelPositions;
}
QPipewireAudioDevicePrivate::~QPipewireAudioDevicePrivate() = default;

} // namespace QtPipeWire

QT_END_NAMESPACE
