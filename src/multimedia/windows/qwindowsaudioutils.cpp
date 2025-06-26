// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsaudioutils_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/private/qsystemerror_p.h>
#include <QtMultimedia/private/qwindowsmediafoundation_p.h>


#include <audioclient.h>
#include <mmdeviceapi.h>
#include <ks.h>
#include <ksmedia.h>

// MinGW misses some error codes
#ifndef AUDCLNT_E_EFFECT_NOT_AVAILABLE
#  define AUDCLNT_E_EFFECT_NOT_AVAILABLE AUDCLNT_ERR(0x041)
#endif

#ifndef AUDCLNT_E_EFFECT_STATE_READ_ONLY
#  define AUDCLNT_E_EFFECT_STATE_READ_ONLY AUDCLNT_ERR(0x042)
#endif

QT_BEGIN_NAMESPACE

namespace QWindowsAudioUtils {

using namespace std::chrono_literals;
using namespace Qt::Literals;

static constexpr QAudioFormat::AudioChannelPosition channelFormatMap[] =
        { QAudioFormat::FrontLeft         // SPEAKER_FRONT_LEFT (0x1)
        , QAudioFormat::FrontRight        // SPEAKER_FRONT_RIGHT (0x2)
        , QAudioFormat::FrontCenter       // SPEAKER_FRONT_CENTER (0x4)
        , QAudioFormat::LFE               // SPEAKER_LOW_FREQUENCY (0x8)
        , QAudioFormat::BackLeft          // SPEAKER_BACK_LEFT (0x10)
        , QAudioFormat::BackRight         // SPEAKER_BACK_RIGHT (0x20)
        , QAudioFormat::FrontLeftOfCenter // SPEAKER_FRONT_LEFT_OF_CENTER (0x40)
        , QAudioFormat::FrontRightOfCenter// SPEAKER_FRONT_RIGHT_OF_CENTER (0x80)
        , QAudioFormat::BackCenter        // SPEAKER_BACK_CENTER (0x100)
        , QAudioFormat::SideLeft          // SPEAKER_SIDE_LEFT (0x200)
        , QAudioFormat::SideRight         // SPEAKER_SIDE_RIGHT (0x400)
        , QAudioFormat::TopCenter         // SPEAKER_TOP_CENTER (0x800)
        , QAudioFormat::TopFrontLeft      // SPEAKER_TOP_FRONT_LEFT (0x1000)
        , QAudioFormat::TopFrontCenter    // SPEAKER_TOP_FRONT_CENTER (0x2000)
        , QAudioFormat::TopFrontRight     // SPEAKER_TOP_FRONT_RIGHT (0x4000)
        , QAudioFormat::TopBackLeft       // SPEAKER_TOP_BACK_LEFT (0x8000)
        , QAudioFormat::TopBackCenter     // SPEAKER_TOP_BACK_CENTER (0x10000)
        , QAudioFormat::TopBackRight      // SPEAKER_TOP_BACK_RIGHT (0x20000)
        };

QAudioFormat::ChannelConfig maskToChannelConfig(UINT32 mask, int count)
{
    quint32 config = 0;
    int set = 0;
    for (auto c : channelFormatMap) {
        if (mask & 1) {
            config |= QAudioFormat::channelConfig(c);
            ++set;
        }
        if (set >= count)
            break;
        mask >>= 1;
    }
    return QAudioFormat::ChannelConfig(config);
}

static UINT32 channelConfigToMask(QAudioFormat::ChannelConfig config)
{
    UINT32 mask = 0;
    quint32 i = 0;
    for (auto c : channelFormatMap) {
        if (config & QAudioFormat::channelConfig(c))
            mask |= 1 << i;
        ++i;
    }
    return mask;
}

bool formatToWaveFormatExtensible(const QAudioFormat &format, WAVEFORMATEXTENSIBLE &wfx)
{
    if (!format.isValid())
        return false;

    wfx.Format.nSamplesPerSec = format.sampleRate();
    wfx.Format.wBitsPerSample = wfx.Samples.wValidBitsPerSample = format.bytesPerSample()*8;
    wfx.Format.nChannels = format.channelCount();
    wfx.Format.nBlockAlign = (wfx.Format.wBitsPerSample / 8) * wfx.Format.nChannels;
    wfx.Format.nAvgBytesPerSec = wfx.Format.nBlockAlign * wfx.Format.nSamplesPerSec;
    wfx.Format.cbSize = 0;

    if (format.sampleFormat() == QAudioFormat::Float) {
        wfx.Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    } else {
        wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }

    if (format.channelCount() > 2) {
        wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx.Format.cbSize = 22;
        wfx.dwChannelMask = format.channelConfig() == QAudioFormat::ChannelConfigUnknown ? KSAUDIO_SPEAKER_DIRECTOUT
                                                                                         : DWORD(format.channelConfig());
    }

    return true;
}

std::optional<WAVEFORMATEXTENSIBLE> toWaveFormatExtensible(const QAudioFormat &format)
{
    WAVEFORMATEXTENSIBLE ret{};
    if (formatToWaveFormatExtensible(format, ret))
        return ret;

    return std::nullopt;
}

QAudioFormat waveFormatExToFormat(const WAVEFORMATEX &in)
{
    QAudioFormat out;
    out.setSampleRate(in.nSamplesPerSec);
    out.setChannelCount(in.nChannels);
    if (in.wFormatTag == WAVE_FORMAT_PCM) {
        if (in.wBitsPerSample == 8)
            out.setSampleFormat(QAudioFormat::UInt8);
        else if (in.wBitsPerSample == 16)
            out.setSampleFormat(QAudioFormat::Int16);
        else if (in.wBitsPerSample == 32)
            out.setSampleFormat(QAudioFormat::Int32);
    } else if (in.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        if (in.cbSize >= 22) {
            auto wfe = reinterpret_cast<const WAVEFORMATEXTENSIBLE &>(in);
            if (wfe.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
                out.setSampleFormat(QAudioFormat::Float);
            if (qPopulationCount(wfe.dwChannelMask) >= in.nChannels)
                out.setChannelConfig(maskToChannelConfig(wfe.dwChannelMask, in.nChannels));
        }
    } else if (in.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        out.setSampleFormat(QAudioFormat::Float);
    }

    return out;
}

QAudioFormat mediaTypeToFormat(IMFMediaType *mediaType)
{
    QAudioFormat format;
    if (!mediaType)
        return format;

    UINT32 val = 0;
    if (SUCCEEDED(mediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &val))) {
        format.setChannelCount(int(val));
    } else {
        qWarning() << "Could not determine channel count from IMFMediaType";
        return {};
    }

    if (SUCCEEDED(mediaType->GetUINT32(MF_MT_AUDIO_CHANNEL_MASK, &val))) {
        if (int(qPopulationCount(val)) >= format.channelCount())
            format.setChannelConfig(maskToChannelConfig(val, format.channelCount()));
    }

    if (SUCCEEDED(mediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &val))) {
        format.setSampleRate(int(val));
    }
    UINT32 bitsPerSample = 0;
    mediaType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);

    GUID subType;
    if (SUCCEEDED(mediaType->GetGUID(MF_MT_SUBTYPE, &subType))) {
        if (subType == MFAudioFormat_Float) {
            format.setSampleFormat(QAudioFormat::Float);
        } else if (bitsPerSample == 8) {
            format.setSampleFormat(QAudioFormat::UInt8);
        } else if (bitsPerSample == 16) {
            format.setSampleFormat(QAudioFormat::Int16);
        } else if (bitsPerSample == 32){
            format.setSampleFormat(QAudioFormat::Int32);
        }
    }
    return format;
}

