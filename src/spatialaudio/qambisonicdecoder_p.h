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

#include <QtSpatialAudio/private/qtspatialaudioglobal_p.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtCore/qspan.h>

#include <memory>

QT_BEGIN_NAMESPACE

struct QAmbisonicDecoderData;
class QAmbisonicDecoderFilter;

class QAmbisonicDecoder
{
public:
    enum AmbisonicOrder {
        Ambisonic1stOrder = 1,
        LowQuality = Ambisonic1stOrder,
        Ambisonic2ndOrder = 2,
        MediumQuality = Ambisonic2ndOrder,
        Ambisonic3rdOrder = 3,
        HighQuality = Ambisonic3rdOrder
    };
    QAmbisonicDecoder(AmbisonicOrder, int sampleRate, int numberOfOutputChannels,
                      QAudioFormat::ChannelConfig);
    ~QAmbisonicDecoder();

    bool hasValidConfig() const { return decoderData != nullptr || simpleDecoderFactors != nullptr; }

    int nInputChannels() const { return inputChannels; }
    int nOutputChannels() const { return outputChannels; }

    int outputSamples(int nFrames) const { return outputChannels * nFrames; }

    // input is planar, output interleaved
    void processBuffer(QSpan<const float *> input, QSpan<float> output);

    void processBufferWithReverb(QSpan<const float *> input, QSpan<const float *, 2> reverb,
                                 QSpan<float> output);

    static constexpr int maxAmbisonicChannels = 16;
    static constexpr int maxAmbisonicOrder = 3;

private:
    const QAudioFormat::ChannelConfig channelConfig;
    const AmbisonicOrder order;
    const int inputChannels;
    const int outputChannels;
    const QAmbisonicDecoderData *decoderData = nullptr;
    std::unique_ptr<QAmbisonicDecoderFilter[]> filters;
    std::unique_ptr<float[]> simpleDecoderFactors;

    const float *reverbFactors = nullptr;
    std::unique_ptr<float[]> m_reverbFactorsOwned;
};


QT_END_NAMESPACE

#endif
