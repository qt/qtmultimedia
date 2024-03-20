// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#ifndef QAMBISONICDECODER_P_H
#define QAMBISONICDECODER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qtspatialaudioglobal_p.h>
#include <qaudioformat.h>

QT_BEGIN_NAMESPACE

struct QAmbisonicDecoderData;
class QAmbisonicDecoderFilter;

class QAmbisonicDecoder
{
public:
    enum AmbisonicLevel
    {
        AmbisonicLevel1 = 1,
        LowQuality = AmbisonicLevel1,
        AmbisonicLevel2 = 2,
        MediumQuality = AmbisonicLevel2,
        AmbisonicLevel3 = 3,
        HighQuality = AmbisonicLevel3
    };
    QAmbisonicDecoder(AmbisonicLevel ambisonicLevel, const QAudioFormat &format);
    ~QAmbisonicDecoder();

    bool hasValidConfig() const { return outputChannels > 0; }

    int nInputChannels() const { return inputChannels; }
    int nOutputChannels() const { return outputChannels; }

    int outputSize(int nSamples) const { return outputChannels * nSamples; }

    // input is planar, output interleaved
    void processBuffer(const float *input[], float *output, int nSamples);
    void processBuffer(const float *input[], short *output, int nSamples);

    void processBufferWithReverb(const float *input[], const float *reverb[2], short *output, int nSamples);

    static constexpr int maxAmbisonicChannels = 16;
    static constexpr int maxAmbisonicLevel = 3;
private:
    QAudioFormat::ChannelConfig channelConfig;
    AmbisonicLevel level = AmbisonicLevel1;
    int inputChannels = 0;
    int outputChannels = 0;
    const QAmbisonicDecoderData *decoderData = nullptr;
    QAmbisonicDecoderFilter *filters = nullptr;
    float *simpleDecoderFactors = nullptr;
    const float *reverbFactors = nullptr;
};


QT_END_NAMESPACE

#endif
