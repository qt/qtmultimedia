// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#include "qambientsound.h"
#include "qambientsound_p.h"
#include "qaudioengine_p.h"
#include "resonance_audio.h"
#include <qaudiosink.h>
#include <qurl.h>
#include <qdebug.h>
#include <qaudiodecoder.h>

QT_BEGIN_NAMESPACE

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
    \class QAmbientSound
    \inmodule QtSpatialAudio
    \ingroup spatialaudio
    \ingroup multimedia_audio

    \brief A stereo overlay sound.

    QAmbientSound represents a position and orientation independent sound.
    It's commonly used for background sounds (e.g. music) that is supposed to be independent
    of the listeners position and orientation.
  */

/*!
    Creates a stereo sound source for \a engine.
 */
QAmbientSound::QAmbientSound(QAudioEngine *engine)
    : d(new QAmbientSoundPrivate(this))
{
    setEngine(engine);
}

QAmbientSound::~QAmbientSound()
{
    setEngine(nullptr);
    delete d;
}

/*!
    \property QAmbientSound::volume

    Defines the volume of the sound.

    Values between 0 and 1 will attenuate the sound, while values above 1
    provide an additional gain boost.
 */
void QAmbientSound::setVolume(float volume)
{
    if (d->volume == volume)
        return;
    d->volume = volume;
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->resonanceAudio->api->SetSourceVolume(d->sourceId, d->volume);
    emit volumeChanged();
}

float QAmbientSound::volume() const
{
    return d->volume;
}

void QAmbientSound::setSource(const QUrl &url)
{
    if (d->url == url)
        return;
    d->url = url;

    d->load();
    emit sourceChanged();
}

/*!
    \property QAmbientSound::source

    The source file for the sound to be played.
 */
QUrl QAmbientSound::source() const
{
    return d->url;
}
/*!
    \enum QAmbientSound::Loops

    Lets you control the playback loop using the following values:

    \value Infinite Loops infinitely
    \value Once Stops playback after running once
*/
/*!
   \property QAmbientSound::loops

    Determines how many times the sound is played before the player stops.
    Set to QAmbientSound::Infinite to play the current sound in
    a loop forever.

    The default value is \c 1.
 */
int QAmbientSound::loops() const
{
    return d->m_loops.loadRelaxed();
}

void QAmbientSound::setLoops(int loops)
{
    int oldLoops = d->m_loops.fetchAndStoreRelaxed(loops);
    if (oldLoops != loops)
        emit loopsChanged();
}

/*!
   \property QAmbientSound::autoPlay

    Determines whether the sound should automatically start playing when a source
    gets specified.

    The default value is \c true.
 */
bool QAmbientSound::autoPlay() const
{
    return d->m_autoPlay.loadRelaxed();
}

void QAmbientSound::setAutoPlay(bool autoPlay)
{
    bool old = d->m_autoPlay.fetchAndStoreRelaxed(autoPlay);
    if (old != autoPlay)
        emit autoPlayChanged();
}

/*!
    Starts playing back the sound. Does nothing if the sound is already playing.
 */
void QAmbientSound::play()
{
    d->play();
}

/*!
    Pauses sound playback. Calling play() will continue playback.
 */
void QAmbientSound::pause()
{
    d->pause();
}

/*!
    Stops sound playback and resets the current position and current loop count to 0.
    Calling play() will start playback at the beginning of the sound file.
 */
void QAmbientSound::stop()
{
    d->stop();
}

/*!
    \internal
 */
void QAmbientSound::setEngine(QAudioEngine *engine)
{
    if (d->engine == engine)
        return;

    // Remove self from old engine (if necessary)
    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->removeStereoSound(this);

    d->engine = engine;

    // Add self to new engine if necessary
    ep = QAudioEnginePrivate::get(d->engine);
    if (ep) {
        ep->addStereoSound(this);
        ep->resonanceAudio->api->SetSourceVolume(d->sourceId, d->volume);
    }
}

/*!
    Returns the engine associated with this sound.
 */
QAudioEngine *QAmbientSound::engine() const
{
    return d->engine;
}

QT_END_NAMESPACE

#include "moc_qambientsound.cpp"
