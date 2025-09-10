// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#include "qaudioengine_p.h"

#include <QtCore/qiodevice.h>
#include <QtCore/qdebug.h>
#include <QtCore/qelapsedtimer.h>

#include <QtMultimedia/qaudiodecoder.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qaudiosink.h>
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
    explicit QAudioOutputStream(QAudioEnginePrivate *d)
        : d(d)
    {
        open(QIODevice::ReadOnly);
    }
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
        QAudioFormat format;
        auto channelConfig = d->outputMode == QAudioEngine::Surround ?
                             d->device.channelConfiguration() : QAudioFormat::ChannelConfigStereo;
        if (channelConfig != QAudioFormat::ChannelConfigUnknown)
            format.setChannelConfig(channelConfig);
        else
            format.setChannelCount(d->device.preferredFormat().channelCount());
        format.setSampleRate(d->sampleRate);
        format.setSampleFormat(QAudioFormat::Int16);
        ambisonicDecoder.reset(new QAmbisonicDecoder(QAmbisonicDecoder::HighQuality, format));
        sink.reset(new QAudioSink(d->device, format));
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

    void restartOutput()
    {
        stopOutput();
        startOutput();
    }

    void setPaused(bool paused) {
        if (paused)
            sink->suspend();
        else
            sink->resume();
    }

private:
    qint64 m_pos = 0;
    QAudioEnginePrivate *d = nullptr;
    std::unique_ptr<QAudioSink> sink;
    std::unique_ptr<QAmbisonicDecoder> ambisonicDecoder;
};


QAudioOutputStream::~QAudioOutputStream()
{
}

qint64 QAudioOutputStream::writeData(const char *, qint64)
{
    return 0;
}

qint64 QAudioOutputStream::readData(char *data, qint64 len)
{
    if (d->paused.loadRelaxed())
        return 0;

    QMutexLocker l(&d->mutex);
    d->updateRooms();

    int nChannels = ambisonicDecoder ? ambisonicDecoder->nOutputChannels() : 2;
    if (len < nChannels*int(sizeof(float))*QAudioEnginePrivate::bufferSize)
        return 0;

    short *fd = (short *)data;
    qint64 frames = len / nChannels / sizeof(short);
    bool ok = true;
    while (frames >= qint64(QAudioEnginePrivate::bufferSize)) {
        // Fill input buffers
        for (auto *source : std::as_const(d->sources)) {
            auto *sp = QSpatialSoundPrivate::get(source);
            if (!sp)
                continue;
            float buf[QAudioEnginePrivate::bufferSize];
            sp->getBuffer(buf, QAudioEnginePrivate::bufferSize, 1);
            d->resonanceAudio->api->SetInterleavedBuffer(sp->sourceId, buf, 1, QAudioEnginePrivate::bufferSize);
        }
        for (auto *source : std::as_const(d->stereoSources)) {
            auto *sp = QAmbientSoundPrivate::get(source);
            if (!sp)
                continue;
            float buf[2*QAudioEnginePrivate::bufferSize];
            sp->getBuffer(buf, QAudioEnginePrivate::bufferSize, 2);
            d->resonanceAudio->api->SetInterleavedBuffer(sp->sourceId, buf, 2, QAudioEnginePrivate::bufferSize);
        }

        if (ambisonicDecoder && d->outputMode == QAudioEngine::Surround) {
            const float *channels[QAmbisonicDecoder::maxAmbisonicChannels];
            const float *reverbBuffers[2];
            int nSamples = d->resonanceAudio->getAmbisonicOutput(channels, reverbBuffers, ambisonicDecoder->nInputChannels());
            Q_ASSERT(ambisonicDecoder->nOutputChannels() <= 8);
            ambisonicDecoder->processBufferWithReverb(channels, reverbBuffers, fd, nSamples);
        } else {
            ok = d->resonanceAudio->api->FillInterleavedOutputBuffer(2, QAudioEnginePrivate::bufferSize, fd);
            if (!ok) {
                // If we get here, it means that resonanceAudio did not actually fill the buffer.
                // Sometimes this is expected, for example if resonanceAudio does not have any sources.
                // In this case we just fill the buffer with silence.
                if (d->sources.isEmpty() && d->stereoSources.isEmpty()) {
                    memset(fd, 0, nChannels * QAudioEnginePrivate::bufferSize * sizeof(short));
                } else {
                    // If we get here, it means that something unexpected happened, so bail.
                    qWarning() << "    Reading failed!";
                    break;
                }
            }
        }
        fd += nChannels*QAudioEnginePrivate::bufferSize;
        frames -= QAudioEnginePrivate::bufferSize;
    }
    const int bytesProcessed = ((char *)fd - data);
    m_pos += bytesProcessed;
    return bytesProcessed;
}

