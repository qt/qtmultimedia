// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#include "resonance_audio.h"
#include "graph/resonance_audio_api_impl.h"
#include "graph/graph_manager.h"

namespace vraudio
{

ResonanceAudio::ResonanceAudio(size_t num_channels, size_t frames_per_buffer, int sample_rate_hz)
{
    api = CreateResonanceAudioApi(num_channels, frames_per_buffer, sample_rate_hz);
    impl = static_cast<ResonanceAudioApiImpl *>(api);
}

ResonanceAudio::~ResonanceAudio()
{
    delete api;
}

int ResonanceAudio::getAmbisonicOutput(const float *buffers[], const float *reverb[], int nChannels)
{
    impl->ProcessNextBuffer();
    auto *buffer = impl->GetAmbisonicOutputBuffer();
    if (nChannels != buffer->num_channels())
        return -1;

    for (int i = 0; i < nChannels; ++i) {
        buffers[i] = buffer->begin()[i].begin();
    }

    if (roomEffectsEnabled) {
        const vraudio::AudioBuffer *reverbBuffer = impl->GetReverbBuffer();
        for (int i = 0; i < 2; ++i) {
            reverb[i] = reverbBuffer->begin()[i].begin();
        }
    }

    return buffer->num_frames();
}

}
