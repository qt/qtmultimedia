/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Spatial Audio module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-NOGPL2$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qspatialaudioengine_p.h>
#include <qspatialaudiosoundsource_p.h>
#include <qspatialaudiostereosource.h>
#include <resonance_audio_api_extensions.h>
#include <qambisonicdecoder_p.h>
#include <qaudiodecoder.h>
#include <qmediadevices.h>
#include <qiodevice.h>
#include <qaudiosink.h>
#include <qdebug.h>
#include <qelapsedtimer.h>

QT_BEGIN_NAMESPACE

class QAudioOutputStream : public QIODevice
{
    Q_OBJECT
public:
    explicit QAudioOutputStream(QSpatialAudioEnginePrivate *d)
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
        d->ambisonicDecoder.reset(new QAmbisonicDecoder(QAmbisonicDecoder::HighQuality, d->format));
        sink.reset(new QAudioSink(d->device, d->format));
        sink->setBufferSize(16384);
        sink->start(this);
    }

    Q_INVOKABLE void stopOutput() {
        sink->stop();
        sink.reset();
        d->ambisonicDecoder.reset();
        delete this;
    }

private:
    qint64 m_pos = 0;
    QSpatialAudioEnginePrivate *d = nullptr;
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
    int nChannels = d->ambisonicDecoder ? d->ambisonicDecoder->nOutputChannels() : 2;
    if (len < nChannels*int(sizeof(float))*QSpatialAudioEnginePrivate::bufferSize)
        return 0;

    short *fd = (short *)data;
    qint64 frames = len / nChannels / sizeof(short);
    bool ok = true;
    while (frames >= qint64(QSpatialAudioEnginePrivate::bufferSize)) {
        // Fill input buffers
        for (auto *source : qAsConst(d->sources)) {
            auto *sp = QSpatialAudioSoundSourcePrivate::get(source);
            float buf[QSpatialAudioEnginePrivate::bufferSize];
            sp->getBuffer(buf, QSpatialAudioEnginePrivate::bufferSize, 1);
            d->api->SetInterleavedBuffer(sp->sourceId, buf, 1, QSpatialAudioEnginePrivate::bufferSize);
        }
        for (auto *source : qAsConst(d->stereoSources)) {
            auto *sp = QSpatialAudioSound::get(source);
            float buf[2*QSpatialAudioEnginePrivate::bufferSize];
            sp->getBuffer(buf, QSpatialAudioEnginePrivate::bufferSize, 2);
            d->api->SetInterleavedBuffer(sp->sourceId, buf, 2, QSpatialAudioEnginePrivate::bufferSize);
        }

        if (d->ambisonicDecoder && d->outputMode == QSpatialAudioEngine::Normal && d->format.channelCount() != 2) {
            const float *channels[QAmbisonicDecoder::maxAmbisonicChannels];
            int nSamples = vraudio::getAmbisonicOutput(d->api, channels, d->ambisonicDecoder->nInputChannels());
            Q_ASSERT(d->ambisonicDecoder->nOutputChannels() <= 8);
            d->ambisonicDecoder->processBuffer(channels, fd, nSamples);
        } else {
            ok = d->api->FillInterleavedOutputBuffer(2, QSpatialAudioEnginePrivate::bufferSize, fd);
            if (!ok) {
                qWarning() << "    Reading failed!";
                break;
            }
        }
        fd += nChannels*QSpatialAudioEnginePrivate::bufferSize;
        frames -= QSpatialAudioEnginePrivate::bufferSize;
    }
    const int bytesProcessed = ((char *)fd - data);
    m_pos += bytesProcessed;
    return bytesProcessed;
}


QSpatialAudioEnginePrivate::QSpatialAudioEnginePrivate()
{
    device = QMediaDevices::defaultAudioOutput();
}

QSpatialAudioEnginePrivate::~QSpatialAudioEnginePrivate()
{
    delete api;
}

void QSpatialAudioEnginePrivate::addSpatialSound(QSpatialAudioSoundSource *sound)
{
    QSpatialAudioSound *sd = QSpatialAudioSound::get(sound);

    sd->sourceId = api->CreateSoundObjectSource(vraudio::kBinauralHighQuality);
    sources.append(sound);
}