ComPtr<IMFMediaType> formatToMediaType(QWindowsMediaFoundation &wmf, const QAudioFormat &format)
{
    ComPtr<IMFMediaType> mediaType;

    if (!format.isValid())
        return mediaType;

    wmf.mfCreateMediaType(mediaType.GetAddressOf());

    mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (format.sampleFormat() == QAudioFormat::Float) {
        mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
    } else {
        mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    }

    mediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, UINT32(format.channelCount()));
    if (format.channelConfig() != QAudioFormat::ChannelConfigUnknown)
        mediaType->SetUINT32(MF_MT_AUDIO_CHANNEL_MASK, channelConfigToMask(format.channelConfig()));
    mediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, UINT32(format.sampleRate()));
    auto alignmentBlock = UINT32(format.bytesPerFrame());
    mediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, alignmentBlock);
    auto avgBytesPerSec = UINT32(format.sampleRate() * format.bytesPerFrame());
    mediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, avgBytesPerSec);
    mediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, UINT32(format.bytesPerSample()*8));
    mediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

    return mediaType;
}

std::optional<quint32> getBufferSizeInFrames(const ComPtr<IAudioClient3> &client)
{
    Q_ASSERT(client);

    UINT32 bufferFrameCount = 0;

    HRESULT hr = client->GetBufferSize(&bufferFrameCount);
    if (SUCCEEDED(hr))
        return bufferFrameCount;

    qWarning() << "IAudioClient::getBufferSize failed" << audioClientErrorString(hr);
    return std::nullopt;
}

