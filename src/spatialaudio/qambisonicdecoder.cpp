// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#include "qambisonicdecoder_p.h"

#include <QtCore/qdebug.h>
#include <QtMultimedia/private/qaudio_qspan_support_p.h>
#include <QtSpatialAudio/private/qambisonicdecoderdata_p.h>

#include <algorithm>
#include <array>
#include <cmath>

QT_BEGIN_NAMESPACE

// Ambisonic decoding is described in detail in https://ambisonics.dreamhosters.com/BLaH3.pdf.
// We're using a phase matched band splitting filter to split the ambisonic signal into a low
// and high frequency component and apply matrix conversions to those components individually
// as described in the document.
//
// We are currently not using a near field compensation filter, something that could potentially
// improve sound quality further.
//
// For mono and stereo decoding, we use a simpler algorithm to avoid artificially dampening signals
// coming from the back, as we do not have any speakers in that direction and the calculations
// through matlab would give us audible 'holes'.

struct QAmbisonicDecoderData
{
    const QAudioFormat::ChannelConfig config;
    const std::array<const float *, 3> lf;
    const std::array<const float *, 3> hf;
    const float *const reverb;
};

constexpr float reverb_x_0[] = {
    1.f, 0.f, // L
    0.f, 1.f, // R
    .7f, .7f, // C
    1.f, 0.f, // Ls
    0.f, 1.f, // Rs
    1.f, 0.f, // Lb
    0.f, 1.f, // Rb
};

constexpr float reverb_x_1[] = {
    1.f, 0.f, // L
    0.f, 1.f, // R
    .7f, .7f, // C
    .0f, .0f, // LFE
    1.f, 0.f, // Ls
    0.f, 1.f, // Rs
    1.f, 0.f, // Lb
    0.f, 1.f, // Rb
};

static constexpr QAmbisonicDecoderData decoderMap[] = {
    { QAudioFormat::ChannelConfigSurround5Dot0,
      {{ decoderMatrix_5dot0_1_lf, decoderMatrix_5dot0_2_lf, decoderMatrix_5dot0_3_lf }},
      {{ decoderMatrix_5dot0_1_hf, decoderMatrix_5dot0_2_hf, decoderMatrix_5dot0_3_hf }},
      reverb_x_0 },
    { QAudioFormat::ChannelConfigSurround5Dot1,
      {{ decoderMatrix_5dot1_1_lf, decoderMatrix_5dot1_2_lf, decoderMatrix_5dot1_3_lf }},
      {{ decoderMatrix_5dot1_1_hf, decoderMatrix_5dot1_2_hf, decoderMatrix_5dot1_3_hf }},
      reverb_x_1 },
    { QAudioFormat::ChannelConfigSurround7Dot0,
      {{ decoderMatrix_7dot0_1_lf, decoderMatrix_7dot0_2_lf, decoderMatrix_7dot0_3_lf }},
      {{ decoderMatrix_7dot0_1_hf, decoderMatrix_7dot0_2_hf, decoderMatrix_7dot0_3_hf }},
      reverb_x_0 },
    { QAudioFormat::ChannelConfigSurround7Dot1,
      {{ decoderMatrix_7dot1_1_lf, decoderMatrix_7dot1_2_lf, decoderMatrix_7dot1_3_lf }},
      {{ decoderMatrix_7dot1_1_hf, decoderMatrix_7dot1_2_hf, decoderMatrix_7dot1_3_hf }},
      reverb_x_1 }
};

// Implements a split second order IIR filter (TDF-II)
// The audio data is split into a phase synced low and high frequency part
// This allows us to apply different factors to both parts for better sound
// localization when converting from ambisonic formats
//
// Details are described in https://ambisonics.dreamhosters.com/BLaH3.pdf, Appendix A.2.
class QAmbisonicDecoderFilter
{
public:
    QAmbisonicDecoderFilter() = default;
    void configure(float sampleRate, float cutoffFrequency = 380)
    {
        double k = std::tan(M_PI * cutoffFrequency / sampleRate);
        double denom = k * k + 2.0 * k + 1.0;

        a1 = 2.0 * (k * k - 1.0) / denom;
        a2 = (k * k - 2.0 * k + 1.0) / denom;

        b0_lf = (k * k) / denom;
        b1_lf = 2.0 * b0_lf;

        b0_hf = 1.0 / denom;
        b1_hf = -2.0 * b0_hf;
    }

