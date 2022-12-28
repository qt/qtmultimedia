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

QT_BEGIN_NAMESPACE

#if defined(Q_OS_MACOS)
QCoreAudioDeviceInfo::QCoreAudioDeviceInfo(AudioDeviceID id, const QByteArray &device, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode),
    m_deviceId(id)
#else
QCoreAudioDeviceInfo::QCoreAudioDeviceInfo(const QByteArray &device, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode)
#endif
{
    description = getDescription();
    getChannelLayout();
    preferredFormat = determinePreferredFormat();
    minimumSampleRate = 1;
    maximumSampleRate = 96000;
    minimumChannelCount = 1;
    maximumChannelCount = 16;
    supportedSampleFormats << QAudioFormat::UInt8 << QAudioFormat::Int16 << QAudioFormat::Int32 << QAudioFormat::Float;

}

QAudioFormat QCoreAudioDeviceInfo::determinePreferredFormat() const
{
    QAudioFormat format;

#if defined(Q_OS_MACOS)
    const auto audioDevicePropertyStreamsAddress =
            makePropertyAddress(kAudioDevicePropertyStreams, mode);

    if (auto streamIDs = getAudioData<AudioStreamID>(m_deviceId, audioDevicePropertyStreamsAddress,
                                                     "propertyStreams")) {
        const auto audioDevicePhysicalFormatPropertyAddress =
                makePropertyAddress(kAudioStreamPropertyPhysicalFormat, mode);

        for (auto streamID : *streamIDs) {
            if (auto streamDescription = getAudioObject<AudioStreamBasicDescription>(
                        streamID, audioDevicePhysicalFormatPropertyAddress,
                        "prefferedPhysicalFormat")) {
                format = CoreAudioUtils::toQAudioFormat(*streamDescription);
                break;
            }
        }
    }

    if (!format.isValid())
#endif
    {
        format.setSampleRate(44100);
        format.setSampleFormat(QAudioFormat::Int16);
        format.setChannelCount(mode == QAudioDevice::Input ? 1 : 2);
    }
    format.setChannelConfig(channelConfiguration);

    return format;
}


QString QCoreAudioDeviceInfo::getDescription() const
{
#ifdef Q_OS_MACOS
    const auto propertyAddress = makePropertyAddress(kAudioObjectPropertyName, mode);
    if (auto name =
                getAudioObject<CFStringRef>(m_deviceId, propertyAddress, "Device Description")) {
        auto deleter = qScopeGuard([&name]() { CFRelease(*name); });
        return QString::fromCFString(*name);
    }

    return {};
#else
    return QString::fromUtf8(id);
#endif
}

void QCoreAudioDeviceInfo::getChannelLayout()
{
#ifdef Q_OS_MACOS
    const auto propertyAddress =
            makePropertyAddress(kAudioDevicePropertyPreferredChannelLayout, mode);
    if (auto data = getAudioData<char>(m_deviceId, propertyAddress, "prefferedChannelLayout",
                                       sizeof(AudioChannelLayout))) {
        const auto *layout = reinterpret_cast<const AudioChannelLayout *>(data->data());
        channelConfiguration = CoreAudioUtils::fromAudioChannelLayout(layout);
    }
#else
    channelConfiguration = (mode == QAudioDevice::Input) ? QAudioFormat::ChannelConfigMono : QAudioFormat::ChannelConfigStereo;
#endif
}

QT_END_NAMESPACE
