// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegaudiorenderer_p.h"
#include "qaudiosink.h"
#include "qaudiooutput.h"
#include "private/qplatformaudiooutput_p.h"

#include "qffmpegresampler_p.h"
#include "qffmpegmediaformatinfo_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

constexpr std::chrono::microseconds audioSinkBufferSize(100000);

AudioRenderer::AudioRenderer(const TimeController &tc, QAudioOutput *output)
    : Renderer(tc,
               audioSinkBufferSize / 2 /*Ensures kind of "spring" in order to avoid chopy sound*/),
      m_output(output)
{
    if (output) {
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
        m_sink->setBufferSize(m_format.bytesForDuration(audioSinkBufferSize.count()));
        m_ioDevice = m_sink->start();
    }

    if (!m_resampler) {
        initResempler(codec);
    }
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegaudiorenderer_p.cpp"