QAudioEnginePrivate::QAudioEnginePrivate(QAudioEngine *q) : q(q)
{
    audioThread.setObjectName(u"QAudioThread");
    device = QMediaDevices::defaultAudioOutput();
}

QAudioEnginePrivate::~QAudioEnginePrivate()
{
    resonanceAudio = {};
}

void QAudioEnginePrivate::start()
{
    if (outputStream)
        // already started
        return;

    resonanceAudio->api->SetStereoSpeakerMode(outputMode != QAudioEngine::Headphone);
    resonanceAudio->api->SetMasterVolume(masterVolume);

    outputStream = std::make_unique<QAudioOutputStream>(this);
    outputStream->moveToThread(&audioThread);
    audioThread.start(QThread::TimeCriticalPriority);

    QMetaObject::invokeMethod(outputStream.get(), &QAudioOutputStream::startOutput);
}

void QAudioEnginePrivate::stop()
{
    QMetaObject::invokeMethod(outputStream.get(), &QAudioOutputStream::stopOutput,
                              Qt::BlockingQueuedConnection);
    outputStream.reset();
    audioThread.exit(0);
    audioThread.wait();
}

void QAudioEnginePrivate::setPaused(bool paused)
{
    bool old = this->paused.fetchAndStoreRelaxed(paused);
    if (old != paused) {
        if (outputStream)
            outputStream->setPaused(paused);
        emit q->pausedChanged();
    }
}

void QAudioEnginePrivate::setOutputDevice(const QAudioDevice &device)
{
    if (this->device == device)
        return;
    if (resonanceAudio->api) {
        qWarning() << "Changing device on a running engine not implemented";
        return;
    }
    this->device = device;
    emit q->outputDeviceChanged();
}

void QAudioEnginePrivate::setOutputMode(QAudioEngine::OutputMode mode)
{
    if (outputMode == mode)
        return;
    outputMode = mode;
    if (resonanceAudio->api)
        resonanceAudio->api->SetStereoSpeakerMode(mode != QAudioEngine::Headphone);

    QMetaObject::invokeMethod(outputStream.get(), &QAudioOutputStream::restartOutput,
                              Qt::BlockingQueuedConnection);

    emit q->outputModeChanged();
}

void QAudioEnginePrivate::addSpatialSound(QSpatialSound *sound)
{
    QMutexLocker l(&mutex);
    QAmbientSoundPrivate *sd = QAmbientSoundPrivate::get(sound);

    sd->sourceId = resonanceAudio->api->CreateSoundObjectSource(vraudio::kBinauralHighQuality);
    sources.append(sound);
}

void QAudioEnginePrivate::removeSpatialSound(QSpatialSound *sound)
{
    QMutexLocker l(&mutex);
    QAmbientSoundPrivate *sd = QAmbientSoundPrivate::get(sound);

    resonanceAudio->api->DestroySource(sd->sourceId);
    sd->sourceId = vraudio::ResonanceAudioApi::kInvalidSourceId;
    sources.removeOne(sound);
}

void QAudioEnginePrivate::addStereoSound(QAmbientSound *sound)
{
    QMutexLocker l(&mutex);
    QAmbientSoundPrivate *sd = QAmbientSoundPrivate::get(sound);

    sd->sourceId = resonanceAudio->api->CreateStereoSource(2);
    stereoSources.append(sound);
}

void QAudioEnginePrivate::removeStereoSound(QAmbientSound *sound)
{
    QMutexLocker l(&mutex);
    QAmbientSoundPrivate *sd = QAmbientSoundPrivate::get(sound);

    resonanceAudio->api->DestroySource(sd->sourceId);
    sd->sourceId = vraudio::ResonanceAudioApi::kInvalidSourceId;
    stereoSources.removeOne(sound);
}

void QAudioEnginePrivate::addRoom(QAudioRoom *room)
{
    QMutexLocker l(&mutex);
    rooms.append(room);
}

void QAudioEnginePrivate::removeRoom(QAudioRoom *room)
{
    QMutexLocker l(&mutex);
    rooms.removeOne(room);
}

