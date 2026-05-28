// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#include "qaudioengine_withplayer_p.h"

#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/q_pmr_emulation_p.h>
#include <QtMultimedia/private/qmemory_resource_tlsf_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qrtaudioengine_p.h>
#include <QtSpatialAudio/private/qambientsound_p.h>
#include <QtSpatialAudio/private/qambisonicdecoder_p.h>
#include <QtSpatialAudio/private/qspatialsound_p.h>

#include <resonance_audio.h>

QT_BEGIN_NAMESPACE

enum class SourceId : int { };

class QResonanceAudioPlayer : public QtMultimediaPrivate::QRtAudioEngineVoice
{
public:
    using SharedPlaybackState = QAudioEnginePrivate::SharedPlaybackState;

    QResonanceAudioPlayer(QAudioEngineWithPlayer *player, VoiceId id);

    /// QRtAudioEngineVoice overrides
    VoicePlayResult play(QSpan<float>) noexcept Q_DECL_NONBLOCKING_FUNCTION override;
    bool isActive() noexcept Q_DECL_NONBLOCKING_FUNCTION override { return true; }
    const QAudioFormat &format() noexcept override { return m_format; }

    void addSound(SourceId, int numberOfChannels, SharedPlaybackState = nullptr);
    void setSoundPlaybackData(SourceId, SharedPlaybackState);
    void removeSound(SourceId);

    void setPaused(bool);
    void setOutputMode(QAudioEngine::OutputMode mode);

private:
    void processSlice(QSpan<float> hostBuffer);
    void processSliceFillResonanceBuffers();
    void processSliceProcessSurroundMode(QSpan<float>);
    void processSliceProcessWithoutReverb(QSpan<float>);

    constexpr static auto framesPerBuffer = QAudioEnginePrivate::framesPerBuffer;
    QAudioEngine::OutputMode m_outputMode;
    const std::shared_ptr<vraudio::ResonanceAudio> m_resonanceAudio;
    const QAudioFormat m_format;
    QAmbisonicDecoder m_ambisonicDecoder;
    bool m_paused = false;

    struct SoundState
    {
        SharedPlaybackState playbackState;
        int numberOfChannels;
    };

    static constexpr size_t poolSize =
            sizeof(QtMultimediaPrivate::pmr::map<SourceId, SoundState>::node_type)
                    * 512 // space for 512 sounds
            + sizeof(float) * qToUnderlying(framesPerBuffer) * 32 // leftover buffer for 32 channels
            + sizeof(float) * qToUnderlying(framesPerBuffer) * 32 // temporary slice buffer
            + 16384; // some extra space for internal fragmentation and allocator metadata
    QtMultimediaPrivate::QTlsfMemoryResource m_rtMemoryPool{ poolSize };

    QtMultimediaPrivate::pmr::map<SourceId, SoundState> m_sounds{ &m_rtMemoryPool };
    QtMultimediaPrivate::pmr::vector<float> m_leftoverBuffer{ &m_rtMemoryPool };
};

QResonanceAudioPlayer::QResonanceAudioPlayer(QAudioEngineWithPlayer *player, VoiceId id)
    : QRtAudioEngineVoice(id),
      m_outputMode(player->outputMode()),
      m_resonanceAudio(player->resonanceAudio),
      m_format(player->audioFormat()),
      m_ambisonicDecoder{
          QAmbisonicDecoder::HighQuality,
          m_format.sampleRate(),
          m_format.channelCount(),
          m_format.channelConfig(),
      }
{
}

