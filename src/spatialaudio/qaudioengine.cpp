// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#include <qaudioengine_p.h>
#include <qspatialsound_p.h>
#include <qambientsound.h>
#include <qaudioroom_p.h>
#include <qaudiolistener.h>
#include <resonance_audio.h>
#include <qambisonicdecoder_p.h>
#include <qaudiodecoder.h>
#include <qmediadevices.h>
#include <qiodevice.h>
#include <qaudiosink.h>
#include <qdebug.h>
#include <qelapsedtimer.h>

#include <QFile>

QT_BEGIN_NAMESPACE

// We'd like to have short buffer times, so the sound adjusts itself to changes
// quickly, but times below 100ms seem to give stuttering on macOS.
// It might be possible to set this value lower on other OSes.
const int bufferTimeMs = 100;

class QAudioOutputStream : public QIODevice
{
    Q_OBJECT
public:
    explicit QAudioOutputStream(QAudioEnginePrivate *d)
        : d(d)
    {
        open(QIODevice::ReadOnly);
    }
    ~QAudioOutputStream();

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

    Q_INVOKABLE void startOutput() {
        QMutexLocker l(&d->mutex);
        Q_ASSERT(!sink);
        QAudioFormat format;
        format.setChannelConfig(d->outputMode == QAudioEngine::Surround ?
                                    d->device.channelConfiguration() : QAudioFormat::ChannelConfigStereo);
        format.setSampleRate(d->sampleRate);
        format.setSampleFormat(QAudioFormat::Int16);
        d->ambisonicDecoder.reset(new QAmbisonicDecoder(QAmbisonicDecoder::HighQuality, format));
        sink.reset(new QAudioSink(d->device, format));
        sink->setBufferSize(d->sampleRate*bufferTimeMs/1000*sizeof(qint16)*format.channelCount());
        sink->start(this);
    }

    Q_INVOKABLE void stopOutput() {
        sink->stop();
        sink.reset();
        d->ambisonicDecoder.reset();
    }

