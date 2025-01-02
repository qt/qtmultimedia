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
    SwrContext *resampler = nullptr;
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
    swr_init(resampler);
    m_resampler.reset(resampler);
}

Resampler::~Resampler() = default;

QAudioBuffer Resampler::resample(const AVFrame *frame)
{
    const int maxOutSamples = adjustMaxOutSamples(frame);

    QByteArray samples(m_outputFormat.bytesForFrames(maxOutSamples), Qt::Uninitialized);
    auto **in = const_cast<const uint8_t **>(frame->extended_data);
    auto *out = reinterpret_cast<uint8_t *>(samples.data());
    const int outSamples =
            swr_convert(m_resampler.get(), &out, maxOutSamples, in, frame->nb_samples);

    samples.resize(m_outputFormat.bytesForFrames(outSamples));

    qint64 startTime = m_outputFormat.durationForFrames(m_samplesProcessed);
    m_samplesProcessed += outSamples;

    qCDebug(qLcResampler) << "    new frame" << startTime << "in_samples" << frame->nb_samples
                          << outSamples << maxOutSamples;
    return QAudioBuffer(samples, m_outputFormat, startTime);
}

int Resampler::adjustMaxOutSamples(const AVFrame *frame)
{
    int maxOutSamples = swr_get_out_samples(m_resampler.get(), frame->nb_samples);

    const auto remainingCompensationDistance = m_endCompensationSample - m_samplesProcessed;

    if (remainingCompensationDistance > 0 && maxOutSamples > remainingCompensationDistance) {
        // If the remaining compensation distance less than output frame,
        // the ffmpeg resampler bufferises the rest of frames that makes
        // unexpected delays on large frames.
        // The hack might cause some compensation bias on large frames,
        // however it's not significant for our logic, in fact.
        // TODO: probably, it will need some improvements
        setSampleCompensation(0, 0);
        maxOutSamples = swr_get_out_samples(m_resampler.get(), frame->nb_samples);
    }

    return maxOutSamples;
}

void Resampler::setSampleCompensation(qint32 delta, quint32 distance)
{
    const int res = swr_set_compensation(m_resampler.get(), delta, static_cast<int>(distance));
    if (res < 0)
        qCWarning(qLcResampler) << "swr_set_compensation fail:" << res;
    else {
        m_sampleCompensationDelta = delta;
        m_endCompensationSample = m_samplesProcessed + distance;
    }
}

qint32 Resampler::activeSampleCompensationDelta() const
{
    return m_samplesProcessed < m_endCompensationSample ? m_sampleCompensationDelta : 0;
}
}

QT_END_NAMESPACE