std::optional<AudioClientDevicePeriod> getDevicePeriod(const ComPtr<IAudioClient3> &client)
{
    Q_ASSERT(client);
    REFERENCE_TIME defaultPeriodDuration, minimalPeriodDuration;
    HRESULT hr = client->GetDevicePeriod(&defaultPeriodDuration, &minimalPeriodDuration);

    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::GetDevicePeriod failed" << audioClientErrorString(hr);
        return std::nullopt;
    }

    return AudioClientDevicePeriod{
        reference_time{ defaultPeriodDuration },
        reference_time{ minimalPeriodDuration },
    };
}

QString audioClientErrorString(HRESULT hr)
{
    switch (hr) {
    case AUDCLNT_E_NOT_INITIALIZED:
        return u"AUDCLNT_E_NOT_INITIALIZED"_s;
    case AUDCLNT_E_ALREADY_INITIALIZED:
        return u"AUDCLNT_E_ALREADY_INITIALIZED"_s;
    case AUDCLNT_E_WRONG_ENDPOINT_TYPE:
        return u"AUDCLNT_E_WRONG_ENDPOINT_TYPE"_s;
    case AUDCLNT_E_DEVICE_INVALIDATED:
        return u"AUDCLNT_E_DEVICE_INVALIDATED"_s;
    case AUDCLNT_E_NOT_STOPPED:
        return u"AUDCLNT_E_NOT_STOPPED"_s;
    case AUDCLNT_E_BUFFER_TOO_LARGE:
        return u"AUDCLNT_E_BUFFER_TOO_LARGE"_s;
    case AUDCLNT_E_OUT_OF_ORDER:
        return u"AUDCLNT_E_OUT_OF_ORDER"_s;
    case AUDCLNT_E_UNSUPPORTED_FORMAT:
        return u"AUDCLNT_E_UNSUPPORTED_FORMAT"_s;
    case AUDCLNT_E_INVALID_SIZE:
        return u"AUDCLNT_E_INVALID_SIZE"_s;
    case AUDCLNT_E_DEVICE_IN_USE:
        return u"AUDCLNT_E_DEVICE_IN_USE"_s;
    case AUDCLNT_E_BUFFER_OPERATION_PENDING:
        return u"AUDCLNT_E_BUFFER_OPERATION_PENDING"_s;
    case AUDCLNT_E_THREAD_NOT_REGISTERED:
        return u"AUDCLNT_E_THREAD_NOT_REGISTERED"_s;
    case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:
        return u"AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED"_s;
    case AUDCLNT_E_ENDPOINT_CREATE_FAILED:
        return u"AUDCLNT_E_ENDPOINT_CREATE_FAILED"_s;
    case AUDCLNT_E_SERVICE_NOT_RUNNING:
        return u"AUDCLNT_E_SERVICE_NOT_RUNNING"_s;
    case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED:
        return u"AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED"_s;
    case AUDCLNT_E_EXCLUSIVE_MODE_ONLY:
        return u"AUDCLNT_E_EXCLUSIVE_MODE_ONLY"_s;
    case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:
        return u"AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL"_s;
    case AUDCLNT_E_EVENTHANDLE_NOT_SET:
        return u"AUDCLNT_E_EVENTHANDLE_NOT_SET"_s;
    case AUDCLNT_E_INCORRECT_BUFFER_SIZE:
        return u"AUDCLNT_E_INCORRECT_BUFFER_SIZE"_s;
    case AUDCLNT_E_BUFFER_SIZE_ERROR:
        return u"AUDCLNT_E_BUFFER_SIZE_ERROR"_s;
    case AUDCLNT_E_CPUUSAGE_EXCEEDED:
        return u"AUDCLNT_E_CPUUSAGE_EXCEEDED"_s;
    case AUDCLNT_E_BUFFER_ERROR:
        return u"AUDCLNT_E_BUFFER_ERROR"_s;
    case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:
        return u"AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED"_s;
    case AUDCLNT_E_INVALID_DEVICE_PERIOD:
        return u"AUDCLNT_E_INVALID_DEVICE_PERIOD"_s;
    case AUDCLNT_E_INVALID_STREAM_FLAG:
        return u"AUDCLNT_E_INVALID_STREAM_FLAG"_s;
    case AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE:
        return u"AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE"_s;
    case AUDCLNT_E_OUT_OF_OFFLOAD_RESOURCES:
        return u"AUDCLNT_E_OUT_OF_OFFLOAD_RESOURCES"_s;
    case AUDCLNT_E_OFFLOAD_MODE_ONLY:
        return u"AUDCLNT_E_OFFLOAD_MODE_ONLY"_s;
    case AUDCLNT_E_NONOFFLOAD_MODE_ONLY:
        return u"AUDCLNT_E_NONOFFLOAD_MODE_ONLY"_s;
    case AUDCLNT_E_RESOURCES_INVALIDATED:
        return u"AUDCLNT_E_RESOURCES_INVALIDATED"_s;
    case AUDCLNT_E_RAW_MODE_UNSUPPORTED:
        return u"AUDCLNT_E_RAW_MODE_UNSUPPORTED"_s;
    case AUDCLNT_E_ENGINE_PERIODICITY_LOCKED:
        return u"AUDCLNT_E_ENGINE_PERIODICITY_LOCKED"_s;
    case AUDCLNT_E_ENGINE_FORMAT_LOCKED:
        return u"AUDCLNT_E_ENGINE_FORMAT_LOCKED"_s;
    case AUDCLNT_E_HEADTRACKING_ENABLED:
        return u"AUDCLNT_E_HEADTRACKING_ENABLED"_s;
    case AUDCLNT_E_HEADTRACKING_UNSUPPORTED:
        return u"AUDCLNT_E_HEADTRACKING_UNSUPPORTED"_s;
    case AUDCLNT_E_EFFECT_NOT_AVAILABLE:
        return u"AUDCLNT_E_EFFECT_NOT_AVAILABLE"_s;
    case AUDCLNT_E_EFFECT_STATE_READ_ONLY:
        return u"AUDCLNT_E_EFFECT_STATE_READ_ONLY"_s;

    default:
        return QSystemError::windowsComString(hr);
    }
}

