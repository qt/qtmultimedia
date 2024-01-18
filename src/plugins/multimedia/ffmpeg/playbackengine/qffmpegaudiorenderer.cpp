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
using namespace std::chrono;

namespace {
constexpr auto DesiredBufferTime = 110000us;
constexpr auto MinDesiredBufferTime = 22000us;
constexpr auto MaxDesiredBufferTime = 64000us;
constexpr auto MinDesiredFreeBufferTime = 10000us;

// It might be changed with #ifdef, as on Linux, QPulseAudioSink has quite unstable timings,
// and it needs much more time to make sure that the buffer is overloaded.
constexpr auto BufferLoadingMeasureTime = 400ms;

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
    : Renderer(tc), m_output(output)
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

    if (!m_ioDevice || !m_resampler)
        return {};

    Q_ASSERT(m_sink);

    auto firstFrameFlagGuard = qScopeGuard([&]() { m_firstFrame = false; });

    const SynchronizationStamp syncStamp{ m_sink->state(), m_sink->bytesFree(),
                                          m_bufferedData.offset, Clock::now() };

    if (!m_bufferedData.isValid()) {
        if (!frame.isValid()) {
            if (std::exchange(m_drained, true))
                return {};

            const auto time = bufferLoadingTime(syncStamp);

            qCDebug(qLcAudioRenderer) << "Draining AudioRenderer, time:" << time;

            return { time.count() == 0, time };
        }

        m_bufferedData = { m_resampler->resample(frame.avFrame()) };
    }

    if (m_bufferedData.isValid()) {
        // synchronize after "QIODevice::write" to deliver audio data to the sink ASAP.
        auto syncGuard = qScopeGuard([&]() { updateSynchronization(syncStamp, frame); });

        const auto bytesWritten = m_ioDevice->write(m_bufferedData.data(), m_bufferedData.size());

        m_bufferedData.offset += bytesWritten;

        if (m_bufferedData.size() <= 0) {
            m_bufferedData = {};

            return {};
        }

        const auto remainingDuration = durationForBytes(m_bufferedData.size());

        return { false,
                 std::min(remainingDuration + DurationBias, m_timings.actualBufferDuration / 2) };
    }

    return {};
}

void AudioRenderer::onPlaybackRateChanged()
{
    m_resampler.reset();
}

int AudioRenderer::timerInterval() const
{
    constexpr auto MaxFixableInterval = 50; // ms

    const auto interval = Renderer::timerInterval();

    if (m_firstFrame || !m_sink || m_sink->state() != QAudio::IdleState
        || interval > MaxFixableInterval)
        return interval;

    return 0;
}

void AudioRenderer::onPauseChanged()
{
    m_firstFrame = true;
    Renderer::onPauseChanged();
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
    m_resampler = std::make_unique<QFFmpegResampler>(codec, resamplerFormat);
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
    m_deviceChanged = false;
    m_timings = {};
    m_bufferLoadingInfo = {};
}

void AudioRenderer::updateOutput(const Codec *codec)
{
    if (m_deviceChanged) {
        freeOutput();
        m_format = {};
        m_resampler.reset();
    }

    if (!m_output)
        return;

    if (!m_format.isValid()) {
        m_format =
                QFFmpegMediaFormatInfo::audioFormatFromCodecParameters(codec->stream()->codecpar);
        m_format.setChannelConfig(m_output->device().channelConfiguration());
    }

    if (!m_sink) {
        // Insert a delay here to test time offset synchronization, e.g. QThread::sleep(1)
        m_sink = std::make_unique<QAudioSink>(m_output->device(), m_format);
        updateVolume();
        m_sink->setBufferSize(m_format.bytesForDuration(DesiredBufferTime.count()));
        m_ioDevice = m_sink->start();
        m_firstFrame = true;

        connect(m_sink.get(), &QAudioSink::stateChanged, this,
                &AudioRenderer::onAudioSinkStateChanged);

        m_timings.actualBufferDuration = durationForBytes(m_sink->bufferSize());
        m_timings.maxSoundDelay = qMin(MaxDesiredBufferTime,
                                       m_timings.actualBufferDuration - MinDesiredFreeBufferTime);
        m_timings.minSoundDelay = MinDesiredBufferTime;

        Q_ASSERT(DurationBias < m_timings.minSoundDelay
                 && m_timings.maxSoundDelay < m_timings.actualBufferDuration);
    }

    if (!m_resampler) {
        initResempler(codec);
    }
}

