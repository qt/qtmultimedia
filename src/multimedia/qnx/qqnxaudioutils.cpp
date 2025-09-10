// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxaudioutils_p.h"

QT_BEGIN_NAMESPACE

namespace QnxAudioUtils
{

snd_pcm_channel_params_t formatToChannelParams(const QAudioFormat &format, QAudioDevice::Mode mode, int fragmentSize)
{
    snd_pcm_channel_params_t params;
    memset(&params, 0, sizeof(params));
    params.channel = (mode == QAudioDevice::Output) ? SND_PCM_CHANNEL_PLAYBACK : SND_PCM_CHANNEL_CAPTURE;
    params.mode = SND_PCM_MODE_BLOCK;
    params.start_mode = SND_PCM_START_DATA;
    params.stop_mode = SND_PCM_STOP_ROLLOVER;
    params.buf.block.frag_size = fragmentSize;
    params.buf.block.frags_min = 1;
    params.buf.block.frags_max = 1;
    strcpy(params.sw_mixer_subchn_name, "QAudio Channel");

    params.format.interleave = 1;
    params.format.rate = format.sampleRate();
    params.format.voices = format.channelCount();

    switch (format.sampleFormat()) {
    case QAudioFormat::UInt8:
        params.format.format = SND_PCM_SFMT_U8;
        break;
    default:
        // fall through
    case QAudioFormat::Int16:
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        params.format.format = SND_PCM_SFMT_S16_LE;
#else
        params.format.format = SND_PCM_SFMT_S16_BE;
#endif
        break;
    case QAudioFormat::Int32:
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        params.format.format = SND_PCM_SFMT_S32_LE;
#else
        params.format.format = SND_PCM_SFMT_S32_BE;
#endif
        break;
    case QAudioFormat::Float:
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        params.format.format = SND_PCM_SFMT_FLOAT_LE;
#else
        params.format.format = SND_PCM_SFMT_FLOAT_BE;
#endif
        break;
    }

    return params;
}


HandleUniquePtr openPcmDevice(const QByteArray &id, QAudioDevice::Mode mode)
{
    const int pcmMode = mode == QAudioDevice::Output
        ? SND_PCM_OPEN_PLAYBACK
        : SND_PCM_OPEN_CAPTURE;

    snd_pcm_t *handle;

    const int errorCode = snd_pcm_open_name(&handle, id.constData(), pcmMode);

    if (errorCode != 0) {
        qWarning("Unable to open PCM device %s (0x%x)", id.constData(), -errorCode);
        return {};
    }

    return HandleUniquePtr { handle };
}

template <typename T, typename Func>
std::optional<T> pcmChannelGetStruct(snd_pcm_t *handle, QAudioDevice::Mode mode, Func &&func)
{
    // initialize in-place to prevent an extra copy when returning
    std::optional<T> t = { T{} };

    t->channel = mode == QAudioDevice::Output
        ? SND_PCM_CHANNEL_PLAYBACK
        : SND_PCM_CHANNEL_CAPTURE;

    const int errorCode = func(handle, &(*t));

    if (errorCode != 0) {
        qWarning("QAudioDevice: couldn't get channel info (0x%x)", -errorCode);
        return {};
    }

    return t;
}

template <typename T, typename Func>
std::optional<T> pcmChannelGetStruct(const QByteArray &device,
        QAudioDevice::Mode mode, Func &&func)
{
    const HandleUniquePtr handle = openPcmDevice(device, mode);

    if (!handle)
        return {};

    return pcmChannelGetStruct<T>(handle.get(), mode, std::forward<Func>(func));
}


std::optional<snd_pcm_channel_info_t> pcmChannelInfo(snd_pcm_t *handle, QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_info_t>(handle, mode, snd_pcm_plugin_info);
}

std::optional<snd_pcm_channel_info_t> pcmChannelInfo(const QByteArray &device,
        QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_info_t>(device, mode, snd_pcm_plugin_info);
}

std::optional<snd_pcm_channel_setup_t> pcmChannelSetup(snd_pcm_t *handle, QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_setup_t>(handle, mode, snd_pcm_plugin_setup);
}

std::optional<snd_pcm_channel_setup_t> pcmChannelSetup(const QByteArray &device,
        QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_setup_t>(device, mode, snd_pcm_plugin_setup);
}

std::optional<snd_pcm_channel_status_t> pcmChannelStatus(snd_pcm_t *handle, QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_status_t>(handle, mode, snd_pcm_plugin_status);
}

std::optional<snd_pcm_channel_status_t> pcmChannelStatus(const QByteArray &device,
        QAudioDevice::Mode mode)
{
    return pcmChannelGetStruct<snd_pcm_channel_status_t>(device, mode, snd_pcm_plugin_status);
}


} // namespace QnxAudioUtils

QT_END_NAMESPACE
