// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiodevice_p.h"

#include <QtCore/qdebug.h>

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
    : QAudioDevicePrivate(inferDeviceId(nodeProperties), mode)
{
    this->isDefault = isDefault;

    if (auto path = getDeviceSysfsPath(deviceProperties))
        m_sysfsPath.assign(*path);

    if (auto name = getDeviceName(deviceProperties))
        m_deviceName.assign(*name);

    if (auto deviceDescription = getDeviceDescription(deviceProperties))
        description = QString::fromUtf8(*deviceDescription);

    std::visit([&](const auto &arg) {
        setSamplingRates(arg);
    }, formats.rates);

    std::visit([&](const auto &arg) {
        setSampleFormats(arg);
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

void QPipewireAudioDevicePrivate::setSamplingRates(int arg)
{
    minimumSampleRate = arg;
    maximumSampleRate = arg;
    preferredFormat.setSampleRate(arg);
}

void QPipewireAudioDevicePrivate::setSamplingRates(QSpan<const int> arg)
{
    auto [minIt, maxIt] = std::minmax_element(arg.begin(), arg.end());

    minimumSampleRate = *minIt;
    maximumSampleRate = *maxIt;

    if (std::find(arg.begin(), arg.end(), 44100) != arg.end()) {
        preferredFormat.setSampleRate(44100);
    } else {
        // find supported rate, which is closest to 44100 (using a logarithmic scaling)
        auto ratioTo44100 = [](int arg) {
            return arg > 44100 ? float(arg) / 44100.f : 44100.f / float(arg);
        };

        std::vector<int> rates{ arg.begin(), arg.end() };
        std::sort(rates.begin(), rates.end(), [&](int lhs, int rhs) {
            return ratioTo44100(lhs) < ratioTo44100(rhs);
        });

        preferredFormat.setSampleRate(rates.front());
    }
}

void QPipewireAudioDevicePrivate::setSamplingRates(const SpaRange<int> &arg)
{
    minimumSampleRate = arg.minValue;
    maximumSampleRate = arg.maxValue;

    preferredFormat.setSampleRate(arg.defaultValue);
}

void QPipewireAudioDevicePrivate::setSampleFormats(spa_audio_format arg)
{
    QAudioFormat::SampleFormat fmt = toSampleFormat(arg);
    if (fmt == QAudioFormat::Unknown) {
        qWarning() << "No sample format supported found for device" << deviceName();
        return;
    }

    supportedSampleFormats = { fmt };
    preferredFormat.setSampleFormat(fmt);
}

void QPipewireAudioDevicePrivate::setSampleFormats(const SpaEnum<spa_audio_format> &fmt)
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
            qWarning() << "No sample format supported found for device" << deviceName();
    }
}

} // namespace QtPipeWire

QT_END_NAMESPACE
