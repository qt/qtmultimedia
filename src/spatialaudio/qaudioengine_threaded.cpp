// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#include "qaudioengine_threaded_p.h"

#include <QtCore/qiodevice.h>
#include <QtCore/qdebug.h>
#include <QtCore/qelapsedtimer.h>

#include <QtMultimedia/qaudiodecoder.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/private/qaudio_qspan_support_p.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>
#ifdef Q_OS_WIN
#  include <QtMultimedia/private/qwindows_wasapi_warmup_client_p.h>
#endif

#include <QtSpatialAudio/private/qambientsound_p.h>
#include <QtSpatialAudio/private/qspatialsound_p.h>
#include <QtSpatialAudio/private/qaudioroom_p.h>
#include <QtSpatialAudio/private/qambisonicdecoder_p.h>
#include <QtSpatialAudio/qambientsound.h>
#include <QtSpatialAudio/qaudiolistener.h>

#include <resonance_audio.h>

#include <memory>
#include <q20vector.h>

QT_BEGIN_NAMESPACE

// We'd like to have short buffer times, so the sound adjusts itself to changes
// quickly, but times below 100ms seem to give stuttering on macOS.
// It might be possible to set this value lower on other OSes.
const int bufferTimeMs = 100;

// This class lives in the audioThread, but pulls data from QAudioEnginePrivate
// which lives in the mainThread.
class QAudioOutputStream : public QIODevice
{
public:
    explicit QAudioOutputStream(QAudioEngineThreaded *d) : d(d) { open(QIODevice::ReadOnly); }
    ~QAudioOutputStream() override;

    qint64 readData(char *data, qint64 len) override;

    qint64 writeData(const char *, qint64) override;

    qint64 size() const override { return 0; }
    qint64 bytesAvailable() const override {
        return std::numeric_limits<qint64>::max();
    }
    bool isSequential() const override {
        return true;
    }
    bool atEnd() const override {
        return false;
    }
    qint64 pos() const override {
        return m_pos;
    }

    void startOutput()
    {
        d->mutex.lock();
        Q_ASSERT(!sink);
        auto channelConfig = d->m_outputMode == QAudioEngine::Surround
                ? d->m_device.channelConfiguration()
                : QAudioFormat::ChannelConfigStereo;

        QAudioFormat format;
        if (channelConfig != QAudioFormat::ChannelConfigUnknown)
            format.setChannelConfig(channelConfig);
        else
            format.setChannelCount(d->m_device.preferredFormat().channelCount());
        format.setSampleRate(d->sampleRate());
        format.setSampleFormat(QAudioFormat::Int16);

        ambisonicDecoder = std::make_unique<QAmbisonicDecoder>(
                QAmbisonicDecoder::AmbisonicOrder::HighQuality, format.sampleRate(),
                format.channelCount(), format.channelConfig());

        sink = std::make_unique<QAudioSink>(d->m_device, format);
        const qsizetype bufferSize = format.bytesForDuration(bufferTimeMs * 1000);
        sink->setBufferSize(bufferSize);
        d->mutex.unlock();
        // It is important to unlock the mutex before starting the sink, as the sink will
        // call readData() in the audio thread, which will try to lock the mutex (again)
        sink->start(this);

#ifdef Q_OS_WIN
        QtMultimediaPrivate::refreshWarmupClient();
#endif
    }

    void stopOutput()
    {
        if (!sink)
            return;
        sink->stop();
        sink.reset();
        ambisonicDecoder.reset();
    }

    void setPaused(bool paused) {
        if (paused)
            sink->suspend();
        else
            sink->resume();
    }

private:
    qint64 m_pos = 0;
    QAudioEngineThreaded *d = nullptr;
    std::unique_ptr<QAudioSink> sink;
    std::unique_ptr<QAmbisonicDecoder> ambisonicDecoder;
};

QAudioOutputStream::~QAudioOutputStream() = default;

qint64 QAudioOutputStream::writeData(const char *, qint64)
{
    return 0;
}

