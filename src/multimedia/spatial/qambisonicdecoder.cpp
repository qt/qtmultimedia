/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qambisonicdecoder_p.h"

#include "qambisonicdecoderdata_p.h"
#include <cmath>

QT_BEGIN_NAMESPACE

// Ambisonic decoding is described in detail in https://ambisonics.dreamhosters.com/BLaH3.pdf.
// We're using a phase matched band splitting filter to split the ambisonic signal into a low
// and high frequency component and apply matrix conversions to those components individually
// as described in the document.
//
// We are currently not using a near field compensation filter, something that could potentially
// improve sound quality further.


struct QAmbisonicDecoderData
{
    QAudioFormat::ChannelConfig config;
    const float *lf[3];
    const float *hf[3];
};

static const QAmbisonicDecoderData decoderMap[] =
{
    { QAudioFormat::ChannelConfigMono,
      { decoderMatrix_mono_1_lf, decoderMatrix_mono_2_lf, decoderMatrix_mono_3_lf },
      { decoderMatrix_mono_1_hf, decoderMatrix_mono_2_hf, decoderMatrix_mono_3_hf }
    },
    { QAudioFormat::ChannelConfigStereo,
      { decoderMatrix_stereo_1_lf, decoderMatrix_stereo_2_lf, decoderMatrix_stereo_3_lf },
      { decoderMatrix_stereo_1_hf, decoderMatrix_stereo_2_hf, decoderMatrix_stereo_3_hf }
    },
    { QAudioFormat::ChannelConfig2Dot1,
      { decoderMatrix_2dot1_1_lf, decoderMatrix_2dot1_2_lf, decoderMatrix_2dot1_3_lf },
      { decoderMatrix_2dot1_1_hf, decoderMatrix_2dot1_2_hf, decoderMatrix_2dot1_3_hf }
    },
    { QAudioFormat::ChannelConfigSurround5Dot0,
      { decoderMatrix_5dot0_1_lf, decoderMatrix_5dot0_2_lf, decoderMatrix_5dot0_3_lf },
      { decoderMatrix_5dot0_1_hf, decoderMatrix_5dot0_2_hf, decoderMatrix_5dot0_3_hf }
    },
    { QAudioFormat::ChannelConfigSurround5Dot1,
      { decoderMatrix_5dot1_1_lf, decoderMatrix_5dot1_2_lf, decoderMatrix_5dot1_3_lf },
      { decoderMatrix_5dot1_1_hf, decoderMatrix_5dot1_2_hf, decoderMatrix_5dot1_3_hf }
    },
    { QAudioFormat::ChannelConfigSurround7Dot0,
      { decoderMatrix_7dot0_1_lf, decoderMatrix_7dot0_2_lf, decoderMatrix_7dot0_3_lf },
      { decoderMatrix_7dot0_1_hf, decoderMatrix_7dot0_2_hf, decoderMatrix_7dot0_3_hf }
    },
    { QAudioFormat::ChannelConfigSurround7Dot1,
      { decoderMatrix_7dot1_1_lf, decoderMatrix_7dot1_2_lf, decoderMatrix_7dot1_3_lf },
      { decoderMatrix_7dot1_1_hf, decoderMatrix_7dot1_2_hf, decoderMatrix_7dot1_3_hf }
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
    auto outputConfiguration = format.channelConfig();
    if (outputConfiguration == QAudioFormat::ChannelConfigUnknown)
        outputConfiguration = format.defaultChannelConfigForChannelCount(format.channelCount());
    for (const  auto &d : decoderMap) {
        if (d.config == outputConfiguration) {
            decoderData = &d;
            break;
        }
    }
    if (!decoderData) {
        // ### FIXME: use a stereo config, fill other channels with 0
    }

    inputChannels = (level+1)*(level+1);
    outputChannels = format.channelCount();

    filters = new QAmbisonicDecoderFilter[inputChannels];
    for (int i = 0; i < inputChannels; ++i)
        filters[i].configure(format.sampleRate());
}

void QAmbisonicDecoder::processBuffer(const float *input[], float *output, int nSamples)
{
    float *o = output;
    memset(o, 0, nSamples*outputChannels*sizeof(float));

    const float *matrix_hi = decoderData->hf[level];
    const float *matrix_lo = decoderData->hf[level];
    for (int i = 0; i < nSamples; ++i) {
        QAmbisonicDecoderFilter::Output buf[maxAmbisonicChannels];
        for (int j = 0; j < inputChannels; ++j)
            buf[j] = filters[j].next(input[j][i]);
        for (int j = 0; j < inputChannels; ++j) {
            for (int k = 0; k < outputChannels; ++k)
                o[k] += matrix_lo[k*outputChannels + j]*buf[j].lf + matrix_hi[k*outputChannels + j]*buf[j].hf;
        }
        o += outputChannels;
    }
}

void QAmbisonicDecoder::processBuffer(const float *input[], short *output, int nSamples)
{
    if (level == 0) {
        // ### copy W data
        return;
    }
    const float *matrix_hi = decoderData->hf[level - 1];
    const float *matrix_lo = decoderData->hf[level - 1];
    for (int i = 0; i < nSamples; ++i) {
        QAmbisonicDecoderFilter::Output buf[maxAmbisonicChannels];
        for (int j = 0; j < inputChannels; ++j)
            buf[j] = filters[j].next(input[j][i]);
        float o[32]; // we can't support more than 32 channels from our API
        memset(o, 0, 32*sizeof(short));
        for (int j = 0; j < inputChannels; ++j) {
            for (int k = 0; k < outputChannels; ++k)
                o[k] += matrix_lo[k*outputChannels + j]*buf[j].lf + matrix_hi[k*outputChannels + j]*buf[j].hf;
        }
        for (int k = 0; k < outputChannels; ++k)
            output[k] = static_cast<short>(o[k]*32768.);
        output += outputChannels;
    }
}

QT_END_NAMESPACE