QtMultimediaPrivate::QRtAudioEngineVoice::VoicePlayResult
QResonanceAudioPlayer::play(QSpan<float> outputBuffer) noexcept Q_DECL_NONBLOCKING_FUNCTION
{
    using namespace QtMultimediaPrivate;
    namespace ranges = QtMultimediaPrivate::ranges;

    if (m_paused) {
        ranges::fill(outputBuffer, 0);
        return VoicePlayResult::Playing;
    }

    const size_t samplesPerSlice = m_format.channelCount() * qToUnderlying(framesPerBuffer);

    while (!outputBuffer.empty()) {
        if (!m_leftoverBuffer.empty()) {
            // we have leftovers
            QSpan leftoverSpanToCopy = take(QSpan(m_leftoverBuffer), outputBuffer.size());
            ranges::copy(leftoverSpanToCopy, outputBuffer.begin());
            m_leftoverBuffer.erase(m_leftoverBuffer.begin(),
                                   m_leftoverBuffer.begin() + leftoverSpanToCopy.size());
            outputBuffer = drop(outputBuffer, leftoverSpanToCopy.size());
            continue;
        }

        Q_ASSERT(m_leftoverBuffer.empty());

        auto sliceBuffer = take(outputBuffer, samplesPerSlice);
        size_t sliceBufferSize = sliceBuffer.size();

        if (sliceBufferSize == samplesPerSlice) {
            // normal case: we have enough space
            processSlice(sliceBuffer);
            outputBuffer = drop(outputBuffer, samplesPerSlice);
            continue;
        } else {
            // not enough space to fill a whole slice.
            pmr::vector<float> sliceBuffer{
                samplesPerSlice,
                0.0f,
                pmr::vector<float>::allocator_type{ m_leftoverBuffer.get_allocator().resource() },
            };
            processSlice(sliceBuffer);

            QSpan sliceToOutput = take(QSpan(sliceBuffer), sliceBufferSize);
            QSpan sliceToKeep = drop(QSpan(sliceBuffer), sliceBufferSize);
            ranges::copy(sliceToOutput, outputBuffer.begin());
            m_leftoverBuffer.assign(sliceToKeep.begin(), sliceToKeep.end());

            outputBuffer = drop(outputBuffer, sliceToOutput.size());
        }
    }

    return VoicePlayResult::Playing;
}

void QResonanceAudioPlayer::addSound(SourceId id, int numberOfChannels, SharedPlaybackState state)
{
    auto [_, inserted] = m_sounds.insert_or_assign(id,
                                                   SoundState{
                                                           std::move(state),
                                                           numberOfChannels,
                                                   });
    Q_ASSERT(inserted);
}

void QResonanceAudioPlayer::setSoundPlaybackData(SourceId id, SharedPlaybackState state)
{
    auto it = m_sounds.find(id);
    Q_ASSERT(it != m_sounds.end());
    it->second.playbackState = std::move(state);
}

void QResonanceAudioPlayer::removeSound(SourceId sound)
{
    m_sounds.erase(sound);
}

void QResonanceAudioPlayer::setPaused(bool paused)
{
    m_paused = paused;
}

void QResonanceAudioPlayer::setOutputMode(QAudioEngine::OutputMode mode)
{
    m_outputMode = mode;
}

void QResonanceAudioPlayer::processSlice(QSpan<float> hostBuffer)
{
    using QtMultimediaPrivate::drop;
    constexpr auto framesPerBuffer = qToUnderlying(QAudioEnginePrivate::framesPerBuffer);
    const qsizetype samplesPerSlice = m_format.channelCount() * framesPerBuffer;

    Q_ASSERT(hostBuffer.size() == samplesPerSlice);

    // fill host API
    processSliceFillResonanceBuffers();

    // and process
    switch (m_outputMode) {
    case QAudioEngine::Surround:
        processSliceProcessSurroundMode(hostBuffer);
        break;
    default:
        processSliceProcessWithoutReverb(hostBuffer);
        break;
    }
}


