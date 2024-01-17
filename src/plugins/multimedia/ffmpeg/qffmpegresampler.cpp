// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegresampler_p.h"
#include "playbackengine/qffmpegcodec_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include <qloggingcategory.h>

static Q_LOGGING_CATEGORY(qLcResampler, "qt.multimedia.ffmpeg.resampler")

QT_BEGIN_NAMESPACE

namespace QFFmpeg
{

Resampler::Resampler(const Codec *codec, const QAudioFormat &outputFormat)
    : m_outputFormat(outputFormat)
{
    Q_ASSERT(codec);

    qCDebug(qLcResampler) << "createResampler";
    const AVStream *audioStream = codec->stream();

    if (!m_outputFormat.isValid())
        // want the native format
        m_outputFormat = QFFmpegMediaFormatInfo::audioFormatFromCodecParameters(audioStream->codecpar);

    m_resampler = createResampleContext(ResampleAudioFormat(audioStream->codecpar),
                                        ResampleAudioFormat(m_outputFormat));
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