qint64 QAudioOutputStream::readData(char *data, const qint64 len)
{
    if (d->m_paused.loadRelaxed())
        return 0;

    constexpr auto framesPerBuffer = QAudioEngineThreaded::framesPerBuffer;
    QSpan<short> outputBuffer((short *)data, len / sizeof(short));

    QMutexLocker l(&d->mutex);
    d->updateRooms();

    int nChannels = ambisonicDecoder ? ambisonicDecoder->nOutputChannels() : 2;
    if (outputBuffer.size() < nChannels * framesPerBuffer)
        return 0;

    using QtMultimediaPrivate::drop;
    using QtMultimediaPrivate::take;
    using namespace QAudioHelperInternal;

    bool ok = true;
    while (outputBuffer.size() >= nChannels * framesPerBuffer) {
        // Fill input buffers
        for (auto *source : d->sources) {
            Q_ASSERT(source->nchannels <= 2);
            std::array<float, 2 * framesPerBuffer> buf;
            source->getBuffer(take(QSpan<float>{ buf }, source->nchannels * framesPerBuffer),
                              source->nchannels);
            d->resonanceAudio->api->SetInterleavedBuffer(source->sourceId, buf.data(),
                                                         source->nchannels, framesPerBuffer);
        }

        if (ambisonicDecoder && d->m_outputMode == QAudioEngine::Surround) {
            std::array<const float *, QAmbisonicDecoder::maxAmbisonicChannels> channels;
            std::array<const float *, 2> reverbBuffers{};
            int nFrames = d->resonanceAudio->getAmbisonicOutput(
                    channels.data(), reverbBuffers.data(), ambisonicDecoder->nInputChannels());

            if (nFrames < 0) {
                // If we get here, it means that resonanceAudio did not actually fill the buffer.
                // Sometimes this is expected, for example if resonanceAudio does not have any sources.
                // In this case we just fill the buffer with silence.
                std::fill(outputBuffer.begin(), outputBuffer.end(), 0);
                break;
            }

            Q_ASSERT(ambisonicDecoder->nOutputChannels() <= 8);
            int nSamples = ambisonicDecoder->outputSamples(nFrames);

            std::array<float, 2 * framesPerBuffer> reverbFloatBuffers;
            QSpan<float> reverbOutputSpan = take(QSpan{ reverbFloatBuffers }, nSamples);
            QSpan<short> currentOutput = take(outputBuffer, nSamples);

            ambisonicDecoder->processBufferWithReverb(
                    QSpan{ channels.data(), ambisonicDecoder->nInputChannels() }, reverbBuffers,
                    reverbOutputSpan);

            convertSampleFormat(as_bytes(reverbOutputSpan), NativeSampleFormat::float32_t,
                                as_writable_bytes(currentOutput), NativeSampleFormat::int16_t);
            outputBuffer = drop(outputBuffer, nSamples);
        } else {
            QSpan<short> currentOutput = take(outputBuffer, nChannels * framesPerBuffer);
            ok = d->resonanceAudio->api->FillInterleavedOutputBuffer(2, framesPerBuffer,
                                                                     currentOutput.data());
            if (!ok) {
                // If we get here, it means that resonanceAudio did not actually fill the buffer.
                // Sometimes this is expected, for example if resonanceAudio does not have any sources.
                // In this case we just fill the buffer with silence.
                if (d->sources.empty()) {
                    std::fill(currentOutput.begin(), currentOutput.end(), 0);
                } else {
                    // If we get here, it means that something unexpected happened, so bail.
                    qWarning() << "    Reading failed!";
                    break;
                }
            }
            outputBuffer = drop(outputBuffer, nChannels * framesPerBuffer);
        }
    }

    qint64 bytesProcessed = len - outputBuffer.size_bytes();
    m_pos += bytesProcessed;
    return bytesProcessed;
}

QAudioEngineThreaded::QAudioEngineThreaded(int sampleRate) : QAudioEnginePrivate(sampleRate)
{
    audioThread.setObjectName(u"QAudioThread");
    m_device = QMediaDevices::defaultAudioOutput();
}

QAudioEngineThreaded::~QAudioEngineThreaded()
{
    stop();
}

void QAudioEngineThreaded::start()
{
    if (outputStream)
        return; // already started

    resonanceAudio->api->SetStereoSpeakerMode(m_outputMode != QAudioEngine::Headphone);
    resonanceAudio->api->SetMasterVolume(masterVolume());

    outputStream = std::make_unique<QAudioOutputStream>(this);
    outputStream->moveToThread(&audioThread);
    audioThread.start(QThread::TimeCriticalPriority);

    QMetaObject::invokeMethod(outputStream.get(), &QAudioOutputStream::startOutput);
}

