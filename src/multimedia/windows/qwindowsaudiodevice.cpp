// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// INTERNAL USE ONLY: Do NOT use for any other purpose.
//

#include "qwindowsaudiodevice_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qt_windows.h>
#include <QtCore/private/qsystemerror_p.h>

#include <QtMultimedia/private/qaudioformat_p.h>
#include <QtMultimedia/private/qcomtaskresource_p.h>
#include <QtMultimedia/private/qwindowsaudioutils_p.h>
#include <QtMultimedia/private/qcomtaskresource_p.h>

#include <audioclient.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <propkeydef.h>

#include <set>

QT_BEGIN_NAMESPACE

namespace {

std::optional<EndpointFormFactor> inferFormFactor(const ComPtr<IPropertyStore> &propertyStore)
{
    PROPVARIANT var;
    PropVariantInit(&var);
    HRESULT hr = propertyStore->GetValue(PKEY_AudioEndpoint_FormFactor, &var);
    if (SUCCEEDED(hr) && var.uintVal != EndpointFormFactor::UnknownFormFactor)
        return EndpointFormFactor(var.uintVal);

    return std::nullopt;
}

std::optional<QAudioFormat::ChannelConfig>
inferChannelConfiguration(const ComPtr<IPropertyStore> &propertyStore, int maximumChannelCount)
{
    PROPVARIANT var;
    PropVariantInit(&var);
    HRESULT hr = propertyStore->GetValue(PKEY_AudioEndpoint_PhysicalSpeakers, &var);
    if (SUCCEEDED(hr) && var.uintVal != 0)
        return QWindowsAudioUtils::maskToChannelConfig(var.uintVal, maximumChannelCount);

    return std::nullopt;
}

int maxChannelCountForFormFactor(EndpointFormFactor formFactor)
{
    switch (formFactor) {
    case EndpointFormFactor::Headphones:
    case EndpointFormFactor::Headset:
        return 2;
    case EndpointFormFactor::SPDIF:
        return 6; // SPDIF can have 2 channels of uncompressed or 6 channels of compressed audio

    case EndpointFormFactor::DigitalAudioDisplayDevice:
        return 8; // HDMI can have max 8 channels

    default:
        return 128;
    }
}

struct FormatProbeResult
{
    void update(const QAudioFormat &fmt)
    {
        supportedSampleFormats.insert(fmt.sampleFormat());
        updateChannelCount(fmt.channelCount());
        updateSamplingRate(fmt.sampleRate());
    }

    void updateChannelCount(int channelCount)
    {
        if (channelCount < channelCountRange.first)
            channelCountRange.first = channelCount;
        if (channelCount > channelCountRange.second)
            channelCountRange.second = channelCount;
    }

    void updateSamplingRate(int samplingRate)
    {
        if (samplingRate < sampleRateRange.first)
            sampleRateRange.first = samplingRate;
        if (samplingRate > sampleRateRange.second)
            sampleRateRange.second = samplingRate;
    }

    std::set<QAudioFormat::SampleFormat> supportedSampleFormats;
    std::pair<int, int> channelCountRange{ std::numeric_limits<int>::max(), 0 };
    std::pair<int, int> sampleRateRange{ std::numeric_limits<int>::max(), 0 };
};

std::optional<int> inferMaxNumberOfChannelsForSampleRate(const ComPtr<IAudioClient> &audioClient,
                                                         QAudioFormat::SampleFormat sampleFormat,
                                                         int sampleRate,
                                                         int maxChannelsForFormFactor)
{
    using namespace QWindowsAudioUtils;

    QAudioFormat fmt;
    fmt.setSampleFormat(sampleFormat);
    fmt.setSampleRate(sampleRate);
    fmt.setChannelCount(maxChannelsForFormFactor);

    std::optional<WAVEFORMATEXTENSIBLE> formatEx = toWaveFormatExtensible(fmt);
    if (!formatEx)
        return std::nullopt;

    QComTaskResource<WAVEFORMATEX> closestMatch;
    HRESULT result = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &formatEx->Format,
                                                    closestMatch.address());

    if (FAILED(result))
        return std::nullopt;

    if (closestMatch) {
        QAudioFormat closestMatchFormat = waveFormatExToFormat(*closestMatch);
        if (closestMatchFormat.sampleRate() != sampleRate)
            return std::nullopt; // sample rate not supported at all

        if (closestMatchFormat.sampleFormat() != sampleFormat)
            return std::nullopt; // sample format not supported for this sample rate

        return closestMatchFormat.channelCount();
    }

    return maxChannelsForFormFactor;
}

