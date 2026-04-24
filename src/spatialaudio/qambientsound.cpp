// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#include "qambientsound.h"

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qurl.h>
#include <QtMultimedia/qaudiodecoder.h>
#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/private/qaudio_qspan_support_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtSpatialAudio/private/qambientsound_p.h>
#include <QtSpatialAudio/private/qaudioengine_p.h>

#include <resonance_audio.h>
#include <memory>

QT_BEGIN_NAMESPACE

QAmbientSoundPrivate::QAmbientSoundPrivate(QAudioEngine *engine, int nchannels)
    : nchannels(nchannels), engine(engine)
{
}

QAmbientSoundPrivate::~QAmbientSoundPrivate() = default;

void QAmbientSoundPrivate::setVolume(float volume)
{
    m_volume = volume;
    applyVolume();
}

void QAmbientSoundPrivate::applyVolume()
{
    auto *ep = QAudioEnginePrivate::get(engine);
    if (ep)
        ep->resonanceAudio->api->SetSourceVolume(sourceId, m_volume);
}

void QAmbientSoundPrivate::loadUrl(const QUrl &url)
{
    m_url = url;

    Q_Q(QAmbientSound);

    {
        QMutexLocker l(&mutex);
        buffers.clear();
        currentBuffer = 0;
        bufPos = 0;
        m_currentLoop = 0;
        m_playing = false;
    }
    m_loadFuture.cancel();

    auto *ep = QAudioEnginePrivate::get(engine);
    QAudioFormat f;
    f.setSampleFormat(QAudioFormat::Float);
    f.setSampleRate(ep->sampleRate());
    f.setChannelConfig(nchannels == 2 ? QAudioFormat::ChannelConfigStereo
                                      : QAudioFormat::ChannelConfigMono);

    QUrl resolved = m_sourceResolver->resolve(url);
    m_loadFuture = load(resolved, f).then(q, [this](QAmbientSoundPrivate::LoadResult result) {
        QMutexLocker l(&mutex);
        if (result) {
            buffers = std::move(*result);
            if (m_autoPlay)
                m_playing = true;
        } else {
            qWarning() << "QAmbientSound: cannot load file";
        }
    });
}

auto QAmbientSoundPrivate::load(QUrl resolvedUrl, QAudioFormat format) -> QFuture<LoadResult>
{
    m_loadFuture.cancelChain();

    auto promise = std::make_shared<QPromise<LoadResult>>();
    auto future = promise->future();

    m_decoder = std::make_unique<QAudioDecoder>();
    m_decoder->setAudioFormat(format);

    std::shared_ptr<QIODevice> file; // kept alive until decoding is finished
    if (resolvedUrl.scheme().compare(u"qrc", Qt::CaseInsensitive) == 0) {
        file = std::make_unique<QFile>(u':' + resolvedUrl.path());
        if (!file->open(QFile::ReadOnly)) {
            promise->start();
            promise->addResult(q23::unexpected{ QAudioDecoder::Error::ResourceError });
            promise->finish();

            m_decoder = {};

            return future;
        }
        m_decoder->setSourceDevice(file.get());
    } else {
        m_decoder->setSource(resolvedUrl);
    }

    auto accum = std::make_shared<QList<QAudioBuffer>>();
    QObject::connect(m_decoder.get(), &QAudioDecoder::bufferReady, m_decoder.get(), [accum, this] {
        accum->append(m_decoder->read());
    });

    QObject::connect(
            m_decoder.get(),
            static_cast<void (QAudioDecoder::*)(QAudioDecoder::Error)>(&QAudioDecoder::error),
            m_decoder.get(), [promise, file](QAudioDecoder::Error error) {
        promise->start();
        promise->addResult(q23::unexpected{ error });
        promise->finish();
    });

    QObject::connect(m_decoder.get(), &QAudioDecoder::finished, m_decoder.get(),
                     [promise, file, accum] {
        promise->start();
        promise->addResult(std::move(*accum));
        promise->finish();
    });

    m_decoder->start();
    return future;
}