// This method is called from the audio thread
void QAudioEnginePrivate::updateRooms()
{
    if (!roomEffectsEnabled)
        return;

    bool needUpdate = listenerPositionDirty;
    listenerPositionDirty = false;

    bool roomDirty = false;
    for (const auto &room : std::as_const(rooms)) {
        auto *rd = QAudioRoomPrivate::get(room);
        if (rd->dirty) {
            roomDirty = true;
            rd->update();
            needUpdate = true;
        }
    }

    if (!needUpdate)
        return;

    QVector3D listenerPos = listenerPosition();
    float roomVolume = float(qInf());
    QAudioRoom *room = nullptr;
    // Find the smallest room that contains the listener and apply its room effects
    for (auto *r : std::as_const(rooms)) {
        QVector3D dim2 = r->dimensions()/2.;
        float vol = dim2.x()*dim2.y()*dim2.z();
        if (vol > roomVolume)
            continue;
        QVector3D dist = r->position() - listenerPos;
        // transform into room coordinates
        dist = r->rotation().rotatedVector(dist);
        if (qAbs(dist.x()) <= dim2.x() &&
            qAbs(dist.y()) <= dim2.y() &&
            qAbs(dist.z()) <= dim2.z()) {
            room = r;
            roomVolume = vol;
        }
    }
    if (room != currentRoom)
        roomDirty = true;
    const bool previousRoom = currentRoom;
    currentRoom = room;

    if (!roomDirty)
        return;

    // apply room to engine
    if (!currentRoom) {
        resonanceAudio->api->EnableRoomEffects(false);
        return;
    }
    if (!previousRoom)
        resonanceAudio->api->EnableRoomEffects(true);

    QAudioRoomPrivate *rp = QAudioRoomPrivate::get(room);
    resonanceAudio->api->SetReflectionProperties(rp->reflections);
    resonanceAudio->api->SetReverbProperties(rp->reverb);

    // update room effects for all sound sources
    for (auto *s : std::as_const(sources)) {
        auto *sp = QSpatialSoundPrivate::get(s);
        if (!sp)
            continue;
        sp->updateRoomEffects();
    }
}

QVector3D QAudioEnginePrivate::listenerPosition() const
{
    return listener ? listener->position() : QVector3D();
}

/*!
    \class QAudioEngine
    \inmodule QtSpatialAudio
    \ingroup spatialaudio
    \ingroup multimedia_audio

    \brief QAudioEngine manages a three dimensional sound field.

    You can use an instance of QAudioEngine to manage a sound field in
    three dimensions. A sound field is defined by several QSpatialSound
    objects that define a sound at a specified location in 3D space. You can also
    add stereo overlays using QAmbientSound.

    You can use QAudioListener to define the position of the person listening
    to the sound field relative to the sound sources. Sound sources will be less audible
    if the listener is further away from source. They will also get mapped to the corresponding
    loudspeakers depending on the direction between listener and source.

    QAudioEngine offers two output modes. The first mode renders the sound field to a set of
    speakers, either a stereo speaker pair or a surround configuration. The second mode provides
    an immersive 3D sound experience when using headphones.

    Perception of sound localization is driven mainly by two factors. The first factor is timing
    differences of the sound waves between left and right ear. The second factor comes from various
    ways how sounds coming from different direcations create different types of reflections from our
    ears and heads. See https://en.wikipedia.org/wiki/Sound_localization for more details.

    The spatial audio engine emulates those timing differences and reflections through
    Head related transfer functions (HRTF, see
    https://en.wikipedia.org/wiki/Head-related_transfer_function). The functions used emulates those
    effects for an average persons ears and head. It provides a good and immersive 3D sound localization
    experience for most persons when using headphones.

    The engine is rather versatile allowing you to define room properties and reverb settings to emulate
    different types of rooms.

    Sound sources can also be occluded dampening the sound coming from those sources.

    The audio engine uses a coordinate system that is in centimeters by default. The axes are aligned with the
    typical coordinate system used in 3D. Positive x points to the right, positive y points up and positive z points
    backwards.

*/

/*!
    \fn QAudioEngine::QAudioEngine()
    \fn QAudioEngine::QAudioEngine(QObject *parent)
    \fn QAudioEngine::QAudioEngine(int sampleRate, QObject *parent = nullptr)

    Constructs a spatial audio engine with \a parent, if any.

    The engine will operate with a sample rate given by \a sampleRate. The
    default sample rate, if none is provided, is 44100 (44.1kHz).

    Sound content that is not provided at that sample rate will automatically
    get resampled to \a sampleRate when being processed by the engine. The
    default sample rate is fine in most cases, but you can define a different
    rate if most of your sound files are sampled with a different rate, and
    avoid some CPU overhead for resampling.
 */
