// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qambisonicdecoder_p.h"

#include "qambisonicdecoderdata_p.h"
#include <cmath>
#include <qdebug.h>

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
    QAudioFormat::ChannelConfig config;
    const float *lf[3];
    const float *hf[3];
    const float *reverb;
};

const float reverb_x_0[] =
{
    1.f, 0.f, // L
    0.f, 1.f, // R
    .7f, .7f, // C
    1.f, 0.f, // Ls
    0.f, 1.f, // Rs
    1.f, 0.f, // Lb
    0.f, 1.f, // Rb
};

const float reverb_x_1[] =
{
    1.f, 0.f, // L
    0.f, 1.f, // R
    .7f, .7f, // C
    .0f, .0f, // LFE
    1.f, 0.f, // Ls
    0.f, 1.f, // Rs
    1.f, 0.f, // Lb
    0.f, 1.f, // Rb
};

static const QAmbisonicDecoderData decoderMap[] =
{
    { QAudioFormat::ChannelConfigSurround5Dot0,
      { decoderMatrix_5dot0_1_lf, decoderMatrix_5dot0_2_lf, decoderMatrix_5dot0_3_lf },
      { decoderMatrix_5dot0_1_hf, decoderMatrix_5dot0_2_hf, decoderMatrix_5dot0_3_hf },
      reverb_x_0
    },
    { QAudioFormat::ChannelConfigSurround5Dot1,
      { decoderMatrix_5dot1_1_lf, decoderMatrix_5dot1_2_lf, decoderMatrix_5dot1_3_lf },
      { decoderMatrix_5dot1_1_hf, decoderMatrix_5dot1_2_hf, decoderMatrix_5dot1_3_hf },
      reverb_x_1
    },
    { QAudioFormat::ChannelConfigSurround7Dot0,
      { decoderMatrix_7dot0_1_lf, decoderMatrix_7dot0_2_lf, decoderMatrix_7dot0_3_lf },
      { decoderMatrix_7dot0_1_hf, decoderMatrix_7dot0_2_hf, decoderMatrix_7dot0_3_hf },
      reverb_x_0
    },
    { QAudioFormat::ChannelConfigSurround7Dot1,
      { decoderMatrix_7dot1_1_lf, decoderMatrix_7dot1_2_lf, decoderMatrix_7dot1_3_lf },
      { decoderMatrix_7dot1_1_hf, decoderMatrix_7dot1_2_hf, decoderMatrix_7dot1_3_hf },
      reverb_x_1
    }
};

// Implements a split second order IIR filter
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
        double k = tan(M_PI*cutoffFrequency/sampleRate);
        a1 = float(2.*(k*k - 1.)/(k*k + 2*k + 1.));
        a2 = float((k*k - 2*k + 1.)/(k*k + 2*k + 1.));

        b0_lf = float(k*k/(k*k + 2*k + 1));
        b1_lf = 2.f*b0_lf;

        b0_hf = float(1./(k*k + 2*k + 1));
        b1_hf = -2.f*b0_hf;
    }

    struct Output
    {
        float lf;
        float hf;
    };

    Output next(float x)
    {
        float r_lf = x*b0_lf +
                  prevX[0]*b1_lf +
                  prevX[1]*b0_lf -
                  prevR_lf[0]*a1 -
                  prevR_lf[1]*a2;
        float r_hf = x*b0_hf +
                  prevX[0]*b1_hf +
                  prevX[1]*b0_hf -
                  prevR_hf[0]*a1 -
                  prevR_hf[1]*a2;
        prevX[1] = prevX[0];
        prevX[0] = x;
        prevR_lf[1] = prevR_lf[0];
        prevR_lf[0] = r_lf;
        prevR_hf[1] = prevR_hf[0];
        prevR_hf[0] = r_hf;
        return { r_lf, r_hf };
    }

private:
    float a1 = 0.;
    float a2 = 0.;

    float b0_hf = 0.;
    float b1_hf = 0.;

    float b0_lf = 0.;
    float b1_lf = 0.;

    float prevX[2] = {};
    float prevR_lf[2] = {};
    float prevR_hf[2] = {};
};


