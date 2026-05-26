// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosaudiodevices_p.h"

#include "qohosaudiodevice_p.h"
#include "qohosaudiosink_p.h"
#include "qohosaudiosource_p.h"

#include <private/qaudiodevice_p.h>

#include <QtCore/qloggingcategory.h>

#include <ohaudio/native_audio_device_base.h>
#include <ohaudio/native_audio_manager.h>
#include <ohaudio/native_audio_routing_manager.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace {

Q_STATIC_LOGGING_CATEGORY(qLcOhosAudioDevices, "qt.multimedia.ohos.audiodevices")

QAudioFormat preferredDeviceFormat(OH_AudioDeviceDescriptor *descriptor)
{
    QAudioFormat format;

    uint32_t *sampleRates = nullptr;
    uint32_t sampleRateCount = 0;
    if (OH_AudioDeviceDescriptor_GetDeviceSampleRates(descriptor, &sampleRates, &sampleRateCount)
                == AUDIOCOMMON_RESULT_SUCCESS
        && sampleRateCount > 0) {
        uint32_t chosen = sampleRates[0];
        for (uint32_t i = 0; i < sampleRateCount; ++i) {
            if (sampleRates[i] == 48000) {
                chosen = 48000;
                break;
            }
            chosen = std::max(chosen, sampleRates[i]);
        }
        format.setSampleRate(static_cast<int>(chosen));
    } else {
        format.setSampleRate(48000);
    }

    uint32_t *channelCounts = nullptr;
    uint32_t channelCountSize = 0;
    if (OH_AudioDeviceDescriptor_GetDeviceChannelCounts(descriptor, &channelCounts,
                                                        &channelCountSize)
                == AUDIOCOMMON_RESULT_SUCCESS
        && channelCountSize > 0) {
        uint32_t chosen = channelCounts[0];
        for (uint32_t i = 0; i < channelCountSize; ++i) {
            if (channelCounts[i] == 2) {
                chosen = 2;
                break;
            }
            chosen = std::max(chosen, channelCounts[i]);
        }
        format.setChannelConfig(
                QAudioFormat::defaultChannelConfigForChannelCount(static_cast<int>(chosen)));
    } else {
        format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    }

    format.setSampleFormat(QAudioFormat::Float);
    return format;
}

QString deviceDisplayDescription(OH_AudioDeviceDescriptor *descriptor)
{
    char *displayName = nullptr;
    if (OH_AudioDeviceDescriptor_GetDeviceDisplayName(descriptor, &displayName)
                == AUDIOCOMMON_RESULT_SUCCESS
        && displayName && *displayName) {
        return QString::fromUtf8(displayName);
    }
    char *name = nullptr;
    if (OH_AudioDeviceDescriptor_GetDeviceName(descriptor, &name) == AUDIOCOMMON_RESULT_SUCCESS
        && name) {
        return QString::fromUtf8(name);
    }
    OH_AudioDevice_Type type{ AUDIO_DEVICE_TYPE_INVALID };
    OH_AudioDeviceDescriptor_GetDeviceType(descriptor, &type);
    switch (type) {
    case AUDIO_DEVICE_TYPE_EARPIECE:
        return QStringLiteral("Earpiece");
    case AUDIO_DEVICE_TYPE_SPEAKER:
        return QStringLiteral("Speaker");
    case AUDIO_DEVICE_TYPE_WIRED_HEADSET:
        return QStringLiteral("Wired Headset");
    case AUDIO_DEVICE_TYPE_WIRED_HEADPHONES:
        return QStringLiteral("Wired Headphones");
    case AUDIO_DEVICE_TYPE_BLUETOOTH_SCO:
        return QStringLiteral("Bluetooth (SCO)");
    case AUDIO_DEVICE_TYPE_BLUETOOTH_A2DP:
        return QStringLiteral("Bluetooth (A2DP)");
    case AUDIO_DEVICE_TYPE_MIC:
        return QStringLiteral("Microphone");
    case AUDIO_DEVICE_TYPE_USB_HEADSET:
    case AUDIO_DEVICE_TYPE_USB_DEVICE:
        return QStringLiteral("USB Audio");
    default:
        break;
    }
    return QStringLiteral("Audio Device");
}

QList<QAudioDevice> enumerateDevices(QAudioDevice::Mode mode)
{
    OH_AudioManager *manager = nullptr;
    if (OH_GetAudioManager(&manager) != AUDIOCOMMON_RESULT_SUCCESS || !manager) {
        qCWarning(qLcOhosAudioDevices) << "OH_GetAudioManager failed";
        return {};
    }

    OH_AudioRoutingManager *routing = nullptr;
    if (OH_AudioManager_GetAudioRoutingManager(&routing) != AUDIOCOMMON_RESULT_SUCCESS || !routing) {
        qCWarning(qLcOhosAudioDevices) << "OH_AudioManager_GetAudioRoutingManager failed";
        return {};
    }

    const OH_AudioDevice_Flag flag = (mode == QAudioDevice::Input) ? AUDIO_DEVICE_FLAG_INPUT
                                                                   : AUDIO_DEVICE_FLAG_OUTPUT;
    OH_AudioDeviceDescriptorArray *descriptors = nullptr;
    if (OH_AudioRoutingManager_GetDevices(routing, flag, &descriptors)
                != AUDIOCOMMON_RESULT_SUCCESS
        || !descriptors) {
        qCWarning(qLcOhosAudioDevices) << "OH_AudioRoutingManager_GetDevices failed for"
                                       << (mode == QAudioDevice::Input ? "input" : "output");
        return {};
    }

    QList<QAudioDevice> devices;
    devices.reserve(descriptors->size);
    for (uint32_t i = 0; i < descriptors->size; ++i) {
        OH_AudioDeviceDescriptor *descriptor = descriptors->descriptors[i];
        if (!descriptor)
            continue;

        uint32_t deviceId = 0;
        OH_AudioDeviceDescriptor_GetDeviceId(descriptor, &deviceId);

        const QAudioFormat preferredFormat = preferredDeviceFormat(descriptor);
        const QString description = deviceDisplayDescription(descriptor);

        devices << QAudioDevicePrivate::createQAudioDevice(std::make_unique<QOhosAudioDevice>(
                QByteArray::number(deviceId), description, mode, preferredFormat,
                i == 0));
    }

    OH_AudioRoutingManager_ReleaseDevices(routing, descriptors);
    return devices;
}

} // namespace

QOhosAudioDevices::QOhosAudioDevices() : QPlatformAudioDevices() { }

QOhosAudioDevices::~QOhosAudioDevices() = default;

QList<QAudioDevice> QOhosAudioDevices::findAudioInputs() const
{
    return enumerateDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QOhosAudioDevices::findAudioOutputs() const
{
    return enumerateDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QOhosAudioDevices::createAudioSource(const QAudioDevice &device,
                                                           const QAudioFormat &format,
                                                           QObject *parent)
{
    return new QtOHAudio::QOhosAudioSource(device, format, parent);
}

QPlatformAudioSink *QOhosAudioDevices::createAudioSink(const QAudioDevice &device,
                                                       const QAudioFormat &format, QObject *parent)
{
    return new QtOHAudio::QOhosAudioSink(device, format, parent);
}

QT_END_NAMESPACE