    struct Output
    {
        float lf;
        float hf;
    };

    Output next(float x)
    {
#ifdef Q_PROCESSOR_X86
        // tiny DC offset to prevent denormals, which can cause severe performance degradation on x86 CPUs
        x += 1e-18f;
#endif
        // Process LF
        double r_lf = x * b0_lf + s1_lf;
        s1_lf = x * b1_lf - r_lf * a1 + s2_lf;
        s2_lf = x * b0_lf - r_lf * a2;

        // Process HF
        double r_hf = x * b0_hf + s1_hf;
        s1_hf = x * b1_hf - r_hf * a1 + s2_hf;
        s2_hf = x * b0_hf - r_hf * a2;

        return Output{
            float(r_lf),
            float(r_hf),
        };
    }

private:
    // coefficients
    double a1 = 0.;
    double a2 = 0.;
    double b0_hf = 0.;
    double b1_hf = 0.;
    double b0_lf = 0.;
    double b1_lf = 0.;

    // state
    double s1_lf = 0.0, s2_lf = 0.0;
    double s1_hf = 0.0, s2_hf = 0.0;
};

namespace {

int inputChannelsForAmbisonicOrder(QAmbisonicDecoder::AmbisonicOrder ambisonicOrder)
{
    auto order = qToUnderlying(ambisonicOrder);
    Q_ASSERT(order > 0 && order <= 3);
    return (order + 1) * (order + 1);
}

QAudioFormat::ChannelConfig ambisonicDecoderChannelConfig(QAudioFormat::ChannelConfig channelConfig,
                                                          int numberOfOutputChannels)
{
    if (channelConfig == QAudioFormat::ChannelConfigUnknown)
        channelConfig = QAudioFormat::defaultChannelConfigForChannelCount(numberOfOutputChannels);
    return channelConfig;
}

} // namespace

QAmbisonicDecoder::QAmbisonicDecoder(AmbisonicOrder ambisonicOrder, int sampleRate,
                                     int numberOfOutputChannels,
                                     QAudioFormat::ChannelConfig channelCfg)
    : channelConfig(ambisonicDecoderChannelConfig(channelCfg, numberOfOutputChannels)),
      order(ambisonicOrder),
      inputChannels(inputChannelsForAmbisonicOrder(ambisonicOrder)),
      outputChannels(numberOfOutputChannels)
{
    if (channelConfig == QAudioFormat::ChannelConfigMono ||
        channelConfig == QAudioFormat::ChannelConfigStereo ||
        channelConfig == QAudioFormat::ChannelConfig2Dot1 ||
        channelConfig == QAudioFormat::ChannelConfig3Dot0 ||
        channelConfig == QAudioFormat::ChannelConfig3Dot1) {
        // these are non surround configs and handled manually to avoid
        // audible holes for sounds coming from behing
        //
        // We use a simpler decoding process here, only taking first order
        // ambisonics into account
        //
        // Left and right channels get 50% W and 50% X
        // Center gets 50% W and 50% Y
        // LFE gets 50% W
        simpleDecoderFactors = std::make_unique<float[]>(4 * outputChannels);
        m_reverbFactorsOwned = std::make_unique<float[]>(2 * outputChannels); // reverb output is in stereo
        reverbFactors = m_reverbFactorsOwned.get();
        float *f = simpleDecoderFactors.get();
        float *r = m_reverbFactorsOwned.get();
        if (channelConfig & QAudioFormat::channelConfig(QAudioFormat::FrontLeft)) {
            f[0] = 0.5f; f[1] = 0.5f; f[2] = 0.; f[3] = 0.f;
            f += 4;
            r[0] = 1.; r[1] = 0.;
            r += 2;
        }
        if (channelConfig & QAudioFormat::channelConfig(QAudioFormat::FrontRight)) {
            f[0] = 0.5f; f[1] = -0.5f; f[2] = 0.; f[3] = 0.f;
            f += 4;
            r[0] = 0.; r[1] = 1.;
            r += 2;
        }
        if (channelConfig & QAudioFormat::channelConfig(QAudioFormat::FrontCenter)) {
            f[0] = 0.5f; f[1] = -0.f; f[2] = 0.; f[3] = 0.5f;
            f += 4;
            r[0] = .5; r[1] = .5;
            r += 2;
        }
        if (channelConfig & QAudioFormat::channelConfig(QAudioFormat::LFE)) {
            f[0] = 0.5f; f[1] = -0.f; f[2] = 0.; f[3] = 0.0f;
            f += 4;
            r[0] = 0.; r[1] = 0.;
            r += 2;
        }
        Q_UNUSED(f);
        Q_UNUSED(r);
        Q_ASSERT((f - simpleDecoderFactors.get()) == 4 * outputChannels);
        Q_ASSERT((r - reverbFactors) == 2*outputChannels);

        return;
    }

    for (const auto &d : decoderMap) {
        if (d.config == channelConfig) {
            decoderData = &d;
            reverbFactors = decoderData->reverb;
            break;
        }
    }
    if (!decoderData)
        return;

    filters = std::make_unique<QAmbisonicDecoderFilter[]>(inputChannels);
    for (int i = 0; i < inputChannels; ++i)
        filters[i].configure(sampleRate);
}

