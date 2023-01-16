// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegresampler_p.h"
#include "playbackengine/qffmpegcodec_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include <qloggingcategory.h>

extern "C" {
#include <libavutil/opt.h>
}

static Q_LOGGING_CATEGORY(qLcResampler, "qt.multimedia.ffmpeg.resampler")

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


    qCDebug(qLcResampler) << "init resampler" << m_outputFormat.sampleRate() << config << codecpar->sample_rate;
#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    auto inConfig = codecpar->channel_layout;
    if (inConfig == 0)
        inConfig = QFFmpegMediaFormatInfo::avChannelLayout(QAudioFormat::defaultChannelConfigForChannelCount(codecpar->channels));
    resampler = swr_alloc_set_opts(nullptr,  // we're allocating a new context
                                   QFFmpegMediaFormatInfo::avChannelLayout(config),  // out_ch_layout
                                   QFFmpegMediaFormatInfo::avSampleFormat(m_outputFormat.sampleFormat()),    // out_sample_fmt
                                   m_outputFormat.sampleRate(),                // out_sample_rate
                                   inConfig, // in_ch_layout
                                   AVSampleFormat(codecpar->format),   // in_sample_fmt
                                   codecpar->sample_rate,                // in_sample_rate
                                   0,                    // log_offset
                                   nullptr);
#else
    AVChannelLayout in_ch_layout = codecpar->ch_layout;
    AVChannelLayout out_ch_layout = {};
    av_channel_layout_from_mask(&out_ch_layout, QFFmpegMediaFormatInfo::avChannelLayout(config));
    swr_alloc_set_opts2(&resampler,  // we're allocating a new context
                        &out_ch_layout,
                        QFFmpegMediaFormatInfo::avSampleFormat(m_outputFormat.sampleFormat()),
                        m_outputFormat.sampleRate(),
                        &in_ch_layout,
                        AVSampleFormat(codecpar->format),
                        codecpar->sample_rate,
                        0,
                        nullptr);
#endif
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
