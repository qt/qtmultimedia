// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegaudioframeconverter_p.h"

#include <QtMultimedia/private/qaudiobuffer_support_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegframe_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegresampler_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegmediaformatinfo_p.h>

#if defined(Q_CC_MSVC) && defined(QT_MM_OPTIMIZE_DEBUG)
#  pragma optimize("s", on)
#endif

// TODO: namespace 3p library to prevent odr violations
#include <signalsmith-stretch.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

namespace {

qreal sampleRateFactor()
{
    // Test purposes:
    //
    // The env var describes a factor for the sample rate of
    // audio data that we feed to the audio sink.
    //
    // In some cases  audio sink might consume data slightly slower or faster than expected;
    // even though the synchronization in the audio renderer is supposed to handle it,
    // it makes sense to experiment with QT_MEDIA_PLAYER_AUDIO_SAMPLE_RATE_FACTOR != 1.
    //
    // Set QT_MEDIA_PLAYER_AUDIO_SAMPLE_RATE_FACTOR > 1 (e.g. 1.01 - 1.1) to test high buffer
    // loading
    //     or try compensating too fast data consumption by the audio sink.
    // Set QT_MEDIA_PLAYER_AUDIO_SAMPLE_RATE_FACTOR < 1 to test low buffer loading
    //     or try compensating too slow data consumption by the audio sink.

    static const qreal result = []() {
        const auto sampleRateFactorStr =
                qEnvironmentVariable("QT_MEDIA_PLAYER_AUDIO_SAMPLE_RATE_FACTOR");
        bool ok = false;
        const auto result = sampleRateFactorStr.toDouble(&ok);
        return ok ? result : 1.;
    }();

    return result;
}

struct TrivialAudioFrameConverter : AbstractAudioFrameConverter
{
    explicit TrivialAudioFrameConverter(const Frame &frame, QAudioFormat outputFormat,
                                        float playbackRate)
    {
        int sampleRate = qRound(outputFormat.sampleRate() / playbackRate * sampleRateFactor());
        outputFormat.setSampleRate(sampleRate);
        m_converter = createResampler(frame, outputFormat);
    }

    QAudioBuffer convert(AVFrame *frame) override { return m_converter->resample(frame); }

private:
    std::unique_ptr<QFFmpegResampler> m_converter;
};

struct PitchShiftingAudioFrameConverter : AbstractAudioFrameConverter
{
    explicit PitchShiftingAudioFrameConverter(const Frame &frame, QAudioFormat outputFormat,
                                              float playbackRate)
        : m_playbackRate{ playbackRate }
    {
        const QAudioFormat mediaFormat = QFFmpegMediaFormatInfo::audioFormatFromCodecParameters(
                *frame.codecContext()->stream()->codecpar);

        const QAudioFormat floatFormat = [&] {
            QAudioFormat ret = mediaFormat;
            ret.setSampleFormat(QAudioFormat::SampleFormat::Float);
            return ret;
        }();

        m_toPCMDecoder = createResampler(frame, floatFormat); // this is *not* a resampler
        m_stretcher.reset();
        m_pendingFractionalFrames = 0.f;
        m_stretcher.presetDefault(mediaFormat.channelCount(), outputFormat.sampleRate());

        const QAudioFormat pitchCompensatedFormat = [&] {
            QAudioFormat ret = floatFormat;
            ret.setSampleRate(outputFormat.sampleRate());
            return ret;
        }();
        m_toOutputFormatConverter = QFFmpegResampler::createFromInputFormat(pitchCompensatedFormat, outputFormat,
                                                             frame.startTime().get());
    }

    QAudioBuffer convert(AVFrame *frame) override
    {
        using namespace QtPrivate;

        // convert to pcm buffer
        QAudioBuffer wordConverted = m_toPCMDecoder->resample(frame);

        // compute stretch amount
        int mediaFrameCount = wordConverted.frameCount();
        float expectedNumberOfFrames = mediaFrameCount / m_playbackRate + m_pendingFractionalFrames;
        int numberOfFullExpectedFrames = qFloor(expectedNumberOfFrames);
        m_pendingFractionalFrames = expectedNumberOfFrames - numberOfFullExpectedFrames;

        auto timeStretcherOutput = QAudioBuffer{
            numberOfFullExpectedFrames,
            wordConverted.format(),
        };

        // stretch
        m_stretcher.process(
                QAudioBufferDeinterleaveAdaptor<const float>{
                        wordConverted,
                },
                mediaFrameCount,
                QAudioBufferDeinterleaveAdaptor<float>{
                        timeStretcherOutput,
                },
                numberOfFullExpectedFrames);

        // convert to audio output format
        QAudioBuffer outputBuffer = m_toOutputFormatConverter->resample(
                timeStretcherOutput.constData<const char>(), timeStretcherOutput.byteCount());

        return outputBuffer;
    }

private:
    std::unique_ptr<QFFmpegResampler> m_toPCMDecoder;
    signalsmith::stretch::SignalsmithStretch<float> m_stretcher;
    std::unique_ptr<QFFmpegResampler> m_toOutputFormatConverter;
    float m_playbackRate;
    float m_pendingFractionalFrames = 0.f;
};

} // namespace

AbstractAudioFrameConverter::~AbstractAudioFrameConverter() = default;

std::unique_ptr<QFFmpegResampler> createResampler(const Frame &frame,
                                                  const QAudioFormat &outputFormat)
{
    return QFFmpegResampler::createFromCodecContext(frame.codecContext(), outputFormat, frame.startTime().get());
}

std::unique_ptr<AbstractAudioFrameConverter>
makeTrivialAudioFrameConverter(const Frame &frame, QAudioFormat outputFormat, float playbackRate)
{
    return std::make_unique<TrivialAudioFrameConverter>(frame, outputFormat, playbackRate);
}

std::unique_ptr<AbstractAudioFrameConverter>
makePitchShiftingAudioFrameConverter(const Frame &frame, QAudioFormat outputFormat,
                                     float playbackRate)
{
    return std::make_unique<PitchShiftingAudioFrameConverter>(frame, outputFormat, playbackRate);
}

} // namespace QFFmpeg

QT_END_NAMESPACE
