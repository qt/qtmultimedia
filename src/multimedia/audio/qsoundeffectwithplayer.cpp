// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsoundeffectwithplayer_p.h"

#include <QtCore/qmutex.h>
#include <QtCore/q20map.h>

#include <utility>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

namespace {

QSpan<const float> toFloatSpan(QSpan<const char> byteArray)
{
    return QSpan{
        reinterpret_cast<const float *>(byteArray.data()),
        qsizetype(byteArray.size_bytes() / sizeof(float)),
    };
}

} // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////

QSoundEffectVoice::QSoundEffectVoice(VoiceId voiceId, std::shared_ptr<const QSample> sample,
                                     float volume, bool muted, int totalLoopCount,
                                     QAudioFormat engineFormat)
    : QRtAudioEngineVoice{ voiceId },
      m_sample{ std::move(sample) },
      m_engineFormat{ engineFormat },
      m_volume{ volume },
      m_muted{ muted },
      m_loopsRemaining{ totalLoopCount }
{
}

QSoundEffectVoice::~QSoundEffectVoice() = default;

VoicePlayResult QSoundEffectVoice::play(QSpan<float> outputBuffer) noexcept QT_MM_NONBLOCKING
{
    qsizetype playedFrames = playVoice(outputBuffer);
    m_currentFrame += playedFrames;

    if (m_currentFrame == m_totalFrames) {
        const bool isInfiniteLoop = loopsRemaining() == QSoundEffect::Infinite;
        bool continuePlaying = isInfiniteLoop;

        if (!isInfiniteLoop)
            continuePlaying = m_loopsRemaining.fetch_sub(1, std::memory_order_relaxed) > 1;

        if (continuePlaying) {
            if (!isInfiniteLoop)
                m_currentLoopChanged.set();
            m_currentFrame = 0;
            QSpan remainingOutputBuffer =
                    drop(outputBuffer, playedFrames * m_engineFormat.channelCount());
            return play(remainingOutputBuffer);
        }
        return VoicePlayResult::Finished;
    }
    return VoicePlayResult::Playing;
}

qsizetype QSoundEffectVoice::playVoice(QSpan<float> outputBuffer) noexcept QT_MM_NONBLOCKING
{
    const QAudioFormat &format = m_sample->format();
    const int totalSamples = m_totalFrames * format.channelCount();
    const int currentSample = format.channelCount() * m_currentFrame;

    const QSpan fullSample = toFloatSpan(m_sample->data());
    const QSpan playbackRange = take(drop(fullSample, currentSample), totalSamples);

    Q_ASSERT(!playbackRange.empty());

    const int sampleCh = format.channelCount();
    const int engineCh = m_engineFormat.channelCount();
    const qsizetype sampleSamples = playbackRange.size();
    const qsizetype outputSamples = outputBuffer.size();
    const qsizetype maxFrames = std::min(sampleSamples / sampleCh, outputSamples / engineCh);
    const qsizetype framesToPlay = maxFrames;
    const qsizetype outputSamplesPlayed = framesToPlay * engineCh;

    enum ConversionType : uint8_t { SameChannels, MonoToStereo, StereoToMono };
    const ConversionType conversion = [&] {
        if (sampleCh == engineCh)
            return SameChannels;
        if (sampleCh == 1 && engineCh == 2)
            return MonoToStereo;
        if (sampleCh == 2 && engineCh == 1)
            return StereoToMono;
        Q_UNREACHABLE_RETURN(SameChannels);
    }();

    if (m_muted || m_volume == 0.f) {
        std::fill_n(outputBuffer.begin(), outputSamplesPlayed, 0.f);
        return framesToPlay;
    }

    // later: (auto)vectorize?
    switch (conversion) {
    case SameChannels:
        for (qsizetype frame = 0; frame < framesToPlay; ++frame) {
            const qsizetype sampleBase = frame * sampleCh;
            const qsizetype outputBase = frame * engineCh;
            for (int ch = 0; ch < sampleCh; ++ch) {
                outputBuffer[outputBase + ch] += playbackRange[sampleBase + ch] * m_volume;
            }
        }
        break;
    case MonoToStereo:
        for (qsizetype frame = 0; frame < framesToPlay; ++frame) {
            const qsizetype sampleBase = frame * sampleCh;
            const qsizetype outputBase = frame * engineCh;
            const float val = playbackRange[sampleBase] * m_volume;
            outputBuffer[outputBase] += val;
            outputBuffer[outputBase + 1] += val;
        }
        break;
    case StereoToMono:
        float scale = 0.5f * m_volume;
        for (qsizetype frame = 0; frame < framesToPlay; ++frame) {
            const qsizetype sampleBase = frame * sampleCh;
            const qsizetype outputBase = frame * engineCh;
            const float val = (playbackRange[sampleBase] + playbackRange[sampleBase + 1]) * scale;
            outputBuffer[outputBase] += val;
        }
        break;
    }

    return framesToPlay;
}

