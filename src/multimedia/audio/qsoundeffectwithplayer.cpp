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
                                     float volume, bool muted, int totalLoopCount)
    : QRtAudioEngineVoice{ voiceId },
      m_sample{ std::move(sample) },
      m_volume{ volume },
      m_muted{ muted },
      m_loopsRemaining{ totalLoopCount }
{
}

VoicePlayResult QSoundEffectVoice::play(QSpan<float> outputBuffer) noexcept QT_MM_NONBLOCKING
{
    const QAudioFormat &format = m_sample->format();
    int totalSamples = m_totalFrames * format.channelCount();
    int currentSample = format.channelCount() * m_currentFrame;

    const QSpan fullSample = toFloatSpan(m_sample->data());
    QSpan playbackRange = take(drop(fullSample, currentSample), totalSamples);

    Q_ASSERT(!playbackRange.empty());

    // later: (auto)vectorize?
    qsizetype samplesToPlay = std::min(playbackRange.size(), outputBuffer.size());
    if (m_muted || m_volume == 0.f) {
        auto outputRange = take(outputBuffer, samplesToPlay);
        std::fill(outputRange.begin(), outputRange.end(), 0.f);
    } else if (m_volume == 1.f) {
        for (qsizetype i = 0; i != samplesToPlay; ++i)
            outputBuffer[i] += playbackRange[i];
    } else {
        for (qsizetype i = 0; i != samplesToPlay; ++i)
            outputBuffer[i] += playbackRange[i] * m_volume;
    }

    m_currentFrame += samplesToPlay / format.channelCount();

    if (m_currentFrame == m_totalFrames) {
        const bool isInfiniteLoop = loopsRemaining() == QSoundEffect::Infinite;
        bool continuePlaying = isInfiniteLoop;

        if (!isInfiniteLoop)
            continuePlaying = m_loopsRemaining.fetch_sub(1, std::memory_order_relaxed) > 1;

        if (continuePlaying) {
            if (!isInfiniteLoop)
                m_currentLoopChanged.set();
            m_currentFrame = 0;
            QSpan remainingOutputBuffer = drop(outputBuffer, samplesToPlay);
            return play(remainingOutputBuffer);
        }
        return VoicePlayResult::Finished;
    }
    return VoicePlayResult::Playing;
}

bool QSoundEffectVoice::isActive() noexcept QT_MM_NONBLOCKING
{
    if (m_currentFrame != m_totalFrames)
        return true;

    return loopsRemaining() != 0;
}

std::shared_ptr<QSoundEffectVoice> QSoundEffectVoice::clone() const
{
    auto clone = std::make_shared<QSoundEffectVoice>(QRtAudioEngine::allocateVoiceId(), m_sample,
                                                     m_volume, m_muted, loopsRemaining());

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
}

QSoundEffectPrivateWithPlayer::~QSoundEffectPrivateWithPlayer()
{
    stop();
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

    if (!m_player)
        return;

    for (const auto &voice : m_voices)
        m_player->stop(voice);

    std::vector<std::shared_ptr<QSoundEffectVoice>> voices{
        std::make_move_iterator(m_voices.begin()), std::make_move_iterator(m_voices.end())
    };
    m_voices.clear();

    bool hasPlayer = updatePlayer();
    if (!hasPlayer)
        return;

    for (const auto &voice : voices)
        // we re-allocate a new voice ID and play on the new player
        play(voice->clone());
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
    if (m_sampleLoadFuture.isValid())
        m_sampleLoadFuture.cancel();

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

            m_sample = std::move(result);
            setStatus(QSoundEffect::Ready);
            bool hasPlayer = updatePlayer();
            if (std::exchange(m_playPending, false)) {
                if (hasPlayer)
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

    // each `play` will start a new voice
    Q_ASSERT(m_player);

    auto voice = std::make_shared<QSoundEffectVoice>(QRtAudioEngine::allocateVoiceId(), m_sample,
                                                     m_volume, m_muted, m_loopCount);

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

bool QSoundEffectPrivateWithPlayer::updatePlayer()
{
    Q_ASSERT(m_voices.empty());
    QObject::disconnect(m_voiceFinishedConnection);

    m_player = {};
    if (m_resolvedAudioDevice.isNull())
        return false;

    auto player = QRtAudioEngine::getEngineFor(m_resolvedAudioDevice, m_sample->format());
    m_player = player;

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