bool audioClientSetRole(const ComPtr<IAudioClient3> &client, AudioEndpointRole role)
{
    AudioClientProperties properties{};
    properties.cbSize = sizeof(AudioClientProperties);

    switch (role) {
    case AudioEndpointRole::MediaPlayback:
        properties.eCategory = AudioCategory_Media;
        break;
    case AudioEndpointRole::SoundEffect:
        properties.eCategory = AudioCategory_SoundEffects;
        break;
    case AudioEndpointRole::Accessibility:
        properties.eCategory = AudioCategory_Speech;
        break;
    case AudioEndpointRole::Other:
        properties.eCategory = AudioCategory_Other;
        break;
    default:
        Q_UNREACHABLE_RETURN(false);
    }

    HRESULT hr = client->SetClientProperties(&properties);
    if (FAILED(hr)) {
        qWarning() << "IAudioClient2::SetClientProperties failed" << audioClientErrorString(hr);
        abort();
        return false;
    }

    return true;
}

std::optional<AudioClientCreationResult>
createAudioClient(const ComPtr<IMMDevice> &device, const QAudioFormat &format,
                  std::optional<qsizetype> hardwareBufferFrames,
                  const QUniqueWin32NullHandle &wasapiEventHandle,
                  std::optional<AudioEndpointRole> role)
{
    ComPtr<IAudioClient3> audioClient;

    HRESULT hr = device->Activate(__uuidof(IAudioClient3), CLSCTX_INPROC_SERVER, nullptr,
                                  reinterpret_cast<void **>(audioClient.GetAddressOf()));
    if (FAILED(hr)) {
        qWarning() << "Failed to activate audio device" << audioClientErrorString(hr);
        return std::nullopt;
    }

    std::optional<AudioClientDevicePeriod> devicePeriods = getDevicePeriod(audioClient);
    if (!devicePeriods)
        return std::nullopt;

    // qCDebug(qLcAudioSource) << devicePeriods->defaultDuration << devicePeriods->minimalDuration;
    reference_time periodSize = devicePeriods->defaultDuration;
    if (hardwareBufferFrames) {
        std::chrono::microseconds dur{ format.durationForFrames(*hardwareBufferFrames) };
        if (dur < devicePeriods->minimalDuration)
            qWarning() << "Requested hardware buffer size to small:" << dur << "minimal duration"
                       << devicePeriods->minimalDuration;
        else
            periodSize = dur;
    }

    auto fmt = toWaveFormatExtensible(format);
    if (!fmt)
        return std::nullopt;
    auto waveFormat = *fmt;

    if (role)
        audioClientSetRole(audioClient, *role);

    constexpr DWORD streamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK
            | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
            | AUDCLNT_STREAMFLAGS_RATEADJUST;

    hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, streamFlags,
                                 /*hnsBufferDuration=*/reference_time(periodSize).count(),
                                 /*hnsPeriodicity=*/reference_time(periodSize).count(),
                                 &waveFormat.Format, nullptr);

    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::Initialize failed" << audioClientErrorString(hr);
        return std::nullopt;
    }

    std::optional<quint32> framesPerBuffer = getBufferSizeInFrames(audioClient);
    if (!framesPerBuffer)
        return std::nullopt;
    qsizetype audioClientFrames = *framesPerBuffer;

    hr = audioClient->SetEventHandle(wasapiEventHandle.get());
    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::SetEventHandle failed" << audioClientErrorString(hr);
        return std::nullopt;
    }

    return AudioClientCreationResult{
        /*.client =*/std::move(audioClient),
        /*.periodSize =*/periodSize,
        /*.audioClientFrames =*/audioClientFrames,
    };
}