    Q_INVOKABLE void restartOutput() {
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

    d->updateRooms();

    int nChannels = d->ambisonicDecoder ? d->ambisonicDecoder->nOutputChannels() : 2;
    if (len < nChannels*int(sizeof(float))*QAudioEnginePrivate::bufferSize)
        return 0;

    short *fd = (short *)data;
    qint64 frames = len / nChannels / sizeof(short);
    bool ok = true;
    while (frames >= qint64(QAudioEnginePrivate::bufferSize)) {
        // Fill input buffers
        for (auto *source : std::as_const(d->sources)) {
            auto *sp = QSpatialSoundPrivate::get(source);
            float buf[QAudioEnginePrivate::bufferSize];
            sp->getBuffer(buf, QAudioEnginePrivate::bufferSize, 1);
            d->resonanceAudio->api->SetInterleavedBuffer(sp->sourceId, buf, 1, QAudioEnginePrivate::bufferSize);
        }
        for (auto *source : std::as_const(d->stereoSources)) {
            auto *sp = QAmbientSoundPrivate::get(source);
            float buf[2*QAudioEnginePrivate::bufferSize];
            sp->getBuffer(buf, QAudioEnginePrivate::bufferSize, 2);
            d->resonanceAudio->api->SetInterleavedBuffer(sp->sourceId, buf, 2, QAudioEnginePrivate::bufferSize);
        }

        if (d->ambisonicDecoder && d->outputMode == QAudioEngine::Surround) {
            const float *channels[QAmbisonicDecoder::maxAmbisonicChannels];
            const float *reverbBuffers[2];
            int nSamples = d->resonanceAudio->getAmbisonicOutput(channels, reverbBuffers, d->ambisonicDecoder->nInputChannels());
            Q_ASSERT(d->ambisonicDecoder->nOutputChannels() <= 8);
            d->ambisonicDecoder->processBufferWithReverb(channels, reverbBuffers, fd, nSamples);
        } else {
            ok = d->resonanceAudio->api->FillInterleavedOutputBuffer(2, QAudioEnginePrivate::bufferSize, fd);
            if (!ok) {
                qWarning() << "    Reading failed!";
                break;
            }
        }
        fd += nChannels*QAudioEnginePrivate::bufferSize;
        frames -= QAudioEnginePrivate::bufferSize;
    }
    const int bytesProcessed = ((char *)fd - data);
    m_pos += bytesProcessed;
    return bytesProcessed;
}


QAudioEnginePrivate::QAudioEnginePrivate()
{
    device = QMediaDevices::defaultAudioOutput();
    audioThread.setPriority(QThread::TimeCriticalPriority);
}

QAudioEnginePrivate::~QAudioEnginePrivate()
{
    delete resonanceAudio;
}

void QAudioEnginePrivate::addSpatialSound(QSpatialSound *sound)
{
    QAmbientSoundPrivate *sd = QAmbientSoundPrivate::get(sound);

    sd->sourceId = resonanceAudio->api->CreateSoundObjectSource(vraudio::kBinauralHighQuality);
    sources.append(sound);
}

void QAudioEnginePrivate::removeSpatialSound(QSpatialSound *sound)
{
    QAmbientSoundPrivate *sd = QAmbientSoundPrivate::get(sound);

    resonanceAudio->api->DestroySource(sd->sourceId);
    sd->sourceId = vraudio::ResonanceAudioApi::kInvalidSourceId;
    sources.removeOne(sound);
}

void QAudioEnginePrivate::addStereoSound(QAmbientSound *sound)
{
    QAmbientSoundPrivate *sd = QAmbientSoundPrivate::get(sound);

    sd->sourceId = resonanceAudio->api->CreateStereoSource(2);
    stereoSources.append(sound);
}

void QAudioEnginePrivate::removeStereoSound(QAmbientSound *sound)
{
    QAmbientSoundPrivate *sd = QAmbientSoundPrivate::get(sound);

    resonanceAudio->api->DestroySource(sd->sourceId);
    sd->sourceId = vraudio::ResonanceAudioApi::kInvalidSourceId;
    stereoSources.removeOne(sound);
}

void QAudioEnginePrivate::addRoom(QAudioRoom *room)
{
    rooms.append(room);
}

void QAudioEnginePrivate::removeRoom(QAudioRoom *room)
{
    rooms.removeOne(room);
}

void QAudioEnginePrivate::updateRooms()
{
    if (!roomEffectsEnabled)
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
    : QObject(parent)
    , d(new QAudioEnginePrivate)
{
    d->sampleRate = sampleRate;
    d->resonanceAudio = new vraudio::ResonanceAudio(2, QAudioEnginePrivate::bufferSize, d->sampleRate);
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
    if (d->outputMode == mode)
        return;
    d->outputMode = mode;
    if (d->resonanceAudio->api)
        d->resonanceAudio->api->SetStereoSpeakerMode(mode != Headphone);

    QMetaObject::invokeMethod(d->outputStream.get(), "restartOutput", Qt::BlockingQueuedConnection);

    emit outputModeChanged();
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
    if (d->device == device)
        return;
    if (d->resonanceAudio->api) {
        qWarning() << "Changing device on a running engine not implemented";
        return;
    }
    d->device = device;
    emit outputDeviceChanged();
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
    if (d->outputStream)
        // already started
        return;

    d->resonanceAudio->api->SetStereoSpeakerMode(d->outputMode != Headphone);
    d->resonanceAudio->api->SetMasterVolume(d->masterVolume);

    d->outputStream.reset(new QAudioOutputStream(d));
    d->outputStream->moveToThread(&d->audioThread);
    d->audioThread.start();

    QMetaObject::invokeMethod(d->outputStream.get(), "startOutput");
}

/*!
    Stops the engine.
 */
void QAudioEngine::stop()
{
    QMetaObject::invokeMethod(d->outputStream.get(), "stopOutput", Qt::BlockingQueuedConnection);
    d->outputStream.reset();
    d->audioThread.exit(0);
    d->audioThread.wait();
    delete d->resonanceAudio->api;
    d->resonanceAudio->api = nullptr;
}

/*!
    \property QAudioEngine::paused

    Pauses the spatial audio engine.
 */
void QAudioEngine::setPaused(bool paused)
{
    bool old = d->paused.fetchAndStoreRelaxed(paused);
    if (old != paused) {
        if (d->outputStream)
            d->outputStream->setPaused(paused);
        emit pausedChanged();
    }
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


void QAmbientSoundPrivate::load()
{
    decoder.reset(new QAudioDecoder);
    buffers.clear();
    currentBuffer = 0;
    sourceDeviceFile.reset(nullptr);
    bufPos = 0;
    m_playing = false;
    m_loading = true;
    auto *ep = QAudioEnginePrivate::get(engine);
    QAudioFormat f;
    f.setSampleFormat(QAudioFormat::Float);
    f.setSampleRate(ep->sampleRate);
    f.setChannelConfig(nchannels == 2 ? QAudioFormat::ChannelConfigStereo : QAudioFormat::ChannelConfigMono);
    decoder->setAudioFormat(f);
    if (url.scheme().compare(u"qrc", Qt::CaseInsensitive) == 0) {
        auto qrcFile = std::make_unique<QFile>(u':' + url.path());
        if (!qrcFile->open(QFile::ReadOnly))
            return;
        sourceDeviceFile = std::move(qrcFile);
        decoder->setSourceDevice(sourceDeviceFile.get());
    } else {
        decoder->setSource(url);
    }
    connect(decoder.get(), &QAudioDecoder::bufferReady, this, &QAmbientSoundPrivate::bufferReady);
    connect(decoder.get(), &QAudioDecoder::finished, this, &QAmbientSoundPrivate::finished);
    decoder->start();
}

void QAmbientSoundPrivate::getBuffer(float *buf, int nframes, int channels)
{
    Q_ASSERT(channels == nchannels);
    QMutexLocker l(&mutex);
    if (!m_playing || currentBuffer >= buffers.size()) {
        memset(buf, 0, channels * nframes * sizeof(float));
    } else {
        int frames = nframes;
        float *ff = buf;
        while (frames) {
            if (currentBuffer < buffers.size()) {
                const QAudioBuffer &b = buffers.at(currentBuffer);
//            qDebug() << s << b.format().sampleRate() << b.format().channelCount() << b.format().sampleFormat();
                auto *f = b.constData<float>() + bufPos*nchannels;
                int toCopy = qMin(b.frameCount() - bufPos, frames);
                memcpy(ff, f, toCopy*sizeof(float)*nchannels);
                ff += toCopy*nchannels;
                frames -= toCopy;
                bufPos += toCopy;
                Q_ASSERT(bufPos <= b.frameCount());
                if (bufPos == b.frameCount()) {
                    ++currentBuffer;
                    bufPos = 0;
                }
            } else {
                // no more data available
                if (m_loading)
                    qDebug() << "underrun" << frames << "frames when loading" << url;
                memset(ff, 0, frames * channels * sizeof(float));
                ff += frames * channels;
                frames = 0;
            }
            if (!m_loading) {
                if (currentBuffer == buffers.size()) {
                    currentBuffer = 0;
                    ++m_currentLoop;
                }
                if (m_loops > 0 && m_currentLoop >= m_loops) {
                    m_playing = false;
                    m_currentLoop = 0;
                }
            }
        }
        Q_ASSERT(ff - buf == channels*nframes);
    }
}

void QAmbientSoundPrivate::bufferReady()
{
    QMutexLocker l(&mutex);
    auto b = decoder->read();
//    qDebug() << "read buffer" << b.format() << b.startTime() << b.duration();
    buffers.append(b);
    if (m_autoPlay)
        m_playing = true;
}

void QAmbientSoundPrivate::finished()
{
    m_loading = false;
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
#include "qaudioengine.moc"
