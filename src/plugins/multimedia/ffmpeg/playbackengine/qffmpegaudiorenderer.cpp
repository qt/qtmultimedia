// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegaudiorenderer_p.h"

#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/qaudiooutput.h>
#include <QtMultimedia/qaudiobufferoutput.h>
#include <QtMultimedia/private/qaudiobuffer_support_p.h>
#include <QtMultimedia/private/qplatformaudiooutput_p.h>

#include <QtCore/qloggingcategory.h>

#include "qffmpegaudioframeconverter_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegresampler_p.h"

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcAudioRenderer, "qt.multimedia.ffmpeg.audiorenderer");

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

QAudioFormat audioFormatFromFrame(const Frame &frame)
{
    return QFFmpegMediaFormatInfo::audioFormatFromCodecParameters(
            *frame.codecContext()->stream()->codecpar);
}

} // namespace

AudioRenderer::AudioRenderer(const TimeController &tc, QAudioOutput *output,
                             QAudioBufferOutput *bufferOutput, bool pitchCompensation)
    : Renderer(tc),
      m_output(output),
      m_bufferOutput(bufferOutput),
      m_pitchCompensation(pitchCompensation)
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

void AudioRenderer::setOutput(QAudioBufferOutput *bufferOutput)
{
    setOutputInternal(m_bufferOutput, bufferOutput,
                      [this](QAudioBufferOutput *) { m_bufferOutputChanged = true; });
}

void AudioRenderer::setPitchCompensation(bool enabled)
{
    QMetaObject::invokeMethod(this, [this, enabled] {
        if (m_pitchCompensation == enabled)
            return;

        m_pitchCompensation = enabled;
        m_audioFrameConverter.reset();
    });
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
        updateOutputs(frame);

    // push to sink first in order not to waste time on resampling
    // for QAudioBufferOutput
    const RenderingResult result = pushFrameToOutput(frame);

    if (m_lastFramePushDone)
        pushFrameToBufferOutput(frame);
    // else // skip pushing the same data to QAudioBufferOutput

    m_lastFramePushDone = result.done;

    return result;
}

AudioRenderer::RenderingResult AudioRenderer::pushFrameToOutput(const Frame &frame)
{
    if (!m_ioDevice || !m_audioFrameConverter)
        return {};

    Q_ASSERT(m_sink);

    auto firstFrameFlagGuard = qScopeGuard([&]() { m_firstFrameToSink = false; });

    const SynchronizationStamp syncStamp{ m_sink->state(), m_sink->bytesFree(),
                                          m_bufferedData.offset, SteadyClock::now() };

    if (!m_bufferedData.isValid()) {
        if (!frame.isValid()) {
            if (std::exchange(m_drained, true))
                return {};

            const auto time = bufferLoadingTime(syncStamp);

            qCDebug(qLcAudioRenderer) << "Draining AudioRenderer, time:" << time;

            return { time.count() == 0, time };
        }

        m_bufferedData = {
            m_audioFrameConverter->convert(frame.avFrame()),
        };
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

void AudioRenderer::pushFrameToBufferOutput(const Frame &frame)
{
    if (!m_bufferOutput)
        return;

    if (frame.isValid()) {
        Q_ASSERT(m_bufferOutputResampler);

        // TODO: get buffer from m_bufferedData if resample formats are equal
        QAudioBuffer buffer = m_bufferOutputResampler->resample(frame.avFrame());
        emit m_bufferOutput->audioBufferReceived(buffer);
    } else {
        emit m_bufferOutput->audioBufferReceived({});
    }
}

void AudioRenderer::onPlaybackRateChanged()
{
    m_audioFrameConverter.reset();
}

std::chrono::milliseconds AudioRenderer::timerInterval() const
{
    constexpr auto MaxFixableInterval = 50ms;

    const auto interval = Renderer::timerInterval();

    if (m_firstFrameToSink || !m_sink || m_sink->state() != QAudio::IdleState
        || interval > MaxFixableInterval)
        return interval;

    return 0ms;
}

void AudioRenderer::onPauseChanged()
{
    m_firstFrameToSink = true;
    Renderer::onPauseChanged();
}

void AudioRenderer::initAudioFrameConverter(const Frame &frame)
{
    // We recreate the frame converter whenever format or playback rate is changed
    if (!m_pitchCompensation || qFuzzyCompare(playbackRate(), 1.0f)) {
        m_audioFrameConverter = makeTrivialAudioFrameConverter(frame, m_sinkFormat, playbackRate());
    } else {
        m_audioFrameConverter =
                makePitchShiftingAudioFrameConverter(frame, m_sinkFormat, playbackRate());
    }
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
    m_sinkFormat = {};
    m_timings = {};
    m_bufferLoadingInfo = {};
}

void AudioRenderer::updateOutputs(const Frame &frame)
{
    if (m_deviceChanged) {
        freeOutput();
        m_audioFrameConverter.reset();
    }

    if (m_bufferOutput) {
        if (m_bufferOutputChanged) {
            m_bufferOutputChanged = false;
            m_bufferOutputResampler.reset();
        }

        if (!m_bufferOutputResampler) {
            QAudioFormat outputFormat = m_bufferOutput->format();
            if (!outputFormat.isValid())
                outputFormat = audioFormatFromFrame(frame);
            m_bufferOutputResampler = createResampler(frame, outputFormat);
        }
    }

    if (!m_output)
        return;

    if (!m_sinkFormat.isValid()) {
        m_sinkFormat = audioFormatFromFrame(frame);
        m_sinkFormat.setChannelConfig(m_output->device().channelConfiguration());
    }

    if (!m_sink) {
        // Insert a delay here to test time offset synchronization, e.g. QThread::sleep(1)
        m_sink = std::make_unique<QAudioSink>(m_output->device(), m_sinkFormat);
        updateVolume();
        m_sink->setBufferSize(m_sinkFormat.bytesForDuration(DesiredBufferTime.count()));
        m_ioDevice = m_sink->start();
        m_firstFrameToSink = true;

        connect(m_sink.get(), &QAudioSink::stateChanged, this,
                &AudioRenderer::onAudioSinkStateChanged);

        m_timings.actualBufferDuration = durationForBytes(m_sink->bufferSize());
        m_timings.maxSoundDelay = qMin(MaxDesiredBufferTime,
                                       m_timings.actualBufferDuration - MinDesiredFreeBufferTime);
        m_timings.minSoundDelay = MinDesiredBufferTime;

        Q_ASSERT(DurationBias < m_timings.minSoundDelay
                 && m_timings.maxSoundDelay < m_timings.actualBufferDuration);
    }

    if (!m_audioFrameConverter)
        initAudioFrameConverter(frame);
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
                << "\n  First frame:" << m_firstFrameToSink
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
            || (m_firstFrameToSink && isHigh) || shouldHandleIdle) {
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
    if (state == QAudio::IdleState && !m_firstFrameToSink && !m_deviceChanged)
        scheduleNextStep();
}

microseconds AudioRenderer::durationForBytes(qsizetype bytes) const
{
    return microseconds(m_sinkFormat.durationForBytes(static_cast<qint32>(bytes)));
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegaudiorenderer_p.cpp"
