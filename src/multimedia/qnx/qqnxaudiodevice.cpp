// Copyright (C) 2016 Research In Motion
// Copyright (C) 2021 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxaudiodevice_p.h"

#include "qqnxaudioutils_p.h"

#include <array>

#include <sys/asoundlib.h>

using namespace QnxAudioUtils;

QT_BEGIN_NAMESPACE

QnxAudioDeviceInfo::QnxAudioDeviceInfo(const QByteArray &deviceName, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(deviceName, mode, QString::fromUtf8(deviceName))
{
    isDefault = id.contains("Preferred");

    preferredFormat.setChannelCount(mode == QAudioDevice::Input ? 1 : 2);

    minimumChannelCount = 1;
    maximumChannelCount = 2;

    const std::optional<snd_pcm_channel_info_t> info = pcmChannelInfo(id, mode);

    if (!info)
        return;

    minimumSampleRate = info->min_rate;
    maximumSampleRate = info->max_rate;

    constexpr std::array sampleRates { 48000, 44100, 22050, 16000, 11025, 8000 };

    for (int rate : sampleRates) {
        if (rate <= maximumSampleRate && rate >= minimumSampleRate) {
            preferredFormat.setSampleRate(rate);
            break;
        }
    }

    if (info->formats & SND_PCM_FMT_U8) {
        supportedSampleFormats << QAudioFormat::UInt8;
        preferredFormat.setSampleFormat(QAudioFormat::UInt8);
    }

    if (info->formats & SND_PCM_FMT_S16) {
        supportedSampleFormats << QAudioFormat::Int16;
        preferredFormat.setSampleFormat(QAudioFormat::Int16);
    }

    if (info->formats & SND_PCM_FMT_S32)
        supportedSampleFormats << QAudioFormat::Int32;

    if (info->formats & SND_PCM_FMT_FLOAT)
        supportedSampleFormats << QAudioFormat::Float;
}

QnxAudioDeviceInfo::~QnxAudioDeviceInfo()
{
}

bool QnxAudioDeviceInfo::isFormatSupported(const QAudioFormat &format) const
{
    const HandleUniquePtr handle = openPcmDevice(id, mode);

    if (!handle)
        return false;

    const std::optional<snd_pcm_channel_info_t> info = pcmChannelInfo(handle.get(), mode);

    if (!info)
        return false;

    snd_pcm_channel_params_t params = formatToChannelParams(format, mode, info->max_fragment_size);
    const int errorCode = snd_pcm_plugin_params(handle.get(), &params);

    return errorCode == 0;
}

QT_END_NAMESPACE