std::optional<FormatProbeResult> probeFormats(const ComPtr<IAudioClient> &audioClient,
                                              const ComPtr<IPropertyStore> &propertyStore)
{
    using namespace QWindowsAudioUtils;

    // probing formats is a bit slow, so we limit the number of channels of we can
    std::optional<EndpointFormFactor> formFactor =
            propertyStore ? inferFormFactor(propertyStore) : std::nullopt;
    int maxChannelsForFormFactor = formFactor ? maxChannelCountForFormFactor(*formFactor) : 128;

    std::optional<FormatProbeResult> limits;
    for (QAudioFormat::SampleFormat sampleFormat : QtPrivate::allSupportedSampleFormats) {
        for (int sampleRate : QtPrivate::allSupportedSampleRates) {

            // we initially probe for 128 channels for the format.
            // wasapi will recommend a "closest" match, containing the max number of channels we can
            // probe for.
            std::optional<int> maxChannelForFormat = inferMaxNumberOfChannelsForSampleRate(
                    audioClient, sampleFormat, sampleRate, maxChannelsForFormFactor);

            if (!maxChannelForFormat)
                continue;

            for (int channelCount = 1; channelCount != *maxChannelForFormat + 1; ++channelCount) {
                QAudioFormat fmt;
                fmt.setSampleFormat(sampleFormat);
                fmt.setSampleRate(sampleRate);
                fmt.setChannelCount(channelCount);

                std::optional<WAVEFORMATEXTENSIBLE> formatEx = toWaveFormatExtensible(fmt);
                if (!formatEx)
                    continue;

                QComTaskResource<WAVEFORMATEX> closestMatch;
                HRESULT result = audioClient->IsFormatSupported(
                        AUDCLNT_SHAREMODE_SHARED, &formatEx->Format, closestMatch.address());

                if (FAILED(result))
                    continue;

                if (closestMatch)
                    continue; // we don't have an exact match, but just something close by

                if (!limits)
                    limits = FormatProbeResult{};

                limits->update(fmt);
            }
        }
    }

    return limits;
}

std::optional<QAudioFormat> probePreferredFormat(const ComPtr<IAudioClient> &audioClient)
{
    using namespace QWindowsAudioUtils;

    static const QAudioFormat preferredFormat = [] {
        QAudioFormat fmt;
        fmt.setSampleRate(44100);
        fmt.setChannelCount(2);
        fmt.setSampleFormat(QAudioFormat::Int16);
        return fmt;
    }();

    std::optional<WAVEFORMATEXTENSIBLE> formatEx = toWaveFormatExtensible(preferredFormat);
    if (!formatEx)
        return std::nullopt;

    QComTaskResource<WAVEFORMATEX> closestMatch;
    HRESULT result = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &formatEx->Format,
                                                    closestMatch.address());

    if (FAILED(result))
        return std::nullopt;
    if (!closestMatch)
        return preferredFormat;

    QAudioFormat closestMatchFormat = waveFormatExToFormat(*closestMatch);
    if (closestMatchFormat.isValid())
        return closestMatchFormat;
    return std::nullopt;
}

} // namespace

QWindowsAudioDeviceInfo::QWindowsAudioDeviceInfo(QByteArray dev, ComPtr<IMMDevice> immDev,
                                                 const QString &description,
                                                 QAudioDevice::Mode mode)
    : QAudioDevicePrivate(dev, mode), m_immDev(std::move(immDev))
{
    Q_ASSERT(m_immDev);

    this->description = description;

    ComPtr<IAudioClient> audioClient;
    HRESULT hr = m_immDev->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, nullptr,
                                    reinterpret_cast<void **>(audioClient.GetAddressOf()));

    if (SUCCEEDED(hr)) {
        QComTaskResource<WAVEFORMATEX> mixFormat;
        hr = audioClient->GetMixFormat(mixFormat.address());
        if (SUCCEEDED(hr))
            preferredFormat = QWindowsAudioUtils::waveFormatExToFormat(*mixFormat);
    } else {
        qWarning() << "QWindowsAudioDeviceInfo: could not activate audio client:" << description
                   << QSystemError::windowsComString(hr);
        return;
    }

    ComPtr<IPropertyStore> props;
    hr = m_immDev->OpenPropertyStore(STGM_READ, props.GetAddressOf());
    if (!SUCCEEDED(hr)) {
        qWarning() << "QWindowsAudioDeviceInfo: could not open property store:" << description
                   << QSystemError::windowsComString(hr);
        props.Reset();
    }

    std::optional<FormatProbeResult> probedFormats = probeFormats(audioClient, props);
    if (probedFormats) {
        supportedSampleFormats.assign(probedFormats->supportedSampleFormats.begin(),
                                      probedFormats->supportedSampleFormats.end());

        minimumSampleRate = probedFormats->sampleRateRange.first;
        maximumSampleRate = probedFormats->sampleRateRange.second;
        minimumChannelCount = probedFormats->channelCountRange.first;
        maximumChannelCount = probedFormats->channelCountRange.second;
    }

    if (!preferredFormat.isValid()) {
        std::optional<QAudioFormat> probedFormat = probePreferredFormat(audioClient);
        if (probedFormat)
            preferredFormat = *probedFormat;
    }

    std::optional<QAudioFormat::ChannelConfig> config;
    if (props)
        config = inferChannelConfiguration(props, maximumChannelCount);

    channelConfiguration = config
            ? *config
            : QAudioFormat::defaultChannelConfigForChannelCount(maximumChannelCount);
}

QWindowsAudioDeviceInfo::~QWindowsAudioDeviceInfo() = default;

QT_END_NAMESPACE
