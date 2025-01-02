// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsaudioutils_p.h"
#include "qwindowsmediafoundation_p.h"
#include "qdebug.h"
#include "ks.h"
#include "ksmedia.h"

#include <audioclient.h>

QT_BEGIN_NAMESPACE

static QAudioFormat::AudioChannelPosition channelFormatMap[] =
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

QAudioFormat::ChannelConfig QWindowsAudioUtils::maskToChannelConfig(UINT32 mask, int count)
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

bool QWindowsAudioUtils::formatToWaveFormatExtensible(const QAudioFormat &format, WAVEFORMATEXTENSIBLE &wfx)
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

QAudioFormat QWindowsAudioUtils::waveFormatExToFormat(const WAVEFORMATEX &in)
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

QAudioFormat QWindowsAudioUtils::mediaTypeToFormat(IMFMediaType *mediaType)
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

ComPtr<IMFMediaType> QWindowsAudioUtils::formatToMediaType(QWindowsMediaFoundation &wmf, const QAudioFormat &format)
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

std::optional<quint32> QWindowsAudioUtils::audioClientFramesInUse(IAudioClient *client)
{
    Q_ASSERT(client);
    UINT32 framesPadding = 0;
    if (SUCCEEDED(client->GetCurrentPadding(&framesPadding)))
        return framesPadding;
    return {};
}

std::optional<quint32> QWindowsAudioUtils::audioClientFramesAllocated(IAudioClient *client)
{
    Q_ASSERT(client);
    UINT32 bufferFrameCount = 0;
    if (SUCCEEDED(client->GetBufferSize(&bufferFrameCount)))
        return bufferFrameCount;
    return {};
}

QT_END_NAMESPACE