QAudioEngine::QAudioEngine(int sampleRate, QObject *parent)
    : QObject(parent), d(new QAudioEnginePrivate(this))
{
    d->sampleRate = sampleRate;
    d->resonanceAudio = std::make_unique<vraudio::ResonanceAudio>(
            2, QAudioEnginePrivate::bufferSize, d->sampleRate);
}

/*!
    Destroys the spatial audio engine.
 */
QAudioEngine::~QAudioEngine()
{
    stop();
    delete d;
}

/*! \enum QAudioEngine::OutputMode
    \value Surround Map the sounds to the loudspeaker configuration of the output device.
        This is normally a stereo or surround speaker setup.
    \value Stereo Map the sounds to the stereo loudspeaker configuration of the output device.
        This will ignore any additional speakers and only use the left and right channels
        to create a stero rendering of the sound field.
    \value Headphone Use Headphone spatialization to create a 3D audio effect when listening
        to the sound field through headphones
*/

/*!
    \property QAudioEngine::outputMode

    Sets or retrieves the current output mode of the engine.

    \sa QAudioEngine::OutputMode
 */
void QAudioEngine::setOutputMode(OutputMode mode)
{
    d->setOutputMode(mode);
}

QAudioEngine::OutputMode QAudioEngine::outputMode() const
{
    return d->outputMode;
}

/*!
    Returns the sample rate the engine has been configured with.
 */
int QAudioEngine::sampleRate() const
{
    return d->sampleRate;
}

/*!
    \property QAudioEngine::outputDevice

    Sets or returns the device that is being used for playing the sound field.
 */
void QAudioEngine::setOutputDevice(const QAudioDevice &device)
{
    d->setOutputDevice(device);
}

QAudioDevice QAudioEngine::outputDevice() const
{
    return d->device;
}

/*!
    \property QAudioEngine::masterVolume

    Sets or returns volume being used to render the sound field.
 */
void QAudioEngine::setMasterVolume(float volume)
{
    if (d->masterVolume == volume)
        return;
    d->masterVolume = volume;
    d->resonanceAudio->api->SetMasterVolume(volume);
    emit masterVolumeChanged();
}

float QAudioEngine::masterVolume() const
{
    return d->masterVolume;
}

/*!
    Starts the engine.
 */
void QAudioEngine::start()
{
    d->start();
}

/*!
    Stops the engine.
 */
void QAudioEngine::stop()
{
    d->stop();
}

/*!
    \property QAudioEngine::paused

    Pauses the spatial audio engine.
 */
void QAudioEngine::setPaused(bool paused)
{
    d->setPaused(paused);
}

bool QAudioEngine::paused() const
{
    return d->paused.loadRelaxed();
}

/*!
    Enables room effects such as echos and reverb.

    Enables room effects if \a enabled is true.
    Room effects will only apply if you create one or more \l QAudioRoom objects
    and the listener is inside at least one of the rooms. If the listener is inside
    multiple rooms, the room with the smallest volume will be used.
 */
void QAudioEngine::setRoomEffectsEnabled(bool enabled)
{
    if (d->roomEffectsEnabled == enabled)
        return;
    d->roomEffectsEnabled = enabled;
    d->resonanceAudio->roomEffectsEnabled = enabled;
}

/*!
    Returns true if room effects are enabled.
 */
bool QAudioEngine::roomEffectsEnabled() const
{
    return d->roomEffectsEnabled;
}

/*!
    \property QAudioEngine::distanceScale

    Defines the scale of the coordinate system being used by the spatial audio engine.
    By default, all units are in centimeters, in line with the default units being
    used by Qt Quick 3D.

    Set the distance scale to QAudioEngine::DistanceScaleMeter to get units in meters.
*/
void QAudioEngine::setDistanceScale(float scale)
{
    // multiply with 100, to get the conversion to meters that resonance audio uses
    scale /= 100.f;
    if (scale <= 0.0f) {
        qWarning() << "QAudioEngine: Invalid distance scale.";
        return;
    }
    if (scale == d->distanceScale)
        return;
    d->distanceScale = scale;
    emit distanceScaleChanged();
}

float QAudioEngine::distanceScale() const
{
    return d->distanceScale*100.f;
}

/*!
    \fn void QAudioEngine::pause()

    Pauses playback.
*/
/*!
    \fn void QAudioEngine::resume()

    Resumes playback.
*/
/*!
    \variable QAudioEngine::DistanceScaleCentimeter
    \internal
*/
/*!
    \variable QAudioEngine::DistanceScaleMeter
    \internal
*/

QT_END_NAMESPACE

#include "moc_qaudioengine.cpp"
