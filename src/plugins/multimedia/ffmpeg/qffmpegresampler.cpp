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
#include "qffmpegresampler_p.h"
#include "qffmpegdecoder_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include <qloggingcategory.h>

extern "C" {
#include <libavutil/opt.h>
}

Q_LOGGING_CATEGORY(qLcResampler, "qt.multimedia.ffmpeg.resampler")

QT_BEGIN_NAMESPACE

namespace QFFmpeg
{

Resampler::Resampler(const Codec *codec, const QAudioFormat &outputFormat)
    : m_outputFormat(outputFormat)
{
    qCDebug(qLcResampler) << "createResampler";
    const AVStream *audioStream = codec->stream();
    const auto *codecpar = audioStream->codecpar;

    if (!m_outputFormat.isValid())
        // want the native format
        m_outputFormat = QFFmpegMediaFormatInfo::audioFormatFromCodecParameters(audioStream->codecpar);

    QAudioFormat::ChannelConfig config = m_outputFormat.channelConfig();
    if (config == QAudioFormat::ChannelConfigUnknown)
        config = QAudioFormat::defaultChannelConfigForChannelCount(m_outputFormat.channelCount());

    auto inConfig = codecpar->channel_layout;
    if (inConfig == 0)
        inConfig = QFFmpegMediaFormatInfo::avChannelLayout(QAudioFormat::defaultChannelConfigForChannelCount(codecpar->channels));

    qCDebug(qLcResampler) << "init resampler" << m_outputFormat.sampleRate() << config << codecpar->sample_rate;
    resampler = swr_alloc_set_opts(nullptr,  // we're allocating a new context
                                   QFFmpegMediaFormatInfo::avChannelLayout(config),  // out_ch_layout
                                   QFFmpegMediaFormatInfo::avSampleFormat(m_outputFormat.sampleFormat()),    // out_sample_fmt
                                   m_outputFormat.sampleRate(),                // out_sample_rate
                                   inConfig, // in_ch_layout
                                   AVSampleFormat(codecpar->format),   // in_sample_fmt
                                   codecpar->sample_rate,                // in_sample_rate
                                   0,                    // log_offset
                                   nullptr);

    // if we're not the master clock, we might need to handle clock adjustments, initialize for that
    av_opt_set_double(resampler, "async", m_outputFormat.sampleRate()/50, 0);

    swr_init(resampler);
}

Resampler::~Resampler()
{
    swr_free(&resampler);
}

QAudioBuffer Resampler::resample(const AVFrame *frame)
{
    const int outSamples = swr_get_out_samples(resampler, frame->nb_samples);
    QByteArray samples(m_outputFormat.bytesForFrames(outSamples), Qt::Uninitialized);
    auto **in = const_cast<const uint8_t **>(frame->extended_data);
    auto *out = reinterpret_cast<uint8_t *>(samples.data());
    const int out_samples = swr_convert(resampler, &out, outSamples,
                                  in, frame->nb_samples);
    samples.resize(m_outputFormat.bytesForFrames(out_samples));

    qint64 startTime = m_outputFormat.durationForFrames(m_samplesProcessed);
    m_samplesProcessed += out_samples;

    qCDebug(qLcResampler) << "    new frame" << startTime << "in_samples" << frame->nb_samples << out_samples << outSamples;
    QAudioBuffer buffer(samples, m_outputFormat, startTime);
    return buffer;
}


}

QT_END_NAMESPACE