void QAmbientSoundPrivate::getBuffer(QSpan<float> output, int nframes, int channels)
{
    Q_ASSERT(channels == nchannels);
    Q_ASSERT(output.size() == channels * nframes);

    QMutexLocker l(&mutex);
    namespace ranges = QtMultimediaPrivate::ranges;

    if (!m_playing || currentBuffer >= buffers.size()) {
        ranges::fill(output, 0.f);
    } else {
        using QtMultimediaPrivate::drop;
        using QtMultimediaPrivate::take;

        int frames = nframes;
        while (frames) {
            if (currentBuffer < buffers.size()) {
                const QAudioBuffer &b = buffers.at(currentBuffer);
                const float *sourceData = b.constData<float>() + bufPos * nchannels;

                // Copy frames
                int framesToCopy = qMin(b.frameCount() - bufPos, frames);
                QSpan<const float> source(sourceData, framesToCopy * nchannels);
                QSpan<float> destination = take(output, framesToCopy * nchannels);
                std::copy(source.begin(), source.end(), destination.begin());

                // Advance output span
                output = drop(output, framesToCopy * nchannels);

                frames -= framesToCopy;
                bufPos += framesToCopy;
                Q_ASSERT(bufPos <= b.frameCount());

                if (bufPos == b.frameCount()) {
                    ++currentBuffer;
                    bufPos = 0;
                }
            } else {
                // Fill remaining with silence
                ranges::fill(output, 0.f);
                return;
            }

            if (currentBuffer == buffers.size()) {
                currentBuffer = 0;
                ++m_currentLoop;
            }
            if (m_loops > 0 && m_currentLoop >= m_loops) {
                m_playing = false;
                m_currentLoop = 0;
            }
        }
        Q_ASSERT(output.size() == 0);
    }
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

    \note Must be called with a valid QAudioEngine
 */
QAmbientSound::QAmbientSound(QAudioEngine *engine) : QObject(*new QAmbientSoundPrivate(engine))
{
    Q_D(QAmbientSound);

    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep) {
        ep->addStereoSound(this);
        d->applyVolume();
    }
}

QAmbientSound::~QAmbientSound()
{
    Q_D(QAmbientSound);

    auto *ep = QAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->removeStereoSound(this);
}

/*!
    \property QAmbientSound::volume

    Defines the volume of the sound.

    Values between 0 and 1 will attenuate the sound, while values above 1
    provide an additional gain boost.
 */
void QAmbientSound::setVolume(float volume)
{
    Q_D(QAmbientSound);
    if (volume != d->volume()) {
        d->setVolume(volume);
        emit volumeChanged();
    }
}

float QAmbientSound::volume() const
{
    Q_D(const QAmbientSound);
    return d->volume();
}

void QAmbientSound::setSource(const QUrl &url)
{
    Q_D(QAmbientSound);
    if (d->url() == url)
        return;
    d->loadUrl(url);

    emit sourceChanged();
}

/*!
    \property QAmbientSound::source

    The source file for the sound to be played.
 */
QUrl QAmbientSound::source() const
{
    Q_D(const QAmbientSound);
    return d->url();
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
    Q_D(const QAmbientSound);
    return d->m_loops.load(std::memory_order_relaxed);
}

void QAmbientSound::setLoops(int loops)
{
    Q_D(QAmbientSound);
    int oldLoops = d->m_loops.exchange(loops, std::memory_order_relaxed);
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
    Q_D(const QAmbientSound);
    return d->m_autoPlay.load(std::memory_order_relaxed);
}

void QAmbientSound::setAutoPlay(bool autoPlay)
{
    Q_D(QAmbientSound);

    bool old = d->m_autoPlay.exchange(autoPlay, std::memory_order_relaxed);
    if (old != autoPlay)
        emit autoPlayChanged();
}

/*!
    Starts playing back the sound. Does nothing if the sound is already playing.
 */
void QAmbientSound::play()
{
    Q_D(QAmbientSound);
    d->play();
}

/*!
    Pauses sound playback. Calling play() will continue playback.
 */
void QAmbientSound::pause()
{
    Q_D(QAmbientSound);
    d->pause();
}

/*!
    Stops sound playback and resets the current position and current loop count to 0.
    Calling play() will start playback at the beginning of the sound file.
 */
void QAmbientSound::stop()
{
    Q_D(QAmbientSound);
    d->stop();
}

/*!
    Returns the engine associated with this sound.
 */
QAudioEngine *QAmbientSound::engine() const
{
    Q_D(const QAmbientSound);

    return d->engine;
}

QT_END_NAMESPACE

#include "moc_qambientsound.cpp"