void QSpatialAudioEnginePrivate::removeSpatialSound(QSpatialAudioSoundSource *sound)
{
    QSpatialAudioSound *sd = QSpatialAudioSound::get(sound);

    api->DestroySource(sd->sourceId);
    sd->sourceId = vraudio::ResonanceAudioApi::kInvalidSourceId;
    sources.removeOne(sound);
}

void QSpatialAudioEnginePrivate::addStereoSound(QSpatialAudioStereoSource *sound)
{
    QSpatialAudioSound *sd = QSpatialAudioSound::get(sound);

    sd->sourceId = api->CreateStereoSource(2);
    stereoSources.append(sound);
}

void QSpatialAudioEnginePrivate::removeStereoSound(QSpatialAudioStereoSource *sound)
{
    QSpatialAudioSound *sd = QSpatialAudioSound::get(sound);

    api->DestroySource(sd->sourceId);
    sd->sourceId = vraudio::ResonanceAudioApi::kInvalidSourceId;
    stereoSources.removeOne(sound);
}


/*!
    \class QSpatialAudioEngine

    \brief QSpatialAudioEngine manages a three dimensional sound field

    You can use an instance of QSpatialAudioEngine to manage a sound field in
    three dimensions. A sound field is defined by several QSpatialAudioSoundSource
    objects that define a sound at a specified location in 3D space. You can also
    add stereo overlays using QSpatialAudioStereoSource.

    You can use QSpatialAudioListener to define the position of the person listening
    to the sound field relative to the sound sources. Sound sources will be less audible
    if the listener is further away from source. They will also get mapped to the corresponding
    loudspeakers depending on the direction between listener and source.

    QSpatialAudioEngine does offer a mode where Qt is using simulating the effects of the ear
    using head related impulse reponse functions (see also https://en.wikipedia.org/wiki/Sound_localization)
    to localize the sound in 3D space when using headphones and create a spatial audio effect through
    headphones.

    The engine is rather versatile allowing you to define amd emulate room properties and reverb settings emulating
    different types of rooms.

    Sound sources can also be occluded dampening the sound coming from those sources.

*/

/*!
    Constructs a spatial audio engine with \a parent.

    The engine will operate with a sample rate given by \a sampleRate. Sound content that is
    not provided at that sample rate will automatically get resampled to \a sampleRate when
    being processed by the engine. The default sample rate is fine in most cases, but you can define
    a different rate if most of your sound files are sampled with a different rate, and avoid some
    CPU overhead for resampling.
 */
QSpatialAudioEngine::QSpatialAudioEngine(QObject *parent, int sampleRate)
    : QObject(parent)
    , d(new QSpatialAudioEnginePrivate)
{
    d->sampleRate = sampleRate;
    d->api = vraudio::CreateResonanceAudioApi(2, QSpatialAudioEnginePrivate::bufferSize, d->sampleRate);
}

/*!
    Destroys the spatial audio engine.
 */
QSpatialAudioEngine::~QSpatialAudioEngine()
{
    stop();
    delete d;
}

/*! \enum QSpatialAudioEngine::OutputMode
    \value Normal Map the sounds to the loudspeaker configuration of the output device.
    This is normally a stereo or surround speaker setup.
    \value Headphone Use Headphone spatialization to create a 3D audio effect when listening
    to the sound field through headphones
*/

/*!
    \property QSpatialAudioEngine::outputMode

    Sets or retrieves the current output mode of the engine.

    \sa QSpatialAudioEngine::OutputMode
 */
void QSpatialAudioEngine::setOutputMode(OutputMode mode)
{
    if (d->outputMode == mode)
        return;
    d->outputMode = mode;
    if (d->api) {
        d->api->SetStereoSpeakerMode(mode == Normal);
    }
    emit outputModeChanged();
}

QSpatialAudioEngine::OutputMode QSpatialAudioEngine::outputMode() const
{
    return d->outputMode;
}

/*!
    Returns the sample rate the engine has been configured with.
 */
int QSpatialAudioEngine::sampleRate() const
{
    return d->sampleRate;
}

/*!
    \property QSpatialAudioEngine::outputDevice

    Sets or returns the device that is being used for outputting the sound field.
 */
