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

// actual playback rate chang during the soft compensation
constexpr qreal CompensationAngleFactor = 0.01;
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
        if (!frame.isValid())
            return {};

        updateSampleCompensation(frame);
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

        return Renderer::RenderingResult{ std::chrono::microseconds(m_format.durationForBytes(
                m_sink->bufferSize() / 2 + m_bufferedData.byteCount() - m_bufferWritten)) };
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
    resamplerFormat.setSampleRate(qRound(m_format.sampleRate() / playbackRate()));
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
        m_sink = std::make_unique<QAudioSink>(m_output->device(), m_format);
        updateVolume();
        m_sink->setBufferSize(m_format.bytesForDuration(AudioSinkBufferTime.count()));
        m_ioDevice = m_sink->start();
    }

    if (!m_resampler) {
        initResempler(codec);
    }
}

void AudioRenderer::updateSampleCompensation(const Frame &currentFrame)
{
    // Currently we use "soft" compensation with a positive delta
    // for slight increasing of the buffer loading if it's too low
    // in order to avoid choppy sound. If the bufer loading is too low,
    // QAudioSink sometimes utilizes all written data earlier than new data delivered,
    // that produces sound clicks (most hearable on Windows).
    //
    // TODO:
    // 1. Probably, use "hard" compensation (inject silence) on the start and after pause
    // 2. Probably, use "soft" compensation with a negative delta for decreasing buffer loading.
    //    Currently, we use renderers synchronizations, but the suggested approach might imrove
    //    the sound rendering and avoid changing of the current rendering position.

    Q_ASSERT(m_sink);
    Q_ASSERT(m_resampler);
    Q_ASSERT(currentFrame.isValid());

    const auto loadBufferTime = AudioSinkBufferTime
            * qMax(m_sink->bufferSize() - m_sink->bytesFree(), 0) / m_sink->bufferSize();

    constexpr auto frameDelayThreshold = MinDesiredBufferTime / 2;
    const bool positiveCompensationNeeded = loadBufferTime < MinDesiredBufferTime
            && !m_resampler->isSampleCompensationActive()
            && frameDelay(currentFrame) < frameDelayThreshold;

    if (positiveCompensationNeeded) {
        constexpr auto targetBufferTime = MinDesiredBufferTime * 2;
        const auto delta = m_format.sampleRate() * (targetBufferTime - loadBufferTime) / 1s;
        const auto interval = delta / CompensationAngleFactor;

        qCDebug(qLcAudioRenderer) << "Enable audio sample speed up compensation. Delta:" << delta
                                  << "Interval:" << interval
                                  << "SampleRate:" << m_format.sampleRate()
                                  << "SinkLoadTime(us):" << loadBufferTime.count()
                                  << "SamplesProcessed:" << m_resampler->samplesProcessed();

        m_resampler->setSampleCompensation(static_cast<qint32>(delta),
                                           static_cast<quint32>(interval));
    }
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegaudiorenderer_p.cpp"