void QAudioEngineThreaded::stop()
{
    if (!outputStream)
        return; // already stopped

    QMetaObject::invokeMethod(outputStream.get(), &QAudioOutputStream::stopOutput,
                              Qt::BlockingQueuedConnection);
    outputStream.reset();
    audioThread.exit(0);
    audioThread.wait();
}

void QAudioEngineThreaded::setPaused(bool paused)
{
    if (!outputStream)
        return; // can't pause if not started

    bool old = m_paused.fetchAndStoreRelaxed(paused);
    if (old != paused) {
        if (outputStream)
            outputStream->setPaused(paused);
        Q_Q(QAudioEngine);
        emit q->pausedChanged();
    }
}

bool QAudioEngineThreaded::isPaused() const
{
    return m_paused.loadRelaxed();
}

void QAudioEngineThreaded::setOutputDevice(const QAudioDevice &device)
{
    if (m_device == device)
        return;
    if (outputStream) {
        qWarning() << "Changing device on a running engine not implemented";
        return;
    }
    m_device = device;
    Q_Q(QAudioEngine);
    emit q->outputDeviceChanged();
}

void QAudioEngineThreaded::setOutputMode(QAudioEngine::OutputMode mode)
{
    if (m_outputMode == mode)
        return;
    m_outputMode = mode;
    resonanceAudio->api->SetStereoSpeakerMode(mode != QAudioEngine::Headphone);

    Q_Q(QAudioEngine);
    emit q->outputModeChanged();
}

void QAudioEngineThreaded::setRoomEffectsEnabled(bool enabled)
{
    if (m_roomEffectsEnabled == enabled)
        return;
    m_roomEffectsEnabled = enabled;
    resonanceAudio->roomEffectsEnabled = enabled;
}

/*!
    Returns true if room effects are enabled.
 */
bool QAudioEngineThreaded::roomEffectsEnabled() const
{
    return m_roomEffectsEnabled;
}

void QAudioEngineThreaded::setListenerPosition(std::optional<QVector3D> pos)
{
    if (listenerPosition() == pos)
        return;

    QAudioEnginePrivate::setListenerPosition(pos);
    listenerPositionDirty = true;
}

void QAudioEngineThreaded::addSound(QAmbientSoundPrivate *sound)
{
    QMutexLocker l(&mutex);
    sources.push_back(sound);
}

void QAudioEngineThreaded::removeSound(QAmbientSoundPrivate *sound)
{
    QMutexLocker l(&mutex);
    q20::erase(sources, sound);
}

void QAudioEngineThreaded::addRoom(QAudioRoom *room)
{
    QMutexLocker l(&mutex);
    rooms.push_back(room);
}

void QAudioEngineThreaded::removeRoom(QAudioRoom *room)
{
    QMutexLocker l(&mutex);
    q20::erase(rooms, room);
}

// This method is called from the audio thread
void QAudioEngineThreaded::updateRooms()
{
    if (!m_roomEffectsEnabled)
        return;

    bool needUpdate = listenerPositionDirty;
    listenerPositionDirty = false;

    bool roomDirty = false;
    for (const auto &room : rooms) {
        auto *rd = QAudioRoomPrivate::get(room);
        if (rd->dirty) {
            roomDirty = true;
            rd->update();
            needUpdate = true;
        }
    }

    if (!needUpdate)
        return;

    auto inferredRoom = findSmallestRoomForListener(rooms);
    if (inferredRoom.room != m_currentRoom)
        roomDirty = true;
    const bool previousRoom = m_currentRoom;
    m_currentRoom = inferredRoom.room;

    if (!roomDirty)
        return;

    // apply room to engine
    if (!m_currentRoom) {
        resonanceAudio->api->EnableRoomEffects(false);
        return;
    }
    if (!previousRoom)
        resonanceAudio->api->EnableRoomEffects(true);

    QAudioRoomPrivate *rp = QAudioRoomPrivate::get(m_currentRoom);
    resonanceAudio->api->SetReflectionProperties(rp->reflections);
    resonanceAudio->api->SetReverbProperties(rp->reverb);

    // update room effects for all sound sources
    for (auto *s : sources)
        s->updateRoomEffects();
}

QT_END_NAMESPACE
