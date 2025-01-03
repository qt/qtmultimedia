// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinaudiodevice_p.h"
#include "qcoreaudioutils_p.h"
#include <private/qcore_mac_p.h>

#if defined(Q_OS_IOS)
#include "qcoreaudiosessionmanager_p.h"
#else
#include "qmacosaudiodatautils_p.h"
#endif

#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QSet>
#include <QIODevice>

#include <optional>

QT_BEGIN_NAMESPACE

[[nodiscard]] static QAudioFormat qDefaultPreferredFormat(
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

[[nodiscard]] static QAudioFormat::ChannelConfig qGetDefaultChannelLayout(QAudioDevice::Mode mode)
{
    return (mode == QAudioDevice::Input) ? QAudioFormat::ChannelConfigMono : QAudioFormat::ChannelConfigStereo;
}

[[nodiscard]] static QString qGetDefaultDescription(const QByteArray &id)
{
    return QString::fromUtf8(id);
}

#ifdef Q_OS_MACOS

[[nodiscard]] static std::optional<QAudioFormat> qGetPreferredFormatForCoreAudioDevice(
    QAudioDevice::Mode mode,
    AudioDeviceID deviceId)
{
    const auto audioDevicePropertyStreamsAddress =
        makePropertyAddress(kAudioDevicePropertyStreams, mode);

    if (auto streamIDs = getAudioData<AudioStreamID>(deviceId, audioDevicePropertyStreamsAddress)) {
        const auto audioDevicePhysicalFormatPropertyAddress =
            makePropertyAddress(kAudioStreamPropertyPhysicalFormat, mode);

        for (auto streamID : *streamIDs) {
            if (auto streamDescription = getAudioObject<AudioStreamBasicDescription>(
                        streamID, audioDevicePhysicalFormatPropertyAddress)) {
                return QCoreAudioUtils::toQAudioFormat(*streamDescription);
            }
        }
    }

    return std::nullopt;
}

[[nodiscard]] static std::optional<QAudioFormat::ChannelConfig> qGetChannelLayoutForCoreAudioDevice(
    QAudioDevice::Mode mode,
    AudioDeviceID deviceId)
{
    const auto propertyAddress =
        makePropertyAddress(kAudioDevicePropertyPreferredChannelLayout, mode);
    if (auto data = getAudioData<char>(deviceId, propertyAddress, sizeof(AudioChannelLayout))) {
        const auto *layout = reinterpret_cast<const AudioChannelLayout *>(data->data());
        return QCoreAudioUtils::fromAudioChannelLayout(layout);
    }
    return std::nullopt;
}

[[nodiscard]] static std::optional<QString> qGetDescriptionForCoreAudioDevice(
    QAudioDevice::Mode mode,
    AudioDeviceID deviceId)
{
    const auto propertyAddress = makePropertyAddress(kAudioObjectPropertyName, mode);
    if (auto name = getAudioObject<QCFString>(deviceId, propertyAddress))
        return name;

    return std::nullopt;
}

QCoreAudioDeviceInfo::QCoreAudioDeviceInfo(AudioDeviceID id, const QByteArray &device, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode),
      m_deviceId(id)
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

    const std::optional<QString> descriptionOpt = qGetDescriptionForCoreAudioDevice(mode, id);
    if (descriptionOpt.has_value())
        description = descriptionOpt.value();
    else
        description = qGetDefaultDescription(device);

    // TODO: There is some duplicate code here with the iOS constructor
    minimumSampleRate = 1;
    maximumSampleRate = 96000;
    minimumChannelCount = 1;
    maximumChannelCount = 16;
    supportedSampleFormats << QAudioFormat::UInt8 << QAudioFormat::Int16 << QAudioFormat::Int32 << QAudioFormat::Float;
}

#else

QCoreAudioDeviceInfo::QCoreAudioDeviceInfo(const QByteArray &device, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode)
{
    channelConfiguration = qGetDefaultChannelLayout(mode);
    preferredFormat = qDefaultPreferredFormat(mode, channelConfiguration);
    description = qGetDefaultDescription(device);

    minimumSampleRate = 1;
    maximumSampleRate = 96000;
    minimumChannelCount = 1;
    maximumChannelCount = 16;
    supportedSampleFormats << QAudioFormat::UInt8 << QAudioFormat::Int16 << QAudioFormat::Int32 << QAudioFormat::Float;
}

#endif

QT_END_NAMESPACE
