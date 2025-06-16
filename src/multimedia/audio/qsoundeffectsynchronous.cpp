// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsoundeffectsynchronous_p.h"

#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qplatformaudiodevices_p.h>
#include <QtMultimedia/private/qplatformaudioresampler_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#ifdef Q_OS_WIN
#  include <QtMultimedia/private/qwindows_wasapi_warmup_client_p.h>
#endif

QT_BEGIN_NAMESPACE

void QSoundEffectPrivateSynchronous::AudioSinkDeleter::operator()(QAudioSink *sink) const
{
    sink->stop();
    // Investigate:should we just delete?
    sink->deleteLater();
}

QSoundEffectPrivateSynchronous::QSoundEffectPrivateSynchronous(QSoundEffect *q,
                                                               const QAudioDevice &audioDevice)
    : q_ptr(q), m_audioDevice(audioDevice)
{
    open(QIODevice::ReadOnly);
}

QSoundEffectPrivateSynchronous::~QSoundEffectPrivateSynchronous()
{
    m_audioSink.reset();
    m_sample.reset();
}

qint64 QSoundEffectPrivateSynchronous::readData(char *data, qint64 len)
{
    qCDebug(qLcSoundEffect) << this << "readData" << len << m_runningCount;
    if (!len)
        return 0;
    if (m_sample->state() != QSample::Ready)
        return 0;
    if (m_runningCount == 0 || !m_playing)
        return 0;

    qint64 bytesWritten = 0;

    const int sampleSize = m_audioBuffer.byteCount();
    const char *sampleData = m_audioBuffer.constData<char>();

    while (len && m_runningCount) {
        int toWrite = qMax(0, qMin(sampleSize - m_offset, len));
        memcpy(data, sampleData + m_offset, toWrite);
        bytesWritten += toWrite;
        data += toWrite;
        len -= toWrite;
        m_offset += toWrite;
        if (m_offset >= sampleSize) {
            if (m_runningCount > 0)
                setLoopsRemaining(m_runningCount - 1);
            m_offset = 0;
        }
    }

    return bytesWritten;
}

qint64 QSoundEffectPrivateSynchronous::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

qint64 QSoundEffectPrivateSynchronous::size() const
{
    if (m_sample->state() != QSample::Ready)
        return 0;
    return m_loopCount == QSoundEffect::Infinite ? 0 : m_loopCount * m_audioBuffer.byteCount();
}

qint64 QSoundEffectPrivateSynchronous::bytesAvailable() const
{
    if (m_sample->state() != QSample::Ready)
        return 0;
    if (m_loopCount == QSoundEffect::Infinite)
        return std::numeric_limits<qint64>::max();
    return m_runningCount * m_audioBuffer.byteCount() - m_offset;
}

bool QSoundEffectPrivateSynchronous::isSequential() const
{
    return m_loopCount == QSoundEffect::Infinite;
}

bool QSoundEffectPrivateSynchronous::atEnd() const
{
    return m_runningCount == 0;
}

void QSoundEffectPrivateSynchronous::setLoopsRemaining(int loopsRemaining)
{
    if (m_runningCount == loopsRemaining)
        return;
    qCDebug(qLcSoundEffect) << this << "setLoopsRemaining " << loopsRemaining;
    m_runningCount = loopsRemaining;
    emit q_ptr->loopsRemainingChanged();
}

void QSoundEffectPrivateSynchronous::setStatus(QSoundEffect::Status status)
{
    qCDebug(qLcSoundEffect) << this << "setStatus" << status;
    if (m_status == status)
        return;
    bool oldLoaded = q_ptr->isLoaded();
    m_status = status;
    emit q_ptr->statusChanged();
    if (oldLoaded != q_ptr->isLoaded())
        emit q_ptr->loadedChanged();
}

void QSoundEffectPrivateSynchronous::setPlaying(bool playing)
{
    qCDebug(qLcSoundEffect) << this << "setPlaying(" << playing << ")" << m_playing;
    if (m_audioSink) {
        m_audioSink->reset();
        if (playing && !m_sampleReady)
            return;
    }

    if (m_playing == playing)
        return;
    m_playing = playing;

    if (m_audioSink && playing) {
        m_audioSink->start(this);
#ifdef Q_OS_WIN
        QtMultimediaPrivate::refreshWarmupClient();
#endif
    }

    emit q_ptr->playingChanged();
}