QAmbisonicDecoder::QAmbisonicDecoder(AmbisonicLevel ambisonicLevel, const QAudioFormat &format)
    : level(ambisonicLevel)
{
    Q_ASSERT(level > 0 && level <= 3);
    inputChannels = (level+1)*(level+1);
    outputChannels = format.channelCount();

    channelConfig = format.channelConfig();
    if (channelConfig == QAudioFormat::ChannelConfigUnknown)
        channelConfig = format.defaultChannelConfigForChannelCount(format.channelCount());

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
        simpleDecoderFactors = new float[4*outputChannels];
        float *r = new float[2*outputChannels]; // reverb output is in stereo
        float *f = simpleDecoderFactors;
        reverbFactors = r;
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
        Q_ASSERT((f - simpleDecoderFactors) == 4*outputChannels);
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
    if (!decoderData) {
        // can't handle this,
        outputChannels = 0;
        return;
    }

    filters = new QAmbisonicDecoderFilter[inputChannels];
    for (int i = 0; i < inputChannels; ++i)
        filters[i].configure(format.sampleRate());
}

QAmbisonicDecoder::~QAmbisonicDecoder()
{
    if (simpleDecoderFactors) {
        delete simpleDecoderFactors;
        delete reverbFactors;
    }
}

void QAmbisonicDecoder::processBuffer(const float *input[], float *output, int nSamples)
{
    float *o = output;
    memset(o, 0, nSamples*outputChannels*sizeof(float));

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

    const float *matrix_hi = decoderData->hf[level - 1];
    const float *matrix_lo = decoderData->lf[level - 1];
    for (int i = 0; i < nSamples; ++i) {
        QAmbisonicDecoderFilter::Output buf[maxAmbisonicChannels];
        for (int j = 0; j < inputChannels; ++j)
            buf[j] = filters[j].next(input[j][i]);
        for (int j = 0; j < inputChannels; ++j) {
            for (int k = 0; k < outputChannels; ++k)
                o[k] += matrix_lo[k*inputChannels + j]*buf[j].lf + matrix_hi[k*inputChannels + j]*buf[j].hf;
        }
        o += outputChannels;
    }
}

void QAmbisonicDecoder::processBuffer(const float *input[], short *output, int nSamples)
{
    const float *reverb[] = { nullptr, nullptr };
    return processBufferWithReverb(input, reverb, output, nSamples);
}

void QAmbisonicDecoder::processBufferWithReverb(const float *input[], const float *reverb[2], short *output, int nSamples)
{
    if (simpleDecoderFactors) {
        for (int i = 0; i < nSamples; ++i) {
            float o[4] = {};
            for (int k = 0; k < outputChannels; ++k) {
                for (int j = 0; j < 4; ++j)
                    o[k] += simpleDecoderFactors[k*4 + j]*input[j][i];
            }
            if (reverb[0]) {
                for (int k = 0; k < outputChannels; ++k) {
                    o[k] += reverb[0][i]*reverbFactors[2*k] + reverb[1][i]*reverbFactors[2*k+1];
                }
            }

            for (int k = 0; k < outputChannels; ++k)
                output[k] = static_cast<short>(o[k]*32768.);
            output += outputChannels;
        }
        return;
    }

    //    qDebug() << "XXX" << inputChannels << outputChannels;
    const float *matrix_hi = decoderData->hf[level - 1];
    const float *matrix_lo = decoderData->lf[level - 1];
    for (int i = 0; i < nSamples; ++i) {
        QAmbisonicDecoderFilter::Output buf[maxAmbisonicChannels];
        for (int j = 0; j < inputChannels; ++j)
            buf[j] = filters[j].next(input[j][i]);
        float o[32] = {}; // we can't support more than 32 channels from our API
        for (int j = 0; j < inputChannels; ++j) {
            for (int k = 0; k < outputChannels; ++k)
                o[k] += matrix_lo[k*inputChannels + j]*buf[j].lf + matrix_hi[k*inputChannels + j]*buf[j].hf;
        }
        if (reverb[0]) {
            for (int k = 0; k < outputChannels; ++k) {
                o[k] += reverb[0][i]*reverbFactors[2*k] + reverb[1][i]*reverbFactors[2*k+1];
            }
        }
        for (int k = 0; k < outputChannels; ++k)
            output[k] = static_cast<short>(o[k]*32768.);
        output += outputChannels;
    }

}

QT_END_NAMESPACE