void QResonanceAudioPlayer::processSliceFillResonanceBuffers()
{
    constexpr auto framesPerBuffer = qToUnderlying(QAudioEnginePrivate::framesPerBuffer);
    static constexpr std::array<float, 2 * framesPerBuffer> nullBuffer{};
    using QtMultimediaPrivate::take;
    vraudio::ResonanceAudioApi *api = m_resonanceAudio->api.get();

    for (auto &&[sourceId, sourceState] : m_sounds) {
        int numberOfChannels = sourceState.numberOfChannels;
        SharedPlaybackState &playbackState = sourceState.playbackState;
        if (playbackState) {
            Q_ASSERT(playbackState->format().channelCount() <= 2);
            std::array<float, 2 * framesPerBuffer> buf;

            playbackState->getBuffer(take(
                    QSpan<float>{ buf }, playbackState->format().channelCount() * framesPerBuffer));
            api->SetInterleavedBuffer(qToUnderlying(sourceId), buf.data(), numberOfChannels,
                                      framesPerBuffer);
        } else {
            api->SetInterleavedBuffer(qToUnderlying(sourceId), nullBuffer.data(), numberOfChannels,
                                      framesPerBuffer);
        }
    }
}

void QResonanceAudioPlayer::processSliceProcessSurroundMode(QSpan<float> outputBuffer)
{
    constexpr auto framesPerBuffer = qToUnderlying(QAudioEnginePrivate::framesPerBuffer);
    using QtMultimediaPrivate::take;
    namespace ranges = QtMultimediaPrivate::ranges;
    Q_ASSERT(outputBuffer.size() == m_format.channelCount() * framesPerBuffer);

    std::array<const float *, QAmbisonicDecoder::maxAmbisonicChannels> channels;
    std::array<const float *, 2> reverbBuffers{};
    int nFrames = m_resonanceAudio->getAmbisonicOutput(channels.data(), reverbBuffers.data(),
                                                       m_ambisonicDecoder.nInputChannels());

    if (nFrames < 0) {
        // If we get here, it means that resonanceAudio did not actually fill the buffer.
        // Sometimes this is expected, for example if resonanceAudio does not have any sources.
        // In this case we just fill the buffer with silence.
        ranges::fill(outputBuffer, 0);
        return;
    }
    auto nSamples = m_ambisonicDecoder.outputSamples(nFrames);

    Q_ASSERT(m_ambisonicDecoder.nOutputChannels() <= 8);
    QSpan currentOutput = take(outputBuffer, nSamples);
    m_ambisonicDecoder.processBufferWithReverb(
            QSpan{ channels.data(), m_ambisonicDecoder.nInputChannels() }, reverbBuffers,
            currentOutput);
}