bool QSoundEffectPrivateSynchronous::updateAudioOutput()
{
    const auto audioDevice =
            m_audioDevice.isNull() ? QMediaDevices::defaultAudioOutput() : m_audioDevice;

    if (audioDevice.isNull()) {
        // We are likely on a virtual machine, for example in CI
        qCCritical(qLcSoundEffect) << "Failed to update audio output. No audio devices available.";
        setStatus(QSoundEffect::Error);
        return false;
    }

    if (m_audioDevice.isNull()) {
        q_ptr->setAudioDevice(audioDevice); // Updates m_audioDevice and emits audioDeviceChanged
    }

    m_audioBuffer = {};

    Q_ASSERT(m_sample);

    const auto &sampleFormat = m_sample->format();
    const auto sampleChannelConfig =
            sampleFormat.channelConfig() == QAudioFormat::ChannelConfigUnknown
            ? QAudioFormat::defaultChannelConfigForChannelCount(sampleFormat.channelCount())
            : sampleFormat.channelConfig();

    if (sampleChannelConfig != audioDevice.channelConfiguration()
        && audioDevice.channelConfiguration() != QAudioFormat::ChannelConfigUnknown) {
        qCDebug(qLcSoundEffect) << "Create resampler for channels mapping: config"
                                << sampleFormat.channelConfig() << "=> config"
                                << audioDevice.channelConfiguration();
        auto outputFormat = sampleFormat;
        outputFormat.setChannelConfig(audioDevice.channelConfiguration());

        const auto resampler = QPlatformMediaIntegration::instance()->createAudioResampler(
                m_sample->format(), outputFormat);
        if (resampler)
            m_audioBuffer = resampler.value()->resample(m_sample->data().constData(),
                                                        m_sample->data().size());
        else
            qCDebug(qLcSoundEffect) << "Cannot create resampler for channels mapping";
    }

    if (!m_audioBuffer.isValid())
        m_audioBuffer = QAudioBuffer(m_sample->data(), m_sample->format());

    m_audioSink.reset(new QAudioSink(audioDevice, m_audioBuffer.format()));

    QObject::connect(m_audioSink.get(), &QAudioSink::stateChanged, this,
                     [this](QAudio::State state) {
        this->stateChanged(state);
    });

    if (!m_muted)
        m_audioSink->setVolume(m_volume);
    else
        m_audioSink->setVolume(0);

    QPlatformAudioSink *sinkPrivate = QPlatformAudioSink::get(*m_audioSink);
    sinkPrivate->setRole(QtMultimediaPrivate::AudioEndpointRole::SoundEffect);

    return true;
}

void QSoundEffectPrivateSynchronous::decoderError()
{
    qWarning("QSoundEffect(qaudio): Error decoding source %ls", qUtf16Printable(m_url.toString()));

    m_playing = false;
    setStatus(QSoundEffect::Error);
}

void QSoundEffectPrivateSynchronous::sampleReady(SharedSamplePtr sample)
{
    if (m_status == QSoundEffect::Error)
        return;

    m_sample = std::move(sample);

    qCDebug(qLcSoundEffect) << this << "sampleReady: sample size:" << m_sample->data().size();
    if (!m_audioSink) {
        if (!updateAudioOutput()) // Create audio sink
            return; // Returns if no audio devices are available
    }

    m_sampleReady = true;
    setStatus(QSoundEffect::Ready);

    if (m_playing && m_audioSink->state() == QAudio::StoppedState) {
        qCDebug(qLcSoundEffect) << this << "starting playback on audiooutput";
        m_audioSink->start(this);
    }
}

int QSoundEffectPrivateSynchronous::loopCount() const
{
    return m_loopCount;
}

bool QSoundEffectPrivateSynchronous::setLoopCount(int loopCount)
{
    if (loopCount == 0)
        loopCount = 1;

    if (loopCount == m_loopCount)
        return false;

    m_loopCount = loopCount;
    if (m_playing)
        setLoopsRemaining(loopCount);
    return true;
}

