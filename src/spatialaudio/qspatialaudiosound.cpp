// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#include "qspatialaudiosound_p.h"

#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/private/qaudio_qspan_support_p.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtSpatialAudio/private/qaudioengine_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>

#include <resonance_audio.h>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QSpatialAudioPrivate {

constexpr int InfiniteLoops = -1;

QSpatialAudioPlaybackState::QSpatialAudioPlaybackState(QAudioBuffer buffer, bool playing, int loops)
    : m_playing(playing), m_loops(loops), m_buffer(std::move(buffer))
{
}

void QSpatialAudioPlaybackState::getBuffer(QSpan<float> output)
{
    using QtMultimediaPrivate::drop;
    using QtMultimediaPrivate::take;
    namespace ranges = QtMultimediaPrivate::ranges;

    if (!m_playing.load(std::memory_order_relaxed)) {
        ranges::fill(output, 0.f);
        return;
    }

    QSpan<const float> wholeSampleBuffer{
        m_buffer.constData<float>(),
        m_buffer.sampleCount(),
    };

    while (!output.empty()) {
        QSpan remainingSamples = drop(wholeSampleBuffer, m_currentSample);
        const QSpan samplesToCopy = take(remainingSamples, output.size());
        ranges::copy(samplesToCopy, output.begin());
        output = drop(output, samplesToCopy.size());
        m_currentSample += samplesToCopy.size();

        if (output.empty())
            return;

        // Reached end of buffer
        Q_ASSERT(m_currentSample == wholeSampleBuffer.size());
        m_currentSample = 0;

        switch (m_loops) {
        case InfiniteLoops:
            break; // Continue looping
        case 0:
            m_playing = false;
            m_currentLoop = 0;
            ranges::fill(output, 0.f);
            return;
        default:
            ++m_currentLoop;
            if (m_currentLoop >= m_loops) {
                m_playing = false;
                m_currentLoop = 0;
                ranges::fill(output, 0.f);
                return;
            }
            break;
        }
    }
}

void QSpatialAudioPlaybackState::resume()
{
    m_playing = true;
}

void QSpatialAudioPlaybackState::setLoops(int loops)
{
    m_loops = loops;
}

QAudioFormat QSpatialAudioPlaybackState::format() const
{
    return m_buffer.format();
}

void QSpatialAudioPlaybackState::pause()
{
    m_playing = false;
}

} // namespace QSpatialAudioPrivate

QSpatialAudioSoundPrivate::QSpatialAudioSoundPrivate(QAudioEngine *engine, int nchannels, int sourceId)
    : nchannels(nchannels), engine(engine), sourceId(sourceId)
{
    auto *ep = QAudioEnginePrivate::get(engine);
    if (!ep)
        return;
    ep->addSound(this);
}

QSpatialAudioSoundPrivate::~QSpatialAudioSoundPrivate()
{
    auto *ep = QAudioEnginePrivate::get(engine);
    if (ep)
        ep->removeSound(this);

    withResonanceApi([&](vraudio::ResonanceAudioApi *api) {
        api->DestroySource(sourceId);
    });
}

void QSpatialAudioSoundPrivate::setVolume(float volume)
{
    m_volume = volume;
    applyVolume();
}

void QSpatialAudioSoundPrivate::setLoops(int loops)
{
    m_loops = loops;
    if (m_playbackState)
        m_playbackState->setLoops(loops);
}

void QSpatialAudioSoundPrivate::setAutoPlay(bool enabled)
{
    m_autoPlay = enabled;
}

void QSpatialAudioSoundPrivate::applyVolume()
{
    withResonanceApi([&](vraudio::ResonanceAudioApi *api) {
        api->SetSourceVolume(sourceId, m_volume);
    });
}

void QSpatialAudioSoundPrivate::play()
{
    switch (m_state) {
    case State::Stopped: {
        m_state = State::Playing;
        if (m_buffer) {
            setState(std::make_shared<QSpatialAudioPlaybackState>(*m_buffer, /*playing=*/true,
                                                                  m_loops));
        }
        return;
    }
    case State::Paused:
    case State::Playing: {
        m_state = State::Playing;
        if (m_playbackState)
            m_playbackState->resume();
        return;
    }
    }
}