void QSpatialAudioEngine::setOutputDevice(const QAudioDevice &device)
{
    if (d->device == device)
        return;
    if (d->api) {
        qWarning() << "Changing device on a running engine not implemented";
        return;
    }
    d->device = device;
    emit outputDeviceChanged();
}

QAudioDevice QSpatialAudioEngine::outputDevice() const
{
    return d->device;
}

/*!
    \property QSpatialAudioEngine::masterVolume

    Sets or returns overall volume being used to render the sound field.
 */
void QSpatialAudioEngine::setMasterVolume(float volume)
{
    if (d->masterVolume == volume)
        return;
    d->masterVolume = volume;
    d->api->SetMasterVolume(volume);
    emit masterVolumeChanged();
}

float QSpatialAudioEngine::masterVolume() const
{
    return d->masterVolume;
}

/*!
    Starts the engine.
 */
void QSpatialAudioEngine::start()
{
    if (d->outputStream)
        // already started
        return;

    d->format.setChannelCount(2);
    d->format.setSampleRate(d->sampleRate);
    d->format.setSampleFormat(QAudioFormat::Int16);

    d->api->SetStereoSpeakerMode(d->outputMode == Normal);
    d->api->EnableRoomEffects(true);
    d->api->SetMasterVolume(d->masterVolume);

    d->outputStream.reset(new QAudioOutputStream(d));
    d->outputStream->moveToThread(&d->audioThread);
    d->audioThread.start();

    QMetaObject::invokeMethod(d->outputStream.get(), "startOutput");
}

/*!
    Stops the engine.
 */
void QSpatialAudioEngine::stop()
{
    QMetaObject::invokeMethod(d->outputStream.get(), "stopOutput", Qt::BlockingQueuedConnection);
    d->outputStream.reset();
    d->audioThread.exit(0);
    d->audioThread.wait();
    delete d->api;
    d->api = nullptr;
}

/*!
    Enables room effects such as echos and reverb.
 */
void QSpatialAudioEngine::setRoomEffectsEnabled(bool enabled)
{
    if (d->roomEffectsEnabled == enabled)
        return;
    d->roomEffectsEnabled = enabled;
    d->api->EnableRoomEffects(d->roomEffectsEnabled);
}

/*!
    Returns true if room effects are enabled.
 */
bool QSpatialAudioEngine::roomEffectsEnabled() const
{
    return d->roomEffectsEnabled;
}


/*! \class QSpatialAudioSound
    \internal
 */

void QSpatialAudioSound::load()
{
    decoder.reset(new QAudioDecoder);
    buffers.clear();
    auto *ep = QSpatialAudioEnginePrivate::get(engine);
    QAudioFormat f = ep->format;
    f.setSampleFormat(QAudioFormat::Float);
    f.setChannelConfig(nchannels == 2 ? QAudioFormat::ChannelConfigStereo : QAudioFormat::ChannelConfigMono);
    decoder->setAudioFormat(f);
    decoder->setSource(url);

    connect(decoder.get(), &QAudioDecoder::bufferReady, this, &QSpatialAudioSound::bufferReady);
    connect(decoder.get(), &QAudioDecoder::finished, this, &QSpatialAudioSound::finished);
    decoder->start();
}

void QSpatialAudioSound::getBuffer(float *buf, int nframes, int channels)
{
    Q_ASSERT(channels == nchannels);
    QMutexLocker l(&mutex);
    if (currentBuffer >= buffers.size()) {
        memset(buf, 0, nframes*sizeof(float));
    } else {
        int frames = nframes;
        float *ff = buf;
        while (frames) {
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
            if (currentBuffer == buffers.size()) {
                currentBuffer = 0;
            }
        }
        Q_ASSERT(ff - buf == channels*nframes);
    }
}

void QSpatialAudioSound::bufferReady()
{
    QMutexLocker l(&mutex);
    auto b = decoder->read();
//    qDebug() << "read buffer" << b.format() << b.startTime() << b.duration();
    buffers.append(b);
}

void QSpatialAudioSound::finished()
{

}

QT_END_NAMESPACE

#include "moc_qspatialaudioengine.cpp"
#include "qspatialaudioengine.moc"