QAmbisonicDecoder::~QAmbisonicDecoder() = default;

void QAmbisonicDecoder::processBuffer(QSpan<const float *> input, QSpan<float> output)
{
    const int nSamples = int(output.size()) / outputChannels;
    std::fill(output.begin(), output.end(), 0.f);
    float *o = output.data();

    if (simpleDecoderFactors) {
        for (int i = 0; i < nSamples; ++i) {
            for (int j = 0; j < 4; ++j) {
                for (int k = 0; k < outputChannels; ++k)
                    o[k] += simpleDecoderFactors[k*4 + j]*input[j][i];
            }
            o += outputChannels;
        }
        return;
    }

    const float *matrix_hi = decoderData->hf[order - 1];
    const float *matrix_lo = decoderData->lf[order - 1];
    for (int i = 0; i < nSamples; ++i) {
        std::array<QAmbisonicDecoderFilter::Output, maxAmbisonicChannels> buf;
        for (int j = 0; j < inputChannels; ++j)
            buf[j] = filters[j].next(input[j][i]);
        for (int j = 0; j < inputChannels; ++j) {
            for (int k = 0; k < outputChannels; ++k)
                o[k] += matrix_lo[k*inputChannels + j]*buf[j].lf + matrix_hi[k*inputChannels + j]*buf[j].hf;
        }
        o += outputChannels;
    }
}

void QAmbisonicDecoder::processBufferWithReverb(QSpan<const float *> input,
                                                QSpan<const float *, 2> reverb, QSpan<float> output)
{
    Q_ASSERT(outputChannels > 0);
    Q_ASSERT(int(output.size()) % outputChannels == 0);
    const int nSamples = int(output.size()) / outputChannels;

    std::fill(output.begin(), output.end(), 0.f);
    float *o = output.data();

    if (simpleDecoderFactors) {
        for (int i = 0; i < nSamples; ++i) {
            for (int k = 0; k < outputChannels; ++k) {
                for (int j = 0; j < 4; ++j)
                    o[k] += simpleDecoderFactors[k*4 + j]*input[j][i];
            }
            if (reverb[0]) {
                for (int k = 0; k < outputChannels; ++k)
                    o[k] += reverb[0][i] * reverbFactors[2 * k]
                            + reverb[1][i] * reverbFactors[2 * k + 1];
            }

            o += outputChannels;
        }
        return;
    }

    Q_ASSERT(filters);

    const float *matrix_hi = decoderData->hf[order - 1];
    const float *matrix_lo = decoderData->lf[order - 1];
    for (int i = 0; i < nSamples; ++i) {
        std::array<QAmbisonicDecoderFilter::Output, maxAmbisonicChannels> buf;
        for (int j = 0; j < inputChannels; ++j)
            buf[j] = filters[j].next(input[j][i]);
        for (int j = 0; j < inputChannels; ++j) {
            for (int k = 0; k < outputChannels; ++k)
                o[k] += matrix_lo[k*inputChannels + j]*buf[j].lf + matrix_hi[k*inputChannels + j]*buf[j].hf;
        }
        if (reverb[0]) {
            for (int k = 0; k < outputChannels; ++k)
                o[k] += reverb[0][i]*reverbFactors[2*k] + reverb[1][i]*reverbFactors[2*k+1];
        }
        o += outputChannels;
    }
}

QT_END_NAMESPACE

