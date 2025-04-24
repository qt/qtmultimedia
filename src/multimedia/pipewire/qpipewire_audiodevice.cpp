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

} // namespace

QPipewireAudioDevicePrivate::QPipewireAudioDevicePrivate(const PwPropertyDict &nodeProperties,
                                                         const PwPropertyDict &deviceProperties,
                                                         const SpaObjectAudioFormat &formats,
                                                         QAudioDevice::Mode mode, bool isDefault)
    : QAudioDevicePrivate{
          inferDeviceId(nodeProperties),
          mode,
          QString::fromUtf8(getNodeDescription(nodeProperties).value_or("")),
      }
{
    static const QList allSampleFormats = {
        QAudioFormat::SampleFormat::UInt8,
        QAudioFormat::SampleFormat::Int16,
        QAudioFormat::SampleFormat::Int32,
        QAudioFormat::SampleFormat::Float,
    };

    supportedSampleFormats = allSampleFormats;
    this->isDefault = isDefault;

    if (auto path = getDeviceSysfsPath(deviceProperties))
        m_sysfsPath.assign(*path);

    if (auto nodeName = getNodeName(nodeProperties))
        m_nodeName.assign(*nodeName);

    minimumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.front();
    maximumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.back();

    std::visit([&](const auto &arg) {
        setPreferredSamplingRate(arg);
    }, formats.rates);

    std::visit([&](const auto &arg) {
        setPreferredSampleFormats(arg);
    }, formats.sampleTypes);

    minimumChannelCount = 1;
    maximumChannelCount = formats.channelCount;

    m_channelPositions = formats.channelPositions;
    if (channelPositionsEqual(m_channelPositions, channelPositionsMono)) {
        channelConfiguration = QAudioFormat::ChannelConfigMono;
    } else if (channelPositionsEqual(m_channelPositions, channelPositionsStereo)) {
        channelConfiguration = QAudioFormat::ChannelConfigStereo;
    } else if (channelPositionsEqual(m_channelPositions, channelPositions2Dot1)) {
        channelConfiguration = QAudioFormat::ChannelConfig2Dot1;
    } else if (channelPositionsEqual(m_channelPositions, channelPositions3Dot0)) {
        channelConfiguration = QAudioFormat::ChannelConfig3Dot0;
    } else if (channelPositionsEqual(m_channelPositions, channelPositions3Dot1)) {
        channelConfiguration = QAudioFormat::ChannelConfig3Dot1;
    } else if (channelPositionsEqual(m_channelPositions, channelPositions5Dot0)) {
        channelConfiguration = QAudioFormat::ChannelConfigSurround5Dot0;
    } else if (channelPositionsEqual(m_channelPositions, channelPositions5Dot1)) {
        channelConfiguration = QAudioFormat::ChannelConfigSurround5Dot1;
    } else if (channelPositionsEqual(m_channelPositions, channelPositions7Dot0)) {
        channelConfiguration = QAudioFormat::ChannelConfigSurround7Dot0;
    } else if (channelPositionsEqual(m_channelPositions, channelPositions7Dot1)) {
        channelConfiguration = QAudioFormat::ChannelConfigSurround7Dot1;
    } else {
        // now we need to guess
        channelConfiguration =
                QAudioFormat::defaultChannelConfigForChannelCount(formats.channelCount);
    }

    preferredFormat.setChannelCount(formats.channelCount);
    preferredFormat.setChannelConfig(channelConfiguration);
}

QPipewireAudioDevicePrivate::~QPipewireAudioDevicePrivate() = default;

void QPipewireAudioDevicePrivate::setPreferredSamplingRate(int arg)
{
    preferredFormat.setSampleRate(arg);
}

void QPipewireAudioDevicePrivate::setPreferredSamplingRate(QSpan<const int> arg)
{
    constexpr int defaultPipewireSamplingRate = 48000;

    preferredFormat.setSampleRate(
            QtMultimediaPrivate::findClosestSamplingRate(defaultPipewireSamplingRate, arg));
}

void QPipewireAudioDevicePrivate::setPreferredSamplingRate(const SpaRange<int> &arg)
{
    preferredFormat.setSampleRate(arg.defaultValue);
}

void QPipewireAudioDevicePrivate::setPreferredSampleFormats(spa_audio_format arg)
{
    QAudioFormat::SampleFormat fmt = toSampleFormat(arg);
    if (fmt == QAudioFormat::Unknown) {
        qWarning() << "No sample format supported found for device" << nodeName();
        return;
    }

    preferredFormat.setSampleFormat(fmt);
}

void QPipewireAudioDevicePrivate::setPreferredSampleFormats(const SpaEnum<spa_audio_format> &fmt)
{
    for (spa_audio_format f : fmt.values()) {
        auto qtFormat = toSampleFormat(f);
        if (qtFormat != QAudioFormat::Unknown)
            supportedSampleFormats.push_back(qtFormat);
    }

    QAudioFormat::SampleFormat sampleFormat = toSampleFormat(fmt.defaultValue());
    if (sampleFormat != QAudioFormat::Unknown) {
        preferredFormat.setSampleFormat(sampleFormat);
    } else {
        if (!supportedSampleFormats.empty())
            preferredFormat.setSampleFormat(supportedSampleFormats.front());
        else
            qWarning() << "No sample format supported found for device" << nodeName();
    }
}

} // namespace QtPipeWire

QT_END_NAMESPACE
