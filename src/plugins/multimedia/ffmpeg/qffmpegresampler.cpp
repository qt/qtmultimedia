// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegresampler_p.h"
#include "playbackengine/qffmpegcodec_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include <qloggingcategory.h>

static Q_LOGGING_CATEGORY(qLcResampler, "qt.multimedia.ffmpeg.resampler")

QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

QFFmpegResampler::QFFmpegResampler(const QAudioFormat &inputFormat, const QAudioFormat &outputFormat) :
    m_inputFormat(inputFormat), m_outputFormat(outputFormat)
{
    Q_ASSERT(inputFormat.isValid());
    Q_ASSERT(outputFormat.isValid());

    m_resampler =
            createResampleContext(AVAudioFormat(m_inputFormat), AVAudioFormat(m_outputFormat));
}

QFFmpegResampler::QFFmpegResampler(const Codec* codec, const QAudioFormat &outputFormat)
    : m_outputFormat(outputFormat)
{
    Q_ASSERT(codec);

    qCDebug(qLcResampler) << "createResampler";
    const AVStream *audioStream = codec->stream();

    if (!m_outputFormat.isValid())
        // want the native format
        m_outputFormat = QFFmpegMediaFormatInfo::audioFormatFromCodecParameters(audioStream->codecpar);

    m_resampler = createResampleContext(AVAudioFormat(audioStream->codecpar),
                                        AVAudioFormat(m_outputFormat));
}

QFFmpegResampler::~QFFmpegResampler() = default;

QAudioBuffer QFFmpegResampler::resample(const char* data, size_t size)
{
    if (!m_inputFormat.isValid())
        return {};

    return resample(reinterpret_cast<const uint8_t **>(&data),
                    m_inputFormat.framesForBytes(static_cast<qint32>(size)));
}

QAudioBuffer QFFmpegResampler::resample(const AVFrame *frame)
{
    return resample(const_cast<const uint8_t **>(frame->extended_data), frame->nb_samples);
}

QAudioBuffer QFFmpegResampler::resample(const uint8_t **inputData, int inputSamplesCount)
{
    const int maxOutSamples = adjustMaxOutSamples(inputSamplesCount);

    QByteArray samples(m_outputFormat.bytesForFrames(maxOutSamples), Qt::Uninitialized);
    auto *out = reinterpret_cast<uint8_t *>(samples.data());
    const int outSamples =
            swr_convert(m_resampler.get(), &out, maxOutSamples, inputData, inputSamplesCount);

    samples.resize(m_outputFormat.bytesForFrames(outSamples));

    qint64 startTime = m_outputFormat.durationForFrames(m_samplesProcessed);
    m_samplesProcessed += outSamples;

    qCDebug(qLcResampler) << "    new frame" << startTime << "in_samples" << inputSamplesCount
                          << outSamples << maxOutSamples;
    return QAudioBuffer(samples, m_outputFormat, startTime);
}

int QFFmpegResampler::adjustMaxOutSamples(int inputSamplesCount)
{
    int maxOutSamples = swr_get_out_samples(m_resampler.get(), inputSamplesCount);

    const auto remainingCompensationDistance = m_endCompensationSample - m_samplesProcessed;

    if (remainingCompensationDistance > 0 && maxOutSamples > remainingCompensationDistance) {
        // If the remaining compensation distance less than output frame,
        // the ffmpeg resampler bufferises the rest of frames that makes
        // unexpected delays on large frames.
        // The hack might cause some compensation bias on large frames,
        // however it's not significant for our logic, in fact.
        // TODO: probably, it will need some improvements
        setSampleCompensation(0, 0);
        maxOutSamples = swr_get_out_samples(m_resampler.get(), inputSamplesCount);
    }

    return maxOutSamples;
}

void QFFmpegResampler::setSampleCompensation(qint32 delta, quint32 distance)
{
    const int res = swr_set_compensation(m_resampler.get(), delta, static_cast<int>(distance));
    if (res < 0)
        qCWarning(qLcResampler) << "swr_set_compensation fail:" << res;
    else {
        m_sampleCompensationDelta = delta;
        m_endCompensationSample = m_samplesProcessed + distance;
    }
}

qint32 QFFmpegResampler::activeSampleCompensationDelta() const
{
    return m_samplesProcessed < m_endCompensationSample ? m_sampleCompensationDelta : 0;
}

QT_END_NAMESPACE