int QSoundEffectPrivateSynchronous::loopsRemaining() const
{
    return m_runningCount;
}

float QSoundEffectPrivateSynchronous::volume() const
{
    if (m_audioSink && !m_muted)
        return m_audioSink->volume();

    return m_volume;
}

bool QSoundEffectPrivateSynchronous::setVolume(float volume)
{
    volume = qBound(0.0f, volume, 1.0f);
    if (m_volume == volume)
        return false;

    m_volume = volume;

    if (m_audioSink && !m_muted)
        m_audioSink->setVolume(volume);
    return true;
}

bool QSoundEffectPrivateSynchronous::muted() const
{
    return m_muted;
}

bool QSoundEffectPrivateSynchronous::setMuted(bool muted)
{
    if (m_muted == muted)
        return false;

    if (muted && m_audioSink)
        m_audioSink->setVolume(0);
    else if (!muted && m_audioSink && m_muted)
        m_audioSink->setVolume(m_volume);

    m_muted = muted;
    return true;
}

void QSoundEffectPrivateSynchronous::play()
{
    m_offset = 0;
    setLoopsRemaining(m_loopCount);
    qCDebug(qLcSoundEffect) << this << "play" << m_loopCount << m_runningCount;
    if (m_status == QSoundEffect::Null || m_status == QSoundEffect::Error) {
        setStatus(QSoundEffect::Null);
        return;
    }
    setPlaying(true);
}

void QSoundEffectPrivateSynchronous::stop()
{
    if (!m_playing)
        return;
    qCDebug(qLcSoundEffect) << "stop()";
    m_offset = 0;
    setPlaying(false);
}

bool QSoundEffectPrivateSynchronous::playing() const
{
    return m_playing;
}

bool QSoundEffectPrivateSynchronous::setAudioDevice(QAudioDevice device)
{
    if (m_audioDevice == device)
        return false;

    m_audioDevice = device;

    if (!m_sampleReady)
        return true; // The audio sink will be recreated later by QSoundEffect::sampleReady()

    bool playing = m_playing;
    std::chrono::microseconds current_time{ m_audioBuffer.format().durationForBytes(m_offset) };

    // Recreate the QAudioSink with the new audio device and current sample
    if (updateAudioOutput() && playing) {
        // Resume playback from current position
        m_offset = m_audioBuffer.format().bytesForDuration(current_time.count());
        setPlaying(true);
    }
    return true;
}

bool QSoundEffectPrivateSynchronous::setSource(const QUrl &url, QSampleCache &sampleCache)
{
    if (m_sampleLoadFuture.isValid())
        m_sampleLoadFuture.cancel();

    m_url = url;
    m_sampleReady = false;

    if (url.isEmpty()) {
        setStatus(QSoundEffect::Null);
        return false;
    }

    if (!url.isValid()) {
        setStatus(QSoundEffect::Error);
        return false;
    }

    setStatus(QSoundEffect::Loading);
    m_sample = {};

    if (m_audioSink) {
        QObject::disconnect(m_audioSink.get(), &QAudioSink::stateChanged, this,
                            &QSoundEffectPrivateSynchronous::stateChanged);
        m_audioSink.reset();
    }

    m_sampleLoadFuture =
            sampleCache.requestSampleFuture(url).then(this, [this](SharedSamplePtr result) {
        if (result)
            sampleReady(std::move(result));
        else
            decoderError();
    });

    return true;
}

QSoundEffect::Status QSoundEffectPrivateSynchronous::status() const
{
    return m_status;
}

QUrl QSoundEffectPrivateSynchronous::url() const
{
    return m_url;
}

QAudioDevice QSoundEffectPrivateSynchronous::audioDevice() const
{
    return m_audioDevice;
}

void QSoundEffectPrivateSynchronous::stateChanged(QAudio::State state)
{
    qCDebug(qLcSoundEffect) << this << "stateChanged " << state;
    if ((state == QAudio::IdleState && m_runningCount == 0) || state == QAudio::StoppedState)
        stop();
}

QT_END_NAMESPACE