void QSpatialAudioSoundPrivate::pause()
{
    switch (m_state) {
    case State::Stopped: {
        m_state = State::Paused;
        setState(std::make_shared<QSpatialAudioPlaybackState>(*m_buffer, /*playing=*/false,
                                                              m_loops));
        return;
    }
    case State::Paused: {
        return;
    }
    case State::Playing: {
        m_state = State::Paused;
        setState(nullptr);

        if (m_playbackState)
            m_playbackState->pause();
        return;
    }
    }
}

void QSpatialAudioSoundPrivate::stop()
{
    switch (m_state) {
    case State::Stopped: {
        return;
    }
    case State::Paused:
    case State::Playing: {
        m_state = State::Stopped;
        setState({});
        return;
    }
    }
}

void QSpatialAudioSoundPrivate::loadUrl(const QUrl &url)
{
    m_url = url;

    auto *ep = QAudioEnginePrivate::get(engine);
    if (!ep)
        return;

    m_loadFuture.cancelChain();
    m_sample = {};

    setState(nullptr);

    m_loadFuture = QSampleCache::instance()
                           ->requestSampleFuture(m_sourceResolver->resolve(url), ep->sampleRate())
                           .then(q_ptr, [this](SharedSamplePtr sample) {
        if (!sample) {
            qWarning() << "QAmbientSound: cannot load file";
            return;
        }

        // Build QAudioBuffer with the correct channel layout for this source.
        // QSampleCache resampled to engineRate but preserves native channels.
        // Adapt channel count here: mono<->stereo simple mix.
        const QAudioFormat &srcFmt = sample->format();
        const int srcChannels = srcFmt.channelCount();

        QByteArray pcmData;
        QAudioFormat outFmt = srcFmt;
        outFmt.setChannelCount(nchannels);
        outFmt.setChannelConfig(nchannels == 2 ? QAudioFormat::ChannelConfigStereo
                                               : QAudioFormat::ChannelConfigMono);

        using namespace QAudioHelperInternal;

        if (srcChannels == nchannels) {
            pcmData = sample->data();

            m_sample = std::move(sample); // keep the sample in the QSampleCache as long as we live
        } else if (srcChannels == 1 && nchannels == 2) {
            qWarning() << "QAmbientSound: upmixing mono source to stereo";
            pcmData = upmixMonoToStereo(sample->dataAsFloatSpan(), UpmixScaling::Duplicate);
        } else if (srcChannels == 2 && nchannels == 1) {
            qWarning() << "QAmbientSound: downmixing stereo source to mono";
            pcmData = downmixStereoToMono(sample->dataAsFloatSpan(), DownmixScaling::Average);
        } else {
            qWarning() << "QAmbientSound: unsupported channel count"
                       << srcChannels << "- rejecting file";
            return;
        }

        m_buffer = QAudioBuffer{
            pcmData,
            outFmt,
        };

        const bool startingPlayback = m_autoPlay || m_state != State::Stopped;
        if (!startingPlayback)
            return;
        const bool startPaused = (m_state == State::Paused) && !m_autoPlay;
        setState(std::make_shared<QSpatialAudioPlaybackState>(*m_buffer, /*playing=*/!startPaused,
                                                              m_loops));

        m_state = startPaused ? State::Paused : State::Playing;
    });
}

vraudio::ResonanceAudioApi *QSpatialAudioSoundPrivate::getAPI()
{
    auto *ep = QAudioEnginePrivate::get(engine);
    if (!ep)
        return nullptr;
    return ep->resonanceAudio->api.get();
}

void QSpatialAudioSoundPrivate::setState(SharedPlaybackState state)
{
    auto *ep = QAudioEnginePrivate::get(engine);
    if (ep)
        ep->setSoundPlaybackData(this, state);
    m_playbackState = std::move(state);
}

QT_END_NAMESPACE