bool audioClientStart(const ComPtr<IAudioClient3> &client)
{
    HRESULT hr = client->Start();
    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::Start failed" << audioClientErrorString(hr);
        return false;
    }
    return true;
}

bool audioClientStop(const ComPtr<IAudioClient3> &client)
{
    HRESULT hr = client->Stop();
    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::Stop failed" << audioClientErrorString(hr);
        return false;
    }
    return true;
}

bool audioClientReset(const ComPtr<IAudioClient3> &client)
{
    HRESULT hr = client->Reset();
    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::Reset failed" << audioClientErrorString(hr);
        return false;
    }
    return true;
}

bool audioClientSetRate(const ComPtr<IAudioClient3> &client, int rate)
{
    ComPtr<IAudioClockAdjustment> clockAdjustment;

    HRESULT hr = client->GetService(IID_PPV_ARGS(clockAdjustment.GetAddressOf()));
    if (FAILED(hr)) {
        qWarning() << "IAudioClient3::GetService failed to obtain IAudioClockAdjustment"
                   << audioClientErrorString(hr);
        return false;
    }

    hr = clockAdjustment->SetSampleRate(float(rate));
    if (FAILED(hr)) {
        qWarning() << "IAudioClockAdjustment::SetSampleRate failed" << audioClientErrorString(hr);
        return false;
    }

    return true;
}

void setMCSSForPeriodSize(reference_time periodSize)
{
    // heuristics are similar to what windows does for exclusive mode:
    // 10ms is the default, when going lower, "Pro Audio" is the way to go
    auto threadCharacteristics = periodSize < 10ms ? "Pro Audio" : "Audio";
    DWORD taskIndex = 0;
    HANDLE hTask = AvSetMmThreadCharacteristicsA(threadCharacteristics, &taskIndex);

    if (hTask == NULL) {
        qWarning() << "AvSetMmThreadCharacteristics failed to set thread characteristics"
                   << QSystemError::windowsString();
    }

    AVRT_PRIORITY priority = periodSize < 10ms ? AVRT_PRIORITY_CRITICAL : AVRT_PRIORITY_HIGH;
    bool success = AvSetMmThreadPriority(hTask, priority);
    if (!success)
        qWarning() << "AvSetMmThreadPriority failed to set thread priority"
                   << QSystemError::windowsString();
}
} // namespace QWindowsAudioUtils

QT_END_NAMESPACE
