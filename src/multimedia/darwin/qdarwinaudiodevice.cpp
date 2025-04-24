// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinaudiodevice_p.h"

#include <QtCore/private/qcore_mac_p.h>

#include <QtMultimedia/private/qcoreaudioutils_p.h>
#include <QtMultimedia/private/qaudioformat_p.h>
#ifdef Q_OS_MACOS
#include <QtMultimedia/private/qmacosaudiodatautils_p.h>
#endif

#include <optional>

QT_BEGIN_NAMESPACE

namespace {

[[nodiscard]] QAudioFormat qDefaultPreferredFormat(
    QAudioDevice::Mode mode,
    QAudioFormat::ChannelConfig channelConfig)
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setSampleFormat(QAudioFormat::Int16);
    format.setChannelCount(mode == QAudioDevice::Input ? 1 : 2);
    format.setChannelConfig(channelConfig);
    return format;
}

[[nodiscard]] QAudioFormat::ChannelConfig qGetDefaultChannelLayout(QAudioDevice::Mode mode)
{
    return (mode == QAudioDevice::Input) ? QAudioFormat::ChannelConfigMono : QAudioFormat::ChannelConfigStereo;
}

[[nodiscard]] QString qGetDefaultDescription(const QByteArray &id)
{
    return QString::fromUtf8(id);
}

#ifdef Q_OS_MACOS

[[nodiscard]] std::optional<QAudioFormat> qGetPreferredFormatForCoreAudioDevice(
    QAudioDevice::Mode mode,
    AudioDeviceID deviceId)
{
    using namespace QCoreAudioUtils;

    const auto audioDevicePropertyStreamsAddress =
        makePropertyAddress(kAudioDevicePropertyStreams, mode);

    if (auto streamIDs = getAudioPropertyList<AudioStreamID>(deviceId, audioDevicePropertyStreamsAddress)) {
        const auto audioDevicePhysicalFormatPropertyAddress =
            makePropertyAddress(kAudioStreamPropertyPhysicalFormat, mode);

        for (auto streamID : *streamIDs) {
            if (auto streamDescription = getAudioProperty<AudioStreamBasicDescription>(
                        streamID, audioDevicePhysicalFormatPropertyAddress)) {
                return QCoreAudioUtils::toQAudioFormat(*streamDescription);
            }
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<QAudioFormat::ChannelConfig> qGetChannelLayoutForCoreAudioDevice(
    QAudioDevice::Mode mode,
    AudioDeviceID deviceId)
{
    using namespace QCoreAudioUtils;

    const auto propertyAddress =
        makePropertyAddress(kAudioDevicePropertyPreferredChannelLayout, mode);

    if (auto layout = getAudioPropertyWithFlexibleArrayMember<AudioChannelLayout>(deviceId, propertyAddress))
        return QCoreAudioUtils::fromAudioChannelLayout(layout.get());

    return std::nullopt;
}

[[nodiscard]] std::optional<QString> qGetDescriptionForCoreAudioDevice(
    QAudioDevice::Mode mode,
    AudioDeviceID deviceId)
{
    using namespace QCoreAudioUtils;

    const auto propertyAddress = makePropertyAddress(kAudioObjectPropertyName, mode);
    if (auto name = getAudioProperty<QCFString>(deviceId, propertyAddress))
        return name;

    return std::nullopt;
}

[[nodiscard]] std::optional<int> qSupportedNumberOfChannels(
        QAudioDevice::Mode mode,
        AudioDeviceID deviceId)
{
    using namespace QCoreAudioUtils;

    const auto audioDevicePropertyStreamsAddress =
            makePropertyAddress(kAudioDevicePropertyStreams, mode);

    auto streamIDs = getAudioPropertyList<AudioStreamID>(deviceId, audioDevicePropertyStreamsAddress);
    if (!streamIDs)
        return std::nullopt;

    const auto propVirtualFormat = makePropertyAddress(kAudioStreamPropertyVirtualFormat, mode);

    int ret{};

    for (auto streamID : *streamIDs) {
        auto streamDescription = getAudioProperty<AudioStreamBasicDescription>(streamID, propVirtualFormat);
        if (!streamDescription)
            continue;
        ret += streamDescription->mChannelsPerFrame;
    }

    return ret;
}

#endif

} // namespace

#ifdef Q_OS_MACOS

static QString getDescription(AudioDeviceID id, const QByteArray &device, QAudioDevice::Mode mode)
{
    if (auto optionalDescription = qGetDescriptionForCoreAudioDevice(mode, id))
        return *optionalDescription;
    return qGetDefaultDescription(device);
}

QCoreAudioDeviceInfo::QCoreAudioDeviceInfo(AudioDeviceID id, const QByteArray &device, QAudioDevice::Mode mode):
    QAudioDevicePrivate{
        device,
        mode,
        getDescription(id, device, mode),
    }
{
    const std::optional<QAudioFormat::ChannelConfig> channelConfigOpt =
        qGetChannelLayoutForCoreAudioDevice(mode, id);
    if (channelConfigOpt.has_value())
        channelConfiguration = channelConfigOpt.value();
    else
        channelConfiguration = qGetDefaultChannelLayout(mode);

    const std::optional<QAudioFormat> preferredFormatOpt =
        qGetPreferredFormatForCoreAudioDevice(mode, id);
    if (preferredFormatOpt.has_value())
        preferredFormat = preferredFormatOpt.value();
    else
        preferredFormat = qDefaultPreferredFormat(mode, channelConfiguration);

    minimumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.front();
    maximumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.back();
    minimumChannelCount = 1;
    maximumChannelCount = qSupportedNumberOfChannels(mode, id).value_or(16);

    supportedSampleFormats = qAllSupportedSampleFormats();
}

#else

QCoreAudioDeviceInfo::QCoreAudioDeviceInfo(const QByteArray &device, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode, qGetDefaultDescription(device))
{
    channelConfiguration = qGetDefaultChannelLayout(mode);
    preferredFormat = qDefaultPreferredFormat(mode, channelConfiguration);

    minimumSampleRate = 1;
    maximumSampleRate = 96000;
    minimumChannelCount = 1;
    maximumChannelCount = 16;
    supportedSampleFormats = qAllSupportedSampleFormats();
}

#endif

QT_END_NAMESPACE
