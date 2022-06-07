// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#ifndef RESONANCE_AUDIO_H
#define RESONANCE_AUDIO_H

#include <api/resonance_audio_api.h>

namespace vraudio
{

class ResonanceAudioExtensions;
class ResonanceAudioApiImpl;

class EXPORT_API ResonanceAudio
{
public:
    ResonanceAudio(size_t num_channels, size_t frames_per_buffer, int sample_rate_hz);
    ~ResonanceAudio();

    // reverb is only calculated in stereo. We get it here as well, and our ambisonic
    // decoder will then add it to the generated surround signal.
    int getAmbisonicOutput(const float *buffers[], const float *reverb[], int nChannels);

    ResonanceAudioApi *api = nullptr;
    ResonanceAudioApiImpl *impl = nullptr;
    bool roomEffectsEnabled = true;
};



}

#endif
