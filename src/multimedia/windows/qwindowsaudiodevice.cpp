// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsaudiodevice_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qt_windows.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qsystemerror_p.h>

#include <QtMultimedia/private/qaudioformat_p.h>
#include <QtMultimedia/private/qcominitializer_p.h>
#include <QtMultimedia/private/qcomtaskresource_p.h>
#include <QtMultimedia/private/qwindows_propertystore_p.h>
#include <QtMultimedia/private/qwindowsaudioutils_p.h>

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <propkeydef.h>

#include <set>

QT_BEGIN_NAMESPACE

using QtMultimediaPrivate::PropertyStoreHelper;

namespace {

Q_STATIC_LOGGING_CATEGORY(qLcAudioDeviceProbes, "qt.multimedia.audiodevice.probes");

std::optional<EndpointFormFactor> inferFormFactor(PropertyStoreHelper &propertyStore)
{
    std::optional<uint32_t> val = propertyStore.getUInt32(PKEY_AudioEndpoint_FormFactor);
    if (val == EndpointFormFactor::UnknownFormFactor)
        return EndpointFormFactor(*val);

    return std::nullopt;
}

std::optional<QAudioFormat::ChannelConfig>
inferChannelConfiguration(PropertyStoreHelper &propertyStore, int maximumChannelCount)
{
    std::optional<uint32_t> val = propertyStore.getUInt32(PKEY_AudioEndpoint_PhysicalSpeakers);
    if (val && val != 0)
        return QWindowsAudioUtils::maskToChannelConfig(*val, maximumChannelCount);

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

    [[maybe_unused]]
    friend QDebug operator<<(QDebug dbg, const FormatProbeResult &self)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();

        dbg << "FormatProbeResult{supportedSampleFormats: " << self.supportedSampleFormats
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
                                              PropertyStoreHelper &propertyStore,
                                              const QAudioFormat &preferredFormat)
{
    using namespace QWindowsAudioUtils;

    // probing formats is a bit slow, so we limit the number of channels of we can
    std::optional<EndpointFormFactor> formFactor = inferFormFactor(propertyStore);
    int maxChannelsForFormFactor = formFactor ? maxChannelCountForFormFactor(*formFactor) : 128;

    qCDebug(qLcAudioDeviceProbes) << "probing: maxChannelsForFormFactor" << maxChannelsForFormFactor << formFactor;

    std::optional<FormatProbeResult> limits;

    // Note: probing for AUDCLNT_SHAREMODE_SHARED, float32 seems to be the preferred format for all
    // devices.
    constexpr QAudioFormat::SampleFormat initialSampleFormat = QAudioFormat::SampleFormat::Float;

    // we initially probe for the maximum channel count for the format.
    // wasapi will typically recommend a "closest" match, containing the max number of channels
    // we can probe for.
    QAudioFormat initialProbeFormat;
    initialProbeFormat.setSampleFormat(initialSampleFormat);
    initialProbeFormat.setSampleRate(preferredFormat.sampleRate());
    initialProbeFormat.setChannelCount(maxChannelsForFormFactor);

    qCDebug(qLcAudioDeviceProbes) << "probeFormats: probing for" << initialProbeFormat;

    std::optional<QAudioFormat> initialProbeResult =
            performIsFormatSupportedWithClosestMatch(audioClient, initialProbeFormat);

    int maxChannelForFormat;
    if (initialProbeResult) {
        if (initialProbeResult->sampleRate() != preferredFormat.sampleRate()) {
            qCDebug(qLcAudioDeviceProbes)
                    << "probing: returned a different sample rate as closest match ..."
                    << *initialProbeResult;
            return std::nullopt;
        }

        maxChannelForFormat = initialProbeResult->channelCount();
    } else {
        // some drivers seem to not report any closest match, but simply fail.
        // in this case we need to brute-force enumerate the formats
        // however probing is rather expensive, so we limit our probes to a maxmimum of 2
        // channels
        maxChannelForFormat = std::min(maxChannelsForFormFactor, 2);
    }

    // we rely on wasapi's AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM for format conversion, so we only
    // need to check the closest match format
    QAudioFormat::SampleFormat probeSampleFormat =
            initialProbeResult ? initialProbeResult->sampleFormat() : initialSampleFormat;

    for (int channelCount = 1; channelCount != maxChannelForFormat + 1; ++channelCount) {
        QAudioFormat fmt;
        fmt.setSampleFormat(probeSampleFormat);
        fmt.setSampleRate(preferredFormat.sampleRate());
        fmt.setChannelCount(channelCount);

        std::optional<WAVEFORMATEXTENSIBLE> formatEx = toWaveFormatExtensible(fmt);
        if (!formatEx)
            continue;

        qCDebug(qLcAudioDeviceProbes) << "probing" << fmt;

        QComTaskResource<WAVEFORMATEX> closestMatch;
        HRESULT result = audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &formatEx->Format,
                                                        closestMatch.address());

        if (FAILED(result)) {
            qCDebug(qLcAudioDeviceProbes)
                    << "probing format failed" << QSystemError::windowsComString(result);
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

QWindowsAudioDevice::QWindowsAudioDevice(QByteArray id, ComPtr<IMMDevice> immDev, QString desc,
                                         QAudioDevice::Mode mode)
    : QAudioDevicePrivate(std::move(id), mode, std::move(desc))
{
    Q_ASSERT(immDev);

    ComPtr<IAudioClient> audioClient;
    HRESULT hr = immDev->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, nullptr,
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

    auto propStoreHelper = PropertyStoreHelper::open(immDev);
    if (!propStoreHelper) {
        qWarning() << "QWindowsAudioDeviceInfo: could not open property store:" << description
                   << propStoreHelper.error();
        return;
    }

    qCDebug(qLcAudioDeviceProbes) << "probing formats for" << description;

    std::optional<FormatProbeResult> probedFormats =
            probeFormats(audioClient, *propStoreHelper, preferredFormat);
    if (probedFormats) {
        // wasapi does sample format conversion for us: AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
        supportedSampleFormats = qAllSupportedSampleFormats();

        // this is a bit of a lie:
        // for sources, WASAPI only supports a single sample rate, but we inject a resampler
        // for sinks, WASAPI resamples internally
        minimumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.front();
        maximumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.back();

        minimumChannelCount = 1; // we are lying here, but expect the QWASAPIAudioSinkStream to
                                 // perform format conversion
        maximumChannelCount = probedFormats->channelCountRange.second;

        m_probedChannelCountRange = probedFormats->channelCountRange;
        m_probedSampleRateRange = probedFormats->sampleRateRange;
    }

    if (!preferredFormat.isValid()) {
        std::optional<QAudioFormat> probedFormat = probePreferredFormat(audioClient);
        if (probedFormat)
            preferredFormat = *probedFormat;
    }

    std::optional<QAudioFormat::ChannelConfig> config =
            inferChannelConfiguration(*propStoreHelper, maximumChannelCount);

    channelConfiguration = config
            ? *config
            : QAudioFormat::defaultChannelConfigForChannelCount(maximumChannelCount);
}

ComPtr<IMMDevice> QWindowsAudioDevice::open() const
{
    QComInitializer init;
    ComPtr<IMMDeviceEnumerator> deviceEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&deviceEnumerator));
    if (FAILED(hr)) {
        qWarning() << "Failed to create device enumerator" << hr;
        return nullptr;
    }

    auto deviceId = QString::fromUtf8(id);

    ComPtr<IMMDevice> device;
    HRESULT result =
            deviceEnumerator->GetDevice(deviceId.toStdWString().c_str(), device.GetAddressOf());
    if (FAILED(result)) {
        qWarning() << "IMMDeviceEnumerator::GetDevice failed" << id
                   << QSystemError::windowsComString(result);
        return nullptr;
    }
    return device;
}

QWindowsAudioDevice::~QWindowsAudioDevice() = default;

QT_END_NAMESPACE