bool QSoundEffectVoice::isActive() noexcept QT_MM_NONBLOCKING
{
    if (m_currentFrame != m_totalFrames)
        return true;

    return loopsRemaining() != 0;
}

std::shared_ptr<QSoundEffectVoice>
QSoundEffectVoice::clone(std::optional<QAudioFormat> newEngineFormat) const
{
    auto clone = std::make_shared<QSoundEffectVoice>(QRtAudioEngine::allocateVoiceId(), m_sample,
                                                     m_volume, m_muted, loopsRemaining(),
                                                     newEngineFormat.value_or(m_engineFormat));

    // caveat: reading frame is not atomic, so we may have a race here ... is is rare, though,
    // not sure if we really care
    clone->m_currentFrame = m_currentFrame;
    return clone;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

QSoundEffectPrivateWithPlayer::QSoundEffectPrivateWithPlayer(QSoundEffect *q,
                                                             QAudioDevice audioDevice)
    : q_ptr{ q }, m_audioDevice{ std::move(audioDevice) }
{
    resolveAudioDevice();

    QObject::connect(&m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, [this] {
        QAudioDevice defaultAudioDevice = QMediaDevices::defaultAudioOutput();
        if (defaultAudioDevice == m_defaultAudioDevice)
            return;

        m_defaultAudioDevice = QMediaDevices::defaultAudioOutput();
        if (m_audioDevice.isNull())
            setResolvedAudioDevice(m_defaultAudioDevice);
    });

    m_playerReleaseTimer.setTimerType(Qt::VeryCoarseTimer);
    m_playerReleaseTimer.setSingleShot(true);
}

QSoundEffectPrivateWithPlayer::~QSoundEffectPrivateWithPlayer()
{
    stop();
    if (m_sampleLoadFuture)
        m_sampleLoadFuture->cancelChain();
}

bool QSoundEffectPrivateWithPlayer::setAudioDevice(QAudioDevice device)
{
    if (device == m_audioDevice)
        return false;

    m_audioDevice = std::move(device);
    resolveAudioDevice();
    return true;
}

void QSoundEffectPrivateWithPlayer::setResolvedAudioDevice(QAudioDevice device)
{
    if (m_resolvedAudioDevice == device)
        return;

    m_resolvedAudioDevice = std::move(device);

    if (m_player)
        for (const auto &voice : m_voices)
            m_player->stop(voice);

    std::vector<std::shared_ptr<QSoundEffectVoice>> voices{
        std::make_move_iterator(m_voices.begin()), std::make_move_iterator(m_voices.end())
    };
    m_voices.clear();

    if (m_sample) {
        bool hasPlayer = updatePlayer(m_sample);
        if (!hasPlayer) {
            setStatus(QSoundEffect::Error);
            return;
        }

        for (const auto &voice : voices)
            // we re-allocate a new voice ID and play on the new player
            play(voice->clone(m_player->audioSink().format()));

        setStatus(QSoundEffect::Ready);

        for (const auto &voice : voices)
            // we re-allocate a new voice ID and play on the new player
            play(voice->clone());
    } else {
        setStatus(m_sampleLoadFuture ? QSoundEffect::Loading : QSoundEffect::Null);
    }
}

void QSoundEffectPrivateWithPlayer::resolveAudioDevice()
{
    if (m_audioDevice.isNull())
        m_defaultAudioDevice = QMediaDevices::defaultAudioOutput();
    setResolvedAudioDevice(m_audioDevice.isNull() ? m_defaultAudioDevice : m_audioDevice);
}

QAudioDevice QSoundEffectPrivateWithPlayer::audioDevice() const
{
    return m_audioDevice;
}

bool QSoundEffectPrivateWithPlayer::setSource(const QUrl &url, QSampleCache &sampleCache)
{
    if (m_sampleLoadFuture) {
        m_sampleLoadFuture->cancelChain();
        m_sampleLoadFuture = std::nullopt;
    }

    if (m_player) {
        QObject::disconnect(m_voiceFinishedConnection);
        m_playerReleaseTimer.callOnTimeout(this, [player = std::move(m_player)] {
            // we keep the player referenced for a little longer, so that later calls to
            // QRtAudioEngine::getEngineFor will be able to reuse the existing instance
        }, Qt::SingleShotConnection);
        m_playerReleaseTimer.start();
    }

    m_url = url;
    m_sample = {};

    if (url.isEmpty()) {
        setStatus(QSoundEffect::Null);
        return false;
    }

    if (!url.isValid()) {
        setStatus(QSoundEffect::Error);
        return false;
    }

    setStatus(QSoundEffect::Loading);

    m_sampleLoadFuture =
            sampleCache.requestSampleFuture(url).then(this, [this](SharedSamplePtr result) {
        if (result) {
            if (!formatIsSupported(result->format())) {
                qWarning("QSoundEffect: QSoundEffect only supports mono or stereo files");
                setStatus(QSoundEffect::Error);
                return;
            }

            bool hasPlayer = updatePlayer(result);
            m_sample = std::move(result);
            if (!hasPlayer) {
                qWarning("QSoundEffect: playback of this format is not supported on the selected "
                         "audio device");
                setStatus(QSoundEffect::Error);
                return;
            }

            setStatus(QSoundEffect::Ready);
            if (std::exchange(m_playPending, false)) {
                play();
            }
        } else {
            qWarning("QSoundEffect: Error decoding source %ls", qUtf16Printable(m_url.toString()));
            setStatus(QSoundEffect::Error);
        }
    });

    return true;
}

QUrl QSoundEffectPrivateWithPlayer::url() const
{
    return m_url;
}

void QSoundEffectPrivateWithPlayer::setStatus(QSoundEffect::Status status)
{
    if (status == m_status)
        return;
    m_status = status;
    emit q_ptr->statusChanged();
}

QSoundEffect::Status QSoundEffectPrivateWithPlayer::status() const
{
    return m_status;
}

int QSoundEffectPrivateWithPlayer::loopCount() const
{
    return m_loopCount;
}

bool QSoundEffectPrivateWithPlayer::setLoopCount(int loopCount)
{
    if (loopCount == 0)
        loopCount = 1;

    if (loopCount == m_loopCount)
        return false;

    m_loopCount = loopCount;

    if (m_voices.empty())
        return true;

    const std::shared_ptr<QSoundEffectVoice> &voice = *m_voices.rbegin();
    voice->m_loopsRemaining.store(loopCount, std::memory_order_relaxed);

    setLoopsRemaining(loopCount);

    return true;
}

int QSoundEffectPrivateWithPlayer::loopsRemaining() const
{
    if (m_voices.empty())
        return 0;

    return m_loopsRemaining;
}

float QSoundEffectPrivateWithPlayer::volume() const
{
    return m_volume;
}

bool QSoundEffectPrivateWithPlayer::setVolume(float volume)
{
    if (m_volume == volume)
        return false;

    m_volume = volume;
    for (const auto &voice : m_voices) {
        m_player->visitVoiceRt(voice, [volume](QSoundEffectVoice &voice) {
            voice.m_volume = volume;
        });
    }
    return true;
}

bool QSoundEffectPrivateWithPlayer::muted() const
{
    return m_muted;
}

bool QSoundEffectPrivateWithPlayer::setMuted(bool muted)
{
    if (m_muted == muted)
        return false;

    m_muted = muted;
    for (const auto &voice : m_voices) {
        m_player->visitVoiceRt(voice, [muted](QSoundEffectVoice &voice) {
            voice.m_muted = muted;
        });
    }
    return true;
}

void QSoundEffectPrivateWithPlayer::play()
{
    if (!m_sample) {
        m_playPending = true;
        return;
    }

    if (status() != QSoundEffect::Ready)
        return;

    Q_ASSERT(m_player);

    // each `play` will start a new voice
    auto voice = std::make_shared<QSoundEffectVoice>(QRtAudioEngine::allocateVoiceId(), m_sample,
                                                     m_volume, m_muted, m_loopCount,
                                                     m_player->audioSink().format());

    play(std::move(voice));
}

void QSoundEffectPrivateWithPlayer::stop()
{
    size_t activeVoices = m_voices.size();
    for (const auto &voice : m_voices)
        m_player->stop(voice->voiceId());
    setLoopsRemaining(0);

    m_voices.clear();
    m_playPending = false;
    if (activeVoices)
        emit q_ptr->playingChanged();
}

bool QSoundEffectPrivateWithPlayer::playing() const
{
    return !m_voices.empty();
}

void QSoundEffectPrivateWithPlayer::play(std::shared_ptr<QSoundEffectVoice> voice)
{
    QObject::connect(&voice->m_currentLoopChanged, &QAutoResetEvent::activated, this,
                     [this, voiceId = voice->voiceId()] {
        auto foundVoice = m_voices.find(voiceId);
        if (foundVoice == m_voices.end())
            return;

        if (voiceId != activeVoice())
            return;

        setLoopsRemaining((*foundVoice)->loopsRemaining());
    });

    m_player->play(voice);
    m_voices.insert(std::move(voice));
    setLoopsRemaining(m_loopCount);
    if (m_voices.size() == 1)
        emit q_ptr->playingChanged();
}

bool QSoundEffectPrivateWithPlayer::updatePlayer(const SharedSamplePtr &sample)
{
    Q_ASSERT(sample);
    Q_ASSERT(m_voices.empty());
    QObject::disconnect(m_voiceFinishedConnection);

    m_player = {};
    if (m_resolvedAudioDevice.isNull())
        return false;

    m_player = [&]() -> std::shared_ptr<QRtAudioEngine> {
        auto player = QRtAudioEngine::getEngineFor(m_resolvedAudioDevice, sample->format());
        if (player)
            return player;

        QAudioFormat alternativeFormat = sample->format();
        switch (sample->format().channelCount()) {
        case 1:
            alternativeFormat.setChannelCount(2);
            break;
        case 2:
            alternativeFormat.setChannelCount(1);
            break;
        default:
            Q_UNREACHABLE_RETURN({});
        }

        return QRtAudioEngine::getEngineFor(m_resolvedAudioDevice, alternativeFormat);
    }();

    if (!m_player)
        return false;

    m_voiceFinishedConnection = QObject::connect(m_player.get(), &QRtAudioEngine::voiceFinished,
                                                 this, [this](VoiceId voiceId) {
        if (voiceId == activeVoice())
            setLoopsRemaining(0);

        auto found = m_voices.find(voiceId);
        if (found != m_voices.end()) {
            m_voices.erase(found);
            if (m_voices.empty())
                emit q_ptr->playingChanged();
        }
    });
    return true;
}

std::optional<VoiceId> QSoundEffectPrivateWithPlayer::activeVoice() const
{
    if (m_voices.empty())
        return std::nullopt;
    return (*m_voices.rbegin())->voiceId();
}

bool QSoundEffectPrivateWithPlayer::formatIsSupported(const QAudioFormat &fmt)
{
    switch (fmt.channelCount()) {
    case 1:
    case 2:
        return true;
    default:
        return false;
    }
}

void QSoundEffectPrivateWithPlayer::setLoopsRemaining(int loopsRemaining)
{
    if (loopsRemaining == m_loopsRemaining)
        return;
    m_loopsRemaining = loopsRemaining;
    emit q_ptr->loopsRemainingChanged();
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
