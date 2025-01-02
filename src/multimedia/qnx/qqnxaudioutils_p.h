// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNXAUDIOUTILS_H
#define QNXAUDIOUTILS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qaudiosystem_p.h"

#include <memory>
#include <optional>

#include <sys/asoundlib.h>

QT_BEGIN_NAMESPACE

namespace QnxAudioUtils
{
    snd_pcm_channel_params_t formatToChannelParams(const QAudioFormat &format, QAudioDevice::Mode mode, int fragmentSize);

    struct HandleDeleter
    {
        void operator()(snd_pcm_t *h) { if (h) snd_pcm_close(h); }
    };

    using HandleUniquePtr = std::unique_ptr<snd_pcm_t, HandleDeleter>;
    HandleUniquePtr openPcmDevice(const QByteArray &id, QAudioDevice::Mode mode);

    std::optional<snd_pcm_channel_info_t> pcmChannelInfo(snd_pcm_t *handle, QAudioDevice::Mode mode);
    std::optional<snd_pcm_channel_info_t> pcmChannelInfo(const QByteArray &device, QAudioDevice::Mode mode);

    std::optional<snd_pcm_channel_setup_t> pcmChannelSetup(snd_pcm_t *handle, QAudioDevice::Mode mode);
    std::optional<snd_pcm_channel_setup_t> pcmChannelSetup(const QByteArray &device, QAudioDevice::Mode mode);

    std::optional<snd_pcm_channel_status_t> pcmChannelStatus(snd_pcm_t *handle, QAudioDevice::Mode mode);
    std::optional<snd_pcm_channel_status_t> pcmChannelStatus(const QByteArray &device, QAudioDevice::Mode mode);
}

QT_END_NAMESPACE

#endif
