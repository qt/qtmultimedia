// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegaudiorenderer_p.h"
#include "qaudiosink.h"
#include "qaudiooutput.h"
#include "private/qplatformaudiooutput_p.h"
#include <QtCore/qloggingcategory.h>

#include "qffmpegresampler_p.h"
#include "qffmpegmediaformatinfo_p.h"

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcAudioRenderer, "qt.multimedia.ffmpeg.audiorenderer");

namespace QFFmpeg {

using namespace std::chrono_literals;

namespace {
constexpr auto AudioSinkBufferTime = 100000us;
constexpr auto MinDesiredBufferTime = AudioSinkBufferTime / 10;
constexpr auto MaxDesiredBufferTime = 6 * AudioSinkBufferTime / 10;
constexpr auto SampleCompenationOffset = AudioSinkBufferTime / 10;

// actual playback rate chang during the soft compensation
constexpr qreal CompensationAngleFactor = 0.01;

constexpr auto DurationBias = 2ms; // avoids extra timer events

qreal sampleRateFactor() {
    // Test purposes:
    //
    // The env var describes a factor for the sample rate of
    // audio data that we feed to the audio sink.
    //
    // In some cases  audio sink might consume data slightly slower or faster than expected;
    // even though the synchronization in the audio renderer is supposed to handle it,
    // it makes sense to experiment with QT_MEDIA_PLAYER_AUDIO_SAMPLE_RATE_FACTOR != 1.
    //
    // Set QT_MEDIA_PLAYER_AUDIO_SAMPLE_RATE_FACTOR > 1 (e.g. 1.01 - 1.1) to test high buffer loading
    //     or try compensating too fast data consumption by the audio sink.
    // Set QT_MEDIA_PLAYER_AUDIO_SAMPLE_RATE_FACTOR < 1 to test low buffer loading
    //     or try compensating too slow data consumption by the audio sink.


    static const qreal result = []() {
        const auto sampleRateFactorStr = qEnvironmentVariable("QT_MEDIA_PLAYER_AUDIO_SAMPLE_RATE_FACTOR");
        bool ok = false;
        const auto result = sampleRateFactorStr.toDouble(&ok);
        return ok ? result : 1.;
    }();

    return result;
}
} // namespace

AudioRenderer::AudioRenderer(const TimeController &tc, QAudioOutput *output)
    : Renderer(tc, MinDesiredBufferTime), m_output(output)
{
    if (output) {
        // TODO: implement the signals in QPlatformAudioOutput and connect to them, QTBUG-112294
        connect(output, &QAudioOutput::deviceChanged, this, &AudioRenderer::onDeviceChanged);
        connect(output, &QAudioOutput::volumeChanged, this, &AudioRenderer::updateVolume);
        connect(output, &QAudioOutput::mutedChanged, this, &AudioRenderer::updateVolume);
    }
}

void AudioRenderer::setOutput(QAudioOutput *output)
{
    setOutputInternal(m_output, output, [this](QAudioOutput *) { onDeviceChanged(); });
}

AudioRenderer::~AudioRenderer()
{
    freeOutput();
}

void AudioRenderer::updateVolume()
{
    if (m_sink)
        m_sink->setVolume(m_output->isMuted() ? 0.f : m_output->volume());
}

void AudioRenderer::onDeviceChanged()
{
    m_deviceChanged = true;
}

Renderer::RenderingResult AudioRenderer::renderInternal(Frame frame)
{
    if (frame.isValid())
        updateOutput(frame.codec());

    if (!m_sink || !m_resampler || !m_ioDevice)
        return {};

    if (!m_bufferedData.isValid()) {
        if (!frame.isValid()) {
            if (m_drained)
                return {};

            m_drained = true;
            const auto time = currentBufferLoadingTime();

            qCDebug(qLcAudioRenderer) << "Draining AudioRenderer, time:" << time;

            return { time.count() == 0, time };
        }

        updateSynchronization(frame);
        m_bufferedData = m_resampler->resample(frame.avFrame());
        m_bufferWritten = 0;
    }

    if (m_bufferedData.isValid()) {
        auto bytesWritten = m_ioDevice->write(m_bufferedData.constData<char>() + m_bufferWritten,
                                              m_bufferedData.byteCount() - m_bufferWritten);

        m_bufferWritten += bytesWritten;

        if (m_bufferWritten >= m_bufferedData.byteCount()) {
            m_bufferedData = {};
            m_bufferWritten = 0;

            return {};
        }

        const auto remainingDuration = std::chrono::microseconds(
                m_format.durationForBytes(m_bufferedData.byteCount() - m_bufferWritten));

        return { false, std::min(remainingDuration + DurationBias, AudioSinkBufferTime / 2) };
    }

    return {};
}

void AudioRenderer::onPlaybackRateChanged()
{
    m_resampler.reset();
}

void AudioRenderer::initResempler(const Codec *codec)
{
    // We recreate resampler whenever format is changed

    /*    AVSampleFormat requiredFormat =
    QFFmpegMediaFormatInfo::avSampleFormat(m_format.sampleFormat());

    #if QT_FFMPEG_OLD_CHANNEL_LAYOUT
        qCDebug(qLcAudioRenderer) << "init resampler" << requiredFormat
                                  << codec->stream()->codecpar->channels;
    #else
        qCDebug(qLcAudioRenderer) << "init resampler" << requiredFormat
                                  << codec->stream()->codecpar->ch_layout.nb_channels;
    #endif
    */

    auto resamplerFormat = m_format;
    resamplerFormat.setSampleRate(
            qRound(m_format.sampleRate() / playbackRate() * sampleRateFactor()));
    m_resampler = std::make_unique<Resampler>(codec, resamplerFormat);
}

void AudioRenderer::freeOutput()
{
    qCDebug(qLcAudioRenderer) << "Free audio output";
    if (m_sink) {
        m_sink->reset();

        // TODO: inestigate if it's enough to reset the sink without deleting
        m_sink.reset();
    }

    m_ioDevice = nullptr;

    m_bufferedData = {};
    m_bufferWritten = 0;
    m_deviceChanged = false;
}

void AudioRenderer::updateOutput(const Codec *codec)
{
    if (m_deviceChanged) {
        freeOutput();
        m_format = {};
        m_resampler.reset();
    }

    if (!m_output) {
        return;
    }

    if (!m_format.isValid()) {
        m_format =
                QFFmpegMediaFormatInfo::audioFormatFromCodecParameters(codec->stream()->codecpar);
        m_format.setChannelConfig(m_output->device().channelConfiguration());
    }

    if (!m_sink) {
        // Insert a delay here to test time offset synchronization, e.g. QThread::sleep(1)
        m_sink = std::make_unique<QAudioSink>(m_output->device(), m_format);
        updateVolume();
        m_sink->setBufferSize(m_format.bytesForDuration(AudioSinkBufferTime.count()));
        m_ioDevice = m_sink->start();
    }

    if (!m_resampler) {
        initResempler(codec);
    }
}

void AudioRenderer::updateSynchronization(const Frame &currentFrame)
{
    // Currently we use "soft" compensation with a positive/negative delta
    // for slight increasing/decreasing of the sound delay (roughly equal to buffer loading on a
    // normal playng) if it's out of the range [min; max]. If the delay more than
    // AudioSinkBufferTime we synchronize rendering time to ensure free buffer size and than
    // decrease it more with "soft" synchronization.
    //
    // TODO:
    // 1. Probably, use "hard" compensation (inject silence) on the start and after pause
    // 2. Make a delay as much stable as possible. For this aim, we should make
    //    CompensationAngleFactor flexible, and all the time to the narrower range of desired
    //    QAudioSink loading time.

    Q_ASSERT(m_resampler);
    Q_ASSERT(currentFrame.isValid());

    const auto bufferLoadingTime = currentBufferLoadingTime();
    const auto currentFrameDelay = frameDelay(currentFrame);
    auto soundDelay = currentFrameDelay + bufferLoadingTime;

    const auto activeCompensationDelta = m_resampler->activeSampleCompensationDelta();

    if (soundDelay > AudioSinkBufferTime) {
        const auto targetSoundDelay = (AudioSinkBufferTime + MaxDesiredBufferTime) / 2;
        changeRendererTime(soundDelay - targetSoundDelay);
        qCDebug(qLcAudioRenderer) << "Change rendering time: Audio time offset."
                                  << "Prev sound delay:" << soundDelay.count()
                                  << "Target sound delay:" << targetSoundDelay.count()
                                  << "New actual sound delay:"
                                  << (frameDelay(currentFrame) + bufferLoadingTime).count();

        soundDelay = targetSoundDelay;
    }

    constexpr auto AvgDesiredBufferTime = (MinDesiredBufferTime + MaxDesiredBufferTime) / 2;

    std::optional<int> newCompensationSign;
    if (soundDelay < MinDesiredBufferTime && activeCompensationDelta <= 0)
        newCompensationSign = 1;
    else if (soundDelay > MaxDesiredBufferTime && activeCompensationDelta >= 0)
        newCompensationSign = -1;
    else if ((soundDelay <= AvgDesiredBufferTime && activeCompensationDelta < 0)
             || (soundDelay >= AvgDesiredBufferTime && activeCompensationDelta > 0))
        newCompensationSign = 0;

    // qDebug() << soundDelay.count() << bufferLoadingTime.count();

    if (newCompensationSign) {
        const auto target = *newCompensationSign == 0 ? soundDelay
                : *newCompensationSign > 0 ? MinDesiredBufferTime + SampleCompenationOffset
                                           : MaxDesiredBufferTime - SampleCompenationOffset;
        const auto delta = m_format.sampleRate() * (target - soundDelay) / 1s;
        const auto interval = std::abs(delta) / CompensationAngleFactor;

        qDebug(qLcAudioRenderer) << "Set audio sample compensation. Delta (samples and us):"
                                 << delta << (target - soundDelay).count()
                                 << "PrevDelta:" << activeCompensationDelta
                                 << "Interval:" << interval
                                 << "SampleRate:" << m_format.sampleRate()
                                 << "Delay(us):" << soundDelay.count()
                                 << "SamplesProcessed:" << m_resampler->samplesProcessed();

        m_resampler->setSampleCompensation(static_cast<qint32>(delta),
                                           static_cast<quint32>(interval));
    }
}

std::chrono::microseconds AudioRenderer::currentBufferLoadingTime() const
{
    Q_ASSERT(m_sink);

    return AudioSinkBufferTime * qMax(m_sink->bufferSize() - m_sink->bytesFree(), 0)
            / m_sink->bufferSize();
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegaudiorenderer_p.cpp"
