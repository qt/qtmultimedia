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
#include <QtCore/qloggingcategory.h>
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

static Q_LOGGING_CATEGORY(qLcAudioDeviceProbes, "qt.multimedia.audiodevice.probes");

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

    case EndpointFormFactor::Microphone:
        return 32; // 32 channels should be more than enough for real-world microphones

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

    friend QDebug operator<<(QDebug dbg, const FormatProbeResult& self)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();

        dbg << "FormatProbeResult{supportedSampleFormats: " << QList(self.supportedSampleFormats.begin(), self.supportedSampleFormats.end())
            << ", channelCountRange: " << self.channelCountRange.first << " - " << self.channelCountRange.second
            << ", sampleRateRange: " << self.sampleRateRange.first << "-" << self.sampleRateRange.second
            << "}";
        return dbg;
    }
};

std::optional<QAudioFormat> performIsFormatSupportedWithClosestMatch(const ComPtr<IAudioClient> &audioClient,
                                                                     const QAudioFormat &fmt)
{
    using namespace QWindowsAudioUtils;
    std::optional<WAVEFORMATEXTENSIBLE> formatEx = toWaveFormatExtensible(fmt);
    if (!formatEx) {
        qCWarning(qLcAudioDeviceProbes) << "toWaveFormatExtensible failed" << fmt;
        return std::nullopt;
    }

    qCDebug(qLcAudioDeviceProbes) << "performIsFormatSupportedWithClosestMatch for" << fmt;
    QComTaskResource<WAVEFORMATEX> closestMatch;
    HRESULT result = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &formatEx->Format,
                                                    closestMatch.address());

    if (FAILED(result)) {
        qCDebug(qLcAudioDeviceProbes) << "performIsFormatSupportedWithClosestMatch: error" << QSystemError::windowsComString(result);
        return std::nullopt;
    }

    if (closestMatch) {
        QAudioFormat closestMatchFormat = waveFormatExToFormat(*closestMatch);
        qCDebug(qLcAudioDeviceProbes) << "performProbe returned closest match" << closestMatchFormat;
        return closestMatchFormat;
    }

    qCDebug(qLcAudioDeviceProbes) << "performProbe successful";

    return fmt;
}

std::optional<FormatProbeResult> probeFormats(const ComPtr<IAudioClient> &audioClient,
                                              const ComPtr<IPropertyStore> &propertyStore)
{
    using namespace QWindowsAudioUtils;

    // probing formats is a bit slow, so we limit the number of channels of we can
    std::optional<EndpointFormFactor> formFactor =
            propertyStore ? inferFormFactor(propertyStore) : std::nullopt;
    int maxChannelsForFormFactor = formFactor ? maxChannelCountForFormFactor(*formFactor) : 128;

    qCDebug(qLcAudioDeviceProbes) << "probing: maxChannelsForFormFactor" << maxChannelsForFormFactor << formFactor;

    std::optional<FormatProbeResult> limits;
    for (QAudioFormat::SampleFormat sampleFormat : QtPrivate::allSupportedSampleFormats) {
        for (int sampleRate : QtPrivate::allSupportedSampleRates) {

            // we initially probe for the maximum channel count for the format.
            // wasapi will typically recommend a "closest" match, containing the max number of channels we can
            // probe for.
            QAudioFormat initialProbeFormat;
            initialProbeFormat.setSampleFormat(sampleFormat);
            initialProbeFormat.setSampleRate(sampleRate);
            initialProbeFormat.setChannelCount(maxChannelsForFormFactor);

            qCDebug(qLcAudioDeviceProbes) << "probeFormats: probing for" << initialProbeFormat;

            std::optional<QAudioFormat> initialProbeResult = performIsFormatSupportedWithClosestMatch(
                    audioClient, initialProbeFormat);

            int maxChannelForFormat;
            if (initialProbeResult) {
                if (initialProbeResult->sampleRate() != sampleRate) {
                    qCDebug(qLcAudioDeviceProbes) << "probing: returned a different sample rate as closest match ..." << *initialProbeResult;
                    continue;
                }

                if (initialProbeResult->sampleFormat() != sampleFormat) {
                    qCDebug(qLcAudioDeviceProbes) << "probing: returned a different sample format as closest match ...";
                    continue;
                }
                maxChannelForFormat = initialProbeResult->channelCount();
            } else {
                // some drivers seem to not report any closest match, but simply fail.
                // in this case we need to brute-force enumerate the formats
                // however probing is rather expensive, so we limit our probes to a maxmimum of 2 channels
                maxChannelForFormat = std::min(maxChannelsForFormFactor, 2);
            }

            for (int channelCount = 1; channelCount != maxChannelForFormat + 1; ++channelCount) {
                QAudioFormat fmt;
                fmt.setSampleFormat(sampleFormat);
                fmt.setSampleRate(sampleRate);
                fmt.setChannelCount(channelCount);

                std::optional<WAVEFORMATEXTENSIBLE> formatEx = toWaveFormatExtensible(fmt);
                if (!formatEx)
                    continue;

                qCDebug(qLcAudioDeviceProbes) << "probing" << fmt;

                QComTaskResource<WAVEFORMATEX> closestMatch;
                HRESULT result = audioClient->IsFormatSupported(
                        AUDCLNT_SHAREMODE_SHARED, &formatEx->Format, closestMatch.address());

                if (FAILED(result)) {
                    qCDebug(qLcAudioDeviceProbes) << "probing format failed"
                                                  << QSystemError::windowsComString(result);
                    continue;
                }

                if (closestMatch) {
                    qCDebug(qLcAudioDeviceProbes) << "probing format reported a closest match"
                                                  << waveFormatExToFormat(*closestMatch);
                    continue; // we don't have an exact match, but just something close by
                }

                if (!limits)
                    limits = FormatProbeResult{};

                qCDebug(qLcAudioDeviceProbes) << "probing format successful" << fmt;
                limits->update(fmt);
            }
        }
    }

    qCDebug(qLcAudioDeviceProbes) << "probing successful" << limits;

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

QWindowsAudioDeviceInfo::QWindowsAudioDeviceInfo(QByteArray dev,
                                                 ComPtr<IMMDevice> immDev,
                                                 QString description,
                                                 QAudioDevice::Mode mode)
    : QAudioDevicePrivate(std::move(dev), mode, std::move(description))
    , m_immDev(std::move(immDev))
{
    Q_ASSERT(m_immDev);

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

    qCDebug(qLcAudioDeviceProbes) << "probing formats for" << description;

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