void AudioRenderer::updateSynchronization(const SynchronizationStamp &stamp, const Frame &frame)
{
    if (!frame.isValid())
        return;

    Q_ASSERT(m_sink);

    const auto bufferLoadingTime = this->bufferLoadingTime(stamp);
    const auto currentFrameDelay = frameDelay(frame, stamp.timePoint);
    const auto writtenTime = durationForBytes(stamp.bufferBytesWritten);
    const auto soundDelay = currentFrameDelay + bufferLoadingTime - writtenTime;

    auto synchronize = [&](microseconds fixedDelay, microseconds targetSoundDelay) {
        // TODO: investigate if we need sample compensation here

        changeRendererTime(fixedDelay - targetSoundDelay);
        if (qLcAudioRenderer().isDebugEnabled()) {
            // clang-format off
            qCDebug(qLcAudioRenderer)
                << "Change rendering time:"
                << "\n  First frame:" << m_firstFrame
                << "\n  Delay (frame+buffer-written):" << currentFrameDelay << "+"
                                                       << bufferLoadingTime << "-"
                                                       << writtenTime << "="
                                                       << soundDelay
                << "\n  Fixed delay:" << fixedDelay
                << "\n  Target delay:" << targetSoundDelay
                << "\n  Buffer durations (min/max/limit):" << m_timings.minSoundDelay
                                                           << m_timings.maxSoundDelay
                                                           << m_timings.actualBufferDuration
                << "\n  Audio sink state:" << stamp.audioSinkState;
            // clang-format on
        }
    };

    const auto loadingType = soundDelay > m_timings.maxSoundDelay ? BufferLoadingInfo::High
                           : soundDelay < m_timings.minSoundDelay ? BufferLoadingInfo::Low
                                                                  : BufferLoadingInfo::Moderate;

    if (loadingType != m_bufferLoadingInfo.type) {
        //        qCDebug(qLcAudioRenderer) << "Change buffer loading type:" <<
        //        m_bufferLoadingInfo.type
        //                                  << "->" << loadingType << "soundDelay:" << soundDelay;
        m_bufferLoadingInfo = { loadingType, stamp.timePoint, soundDelay };
    }

    if (loadingType != BufferLoadingInfo::Moderate) {
        const auto isHigh = loadingType == BufferLoadingInfo::High;
        const auto shouldHandleIdle = stamp.audioSinkState == QAudio::IdleState && !isHigh;

        auto &fixedDelay = m_bufferLoadingInfo.delay;

        fixedDelay = shouldHandleIdle ? soundDelay
                   : isHigh           ? qMin(soundDelay, fixedDelay)
                                      : qMax(soundDelay, fixedDelay);

        if (stamp.timePoint - m_bufferLoadingInfo.timePoint > BufferLoadingMeasureTime
            || (m_firstFrame && isHigh) || shouldHandleIdle) {
            const auto targetDelay = isHigh
                    ? (m_timings.maxSoundDelay + m_timings.minSoundDelay) / 2
                    : m_timings.minSoundDelay + DurationBias;

            synchronize(fixedDelay, targetDelay);
            m_bufferLoadingInfo = { BufferLoadingInfo::Moderate, stamp.timePoint, targetDelay };
        }
    }
}

microseconds AudioRenderer::bufferLoadingTime(const SynchronizationStamp &syncStamp) const
{
    Q_ASSERT(m_sink);

    if (syncStamp.audioSinkState == QAudio::IdleState)
        return microseconds(0);

    const auto bytes = qMax(m_sink->bufferSize() - syncStamp.audioSinkBytesFree, 0);

#ifdef Q_OS_ANDROID
    // The hack has been added due to QAndroidAudioSink issues (QTBUG-118609).
    // The method QAndroidAudioSink::bytesFree returns 0 or bufferSize, intermediate values are not
    // available now; to be fixed.
    if (bytes == 0)
        return m_timings.minSoundDelay + MinDesiredBufferTime;
#endif

    return durationForBytes(bytes);
}

void AudioRenderer::onAudioSinkStateChanged(QAudio::State state)
{
    if (state == QAudio::IdleState && !m_firstFrame)
        scheduleNextStep();
}

microseconds AudioRenderer::durationForBytes(qsizetype bytes) const
{
    return microseconds(m_format.durationForBytes(static_cast<qint32>(bytes)));
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegaudiorenderer_p.cpp"
