// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosaudiodevices_p.h"

#include "qohosaudiodevice_p.h"
#include "qohosaudiosink_p.h"
#include "qohosaudiosource_p.h"

#include <private/qaudiodevice_p.h>
#include <private/qaudioformat_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qmutex.h>
#include <QtCore/qspan.h>

#include <ohaudio/native_audio_device_base.h>
#include <ohaudio/native_audio_manager.h>
#include <ohaudio/native_audio_routing_manager.h>
#include <ohaudio/native_audiostream_base.h>

#include <optional>

QT_BEGIN_NAMESPACE

namespace {

Q_STATIC_LOGGING_CATEGORY(qLcOhosAudioDevices, "qt.multimedia.ohos.audiodevices")

// OH_AudioRoutingManager_OnDeviceChangedCallback carries no user data, so the
// device-change callbacks reach the (single) QOhosAudioDevices instance through
// this pointer, set for the lifetime of the object.
Q_CONSTINIT QOhosAudioDevices *g_audioDevicesInstance = nullptr;

// Serializes access to g_audioDevicesInstance so a device-change callback
// arriving on an OHAudio thread cannot run against a half-destroyed instance.
Q_CONSTINIT QBasicMutex g_callbackMutex;

QAudioFormat preferredDeviceFormat(OH_AudioDeviceDescriptor *descriptor)
{
    namespace ranges = QtMultimediaPrivate::ranges;

    QAudioFormat format;

    // OHAudio reports INVALID_PARAM for the channel counts (and sometimes the
    // sample rates) of otherwise valid devices such as USB headsets, so fall
    // back per field to sensible defaults.
    constexpr int defaultSampleRate = 48000;
    uint32_t *sampleRates = nullptr;
    uint32_t sampleRateCount = 0;
    if (OH_AudioDeviceDescriptor_GetDeviceSampleRates(descriptor, &sampleRates, &sampleRateCount)
                == AUDIOCOMMON_RESULT_SUCCESS
        && sampleRateCount > 0) {
        format.setSampleRate(QtMultimediaPrivate::findClosestSamplingRate(
                defaultSampleRate, QSpan<const uint32_t>{ sampleRates, sampleRateCount }));
    } else {
        format.setSampleRate(defaultSampleRate);
    }

    uint32_t *channelCounts = nullptr;
    uint32_t channelCountSize = 0;
    if (OH_AudioDeviceDescriptor_GetDeviceChannelCounts(descriptor, &channelCounts, &channelCountSize)
                == AUDIOCOMMON_RESULT_SUCCESS
        && channelCountSize > 0) {
        const QSpan<const uint32_t> channels{ channelCounts, channelCountSize };
        const uint32_t chosenChannels =
                ranges::contains(channels, 2u) ? 2u : *ranges::max_element(channels);
        format.setChannelConfig(
                QAudioFormat::defaultChannelConfigForChannelCount(static_cast<int>(chosenChannels)));
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

std::optional<uint32_t> preferredDeviceId(OH_AudioRoutingManager *routing, QAudioDevice::Mode mode)
{
    OH_AudioDeviceDescriptorArray *preferred = nullptr;
    const OH_AudioCommon_Result result = (mode == QAudioDevice::Input)
            ? OH_AudioRoutingManager_GetPreferredInputDevice(routing, AUDIOSTREAM_SOURCE_TYPE_MIC,
                                                             &preferred)
            : OH_AudioRoutingManager_GetPreferredOutputDevice(routing, AUDIOSTREAM_USAGE_MUSIC,
                                                              &preferred);
    if (result != AUDIOCOMMON_RESULT_SUCCESS || !preferred || preferred->size == 0) {
        if (preferred)
            OH_AudioRoutingManager_ReleaseDevices(routing, preferred);
        return std::nullopt;
    }

    uint32_t deviceId = 0;
    OH_AudioDeviceDescriptor_GetDeviceId(preferred->descriptors[0], &deviceId);
    OH_AudioRoutingManager_ReleaseDevices(routing, preferred);
    return deviceId;
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

    const std::optional<uint32_t> defaultDeviceId = preferredDeviceId(routing, mode);

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
        const bool isDefault = defaultDeviceId ? deviceId == *defaultDeviceId : i == 0;

        devices << QAudioDevicePrivate::createQAudioDevice(std::make_unique<QOhosAudioDevice>(
                QByteArray::number(deviceId), description, mode, preferredFormat,
                isDefault));
    }

    OH_AudioRoutingManager_ReleaseDevices(routing, descriptors);
    return devices;
}

} // namespace

QOhosAudioDevices::QOhosAudioDevices() : QPlatformAudioDevices()
{
    Q_ASSERT(!g_audioDevicesInstance);
    g_audioDevicesInstance = this;
    registerDeviceChangeCallbacks();
}

QOhosAudioDevices::~QOhosAudioDevices()
{
    unregisterDeviceChangeCallbacks();
    QMutexLocker guard{ &g_callbackMutex };
    g_audioDevicesInstance = nullptr;
}

void QOhosAudioDevices::registerDeviceChangeCallbacks()
{
    OH_AudioRoutingManager *routing = nullptr;
    if (OH_AudioManager_GetAudioRoutingManager(&routing) != AUDIOCOMMON_RESULT_SUCCESS || !routing) {
        qCWarning(qLcOhosAudioDevices)
                << "Cannot register device change callbacks: routing manager unavailable";
        return;
    }

    const OH_AudioCommon_Result inputResult = OH_AudioRoutingManager_RegisterDeviceChangeCallback(
            routing, AUDIO_DEVICE_FLAG_INPUT, &QOhosAudioDevices::onInputDevicesChanged);
    const OH_AudioCommon_Result outputResult = OH_AudioRoutingManager_RegisterDeviceChangeCallback(
            routing, AUDIO_DEVICE_FLAG_OUTPUT, &QOhosAudioDevices::onOutputDevicesChanged);

    if (inputResult != AUDIOCOMMON_RESULT_SUCCESS || outputResult != AUDIOCOMMON_RESULT_SUCCESS)
        qCWarning(qLcOhosAudioDevices) << "Failed to register audio device change callbacks";

    m_deviceChangeCallbacksRegistered =
            inputResult == AUDIOCOMMON_RESULT_SUCCESS || outputResult == AUDIOCOMMON_RESULT_SUCCESS;
}

void QOhosAudioDevices::unregisterDeviceChangeCallbacks()
{
    if (!m_deviceChangeCallbacksRegistered)
        return;

    OH_AudioRoutingManager *routing = nullptr;
    if (OH_AudioManager_GetAudioRoutingManager(&routing) != AUDIOCOMMON_RESULT_SUCCESS || !routing)
        return;

    OH_AudioRoutingManager_UnregisterDeviceChangeCallback(
            routing, &QOhosAudioDevices::onInputDevicesChanged);
    OH_AudioRoutingManager_UnregisterDeviceChangeCallback(
            routing, &QOhosAudioDevices::onOutputDevicesChanged);
}

// Invoked on an OHAudio service thread. We ignore the supplied descriptor array
// (owned by the framework) and just invalidate the cache so the next query
// re-enumerates.
int32_t QOhosAudioDevices::onInputDevicesChanged(OH_AudioDevice_ChangeType /*type*/,
                                                 OH_AudioDeviceDescriptorArray * /*devices*/)
{
    QMutexLocker guard{ &g_callbackMutex };
    if (g_audioDevicesInstance)
        g_audioDevicesInstance->onAudioInputsChanged();
    return 0;
}

int32_t QOhosAudioDevices::onOutputDevicesChanged(OH_AudioDevice_ChangeType /*type*/,
                                                  OH_AudioDeviceDescriptorArray * /*devices*/)
{
    QMutexLocker guard{ &g_callbackMutex };
    if (g_audioDevicesInstance)
        g_audioDevicesInstance->onAudioOutputsChanged();
    return 0;
}

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