void QResonanceAudioPlayer::processSliceProcessWithoutReverb(QSpan<float> outputBuffer)
{
    constexpr auto framesPerBuffer = qToUnderlying(QAudioEnginePrivate::framesPerBuffer);

    Q_ASSERT(outputBuffer.size() == m_format.channelCount() * framesPerBuffer);
    namespace ranges = QtMultimediaPrivate::ranges;

    bool ok = m_resonanceAudio->api->FillInterleavedOutputBuffer(
            m_format.channelCount(), framesPerBuffer, outputBuffer.data());
    if (!ok) {
        // If we get here, it means that resonanceAudio did not actually fill the buffer.
        // Sometimes this is expected, for example if resonanceAudio does not have any sources.
        // In this case we just fill the buffer with silence.
        if (m_sounds.empty()) {
            ranges::fill(outputBuffer, 0.f);
        } else {
            // If we get here, it means that something unexpected happened, so bail.
            qWarning() << "    processSliceProcessWithoutReverb failure!";
            return;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

QAudioEngineWithPlayer::QAudioEngineWithPlayer(int sampleRate) : QAudioEnginePrivate(sampleRate)
{
}

QAudioEngineWithPlayer::~QAudioEngineWithPlayer()
{
    stop();
    m_playbackEngine = nullptr;
}

void QAudioEngineWithPlayer::start()
{
    QAudioDevice device = m_device;
    if (device.isNull())
        device = QMediaDevices::defaultAudioOutput();

    if (device.isNull()) {
        qWarning() << "QAudioEngine::start() failed: No audio output device found.";
        return;
    }

    QAudioFormat format;
    format.setSampleFormat(QAudioFormat::Float);
    format.setSampleRate(sampleRate());
    format.setChannelCount(std::min(2, device.maximumChannelCount()));

    m_playbackEngine = std::make_shared<QRtAudioEngine>(device, format, std::nullopt,
                                                        QAudioEnginePrivate::framesPerBuffer);
    m_format = format;

    auto voice = std::make_shared<QResonanceAudioPlayer>(this, QRtAudioEngine::allocateVoiceId());

    for (auto &[sound, state] : playbackStates)
        voice->addSound(SourceId{ sound->sourceId }, sound->nchannels, state);

    m_engineVoice = voice;
    m_playbackEngine->play(voice);
}

void QAudioEngineWithPlayer::stop()
{
    if (!m_engineVoice)
        return;
    m_playbackEngine->stop(m_engineVoice);
    m_engineVoice = {};
}

void QAudioEngineWithPlayer::setPaused(bool paused)
{
    if (paused == m_paused)
        return;
    m_paused = paused;
    if (!m_engineVoice)
        return;

    // CHECK: can we pause the stream? it seems a bit problematic, as it would prevent us from sending
    // rt-visitors to update the state of the player.
    m_playbackEngine->visitVoiceRt(m_engineVoice->voiceId(),
                                   [paused](QResonanceAudioPlayer &player) {
        player.setPaused(paused);
    });
}

bool QAudioEngineWithPlayer::isPaused() const
{
    return m_paused;
}

void QAudioEngineWithPlayer::setOutputDevice(const QAudioDevice &device)
{
    if (m_device == device)
        return;
    if (m_engineVoice) {
        qWarning() << "Changing device on a running engine not implemented";
        return;
    }
    m_device = device;
    Q_Q(QAudioEngine);
    emit q->outputDeviceChanged();
}

QAudioDevice QAudioEngineWithPlayer::outputDevice() const
{
    return m_device;
}

void QAudioEngineWithPlayer::addSound(QAmbientSoundPrivate *sound)
{
    playbackStates.emplace(sound, nullptr);

    if (!m_engineVoice)
        return;

    m_playbackEngine->visitVoiceRt(
            m_engineVoice->voiceId(),
            [id = SourceId{ sound->sourceId },
             numberOfChannels = sound->nchannels](QResonanceAudioPlayer &player) mutable {
        player.addSound(id, numberOfChannels);
    });
}

void QAudioEngineWithPlayer::removeSound(QAmbientSoundPrivate *sound)
{
    SharedPlaybackState oldState = std::move(playbackStates[sound]);
    playbackStates.erase(sound);
    if (!m_engineVoice)
        return;

    // pass oldState to the lambda to ensure that it is not freed on the real-time thread
    m_playbackEngine->visitVoiceRt(m_engineVoice->voiceId(),
                                   [id = SourceId{ sound->sourceId },
                                    oldState = std::move(oldState)](QResonanceAudioPlayer &player) {
        player.removeSound(id);
    });
}

void QAudioEngineWithPlayer::setSoundPlaybackData(QAmbientSoundPrivate *sound,
                                                  SharedPlaybackState state)
{
    auto it = playbackStates.find(sound);
    Q_ASSERT(it != playbackStates.end());

    SharedPlaybackState oldState = std::exchange(it->second, state);

    if (!m_engineVoice)
        return;

    // pass oldState to the lambda to ensure that it is not freed on the real-time thread
    m_playbackEngine->visitVoiceRt(
            m_engineVoice->voiceId(),
            [id = SourceId{ sound->sourceId }, oldState = std::move(oldState),
             state = std::move(state)](QResonanceAudioPlayer &player) mutable {
        player.setSoundPlaybackData(id, std::move(state));
    });
}

void QAudioEngineWithPlayer::setOutputMode(QAudioEngine::OutputMode mode)
{
    if (outputMode() == mode)
        return;
    QAudioEnginePrivate::setOutputMode(mode);
    if (!m_engineVoice)
        return;

    m_playbackEngine->visitVoiceRt(m_engineVoice->voiceId(), [mode](QResonanceAudioPlayer &player) {
        player.setOutputMode(mode);
    });
}

void QAudioEngineWithPlayer::updateRoomEffects()
{
    for (auto [sound, key] : playbackStates)
        sound->updateRoomEffects();
}

QT_END_NAMESPACE
