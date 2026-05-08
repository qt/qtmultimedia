// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#include "qambientsound.h"
#include "qambientsound_p.h"

#include <QtMultimedia/qaudiodecoder.h>
#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/private/qaudio_qspan_support_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtSpatialAudio/private/qaudioengine_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qurl.h>

#include <resonance_audio.h>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QSpatialAudioPrivate {

QSpatialAudioPlaybackState::QSpatialAudioPlaybackState(QAudioBuffer buffer, bool playing, int loops)
    : m_playing(playing), m_loops(loops), m_buffer(std::move(buffer))
{
}

void QSpatialAudioPlaybackState::getBuffer(QSpan<float> output)
{
    using QtMultimediaPrivate::drop;
    using QtMultimediaPrivate::take;
    namespace ranges = QtMultimediaPrivate::ranges;

    if (!m_playing.load(std::memory_order_relaxed)) {
        ranges::fill(output, 0.f);
        return;
    }

    QSpan<const float> wholeSampleBuffer{
        m_buffer.constData<float>(),
        m_buffer.sampleCount(),
    };

    while (!output.empty()) {
        QSpan remainingSamples = drop(wholeSampleBuffer, m_currentSample);
        const QSpan samplesToCopy = take(remainingSamples, output.size());
        ranges::copy(samplesToCopy, output.begin());
        output = drop(output, samplesToCopy.size());
        m_currentSample += samplesToCopy.size();

        if (output.empty())
            return;

        // Reached end of buffer
        Q_ASSERT(m_currentSample == wholeSampleBuffer.size());
        m_currentSample = 0;

        switch (m_loops) {
        case QAmbientSound::Infinite:
            break; // Continue looping
        case 0:
            m_playing = false;
            m_currentLoop = 0;
            ranges::fill(output, 0.f);
            return;
        default:
            ++m_currentLoop;
            if (m_currentLoop >= m_loops) {
                m_playing = false;
                m_currentLoop = 0;
                ranges::fill(output, 0.f);
                return;
            }
            break;
        }
    }
}

void QSpatialAudioPlaybackState::resume()
{
    m_playing = true;
}

void QSpatialAudioPlaybackState::setLoops(int loops)
{
    m_loops = loops;
}

QAudioFormat QSpatialAudioPlaybackState::format() const
{
    return m_buffer.format();
}

void QSpatialAudioPlaybackState::pause()
{
    m_playing = false;
}

} // namespace QSpatialAudioPrivate

namespace {

std::optional<QAudioBuffer> joinBuffers(QSpan<const QAudioBuffer> buffers)
{
    if (buffers.empty())
        return {};

    QByteArray data;
    for (const QAudioBuffer &b : buffers) {
        if (!b.isValid())
            return {};

        if (b.format() != buffers.front().format()) {
            qWarning() << "QAmbientSound: all buffers must have the same format";
            return {};
        }

        data.append(b.constData<char>(), b.byteCount());
    }

    return QAudioBuffer{
        data,
        buffers.front().format(),
        buffers.front().startTime(),
    };
}

int addStereoSource(QAudioEngine *engine)
{
    auto *ep = QAudioEnginePrivate::get(engine);
    if (!ep)
        return -1;
    return ep->resonanceAudio->api->CreateStereoSource(2);
}

} // namespace

QAmbientSoundPrivate::QAmbientSoundPrivate(QAudioEngine *engine)
    : QAmbientSoundPrivate{
          engine,
          2,
          addStereoSource(engine),
      }
{
    QAmbientSoundPrivate::applyVolume();
}

QAmbientSoundPrivate::QAmbientSoundPrivate(QAudioEngine *engine, int nchannels, int sourceId)
    : nchannels(nchannels), engine(engine), sourceId(sourceId)
{
    auto *ep = QAudioEnginePrivate::get(engine);
    if (!ep)
        return;
    ep->addSound(this);
}

QAmbientSoundPrivate::~QAmbientSoundPrivate()
{
    auto *ep = QAudioEnginePrivate::get(engine);
    if (ep)
        ep->removeSound(this);

    withResonanceApi([&](vraudio::ResonanceAudioApi *api) {
        api->DestroySource(sourceId);
    });
}

void QAmbientSoundPrivate::setVolume(float volume)
{
    m_volume = volume;
    applyVolume();
}

void QAmbientSoundPrivate::setLoops(int loops)
{
    m_loops = loops;
    if (m_playbackState)
        m_playbackState->setLoops(loops);
}

void QAmbientSoundPrivate::setAutoPlay(bool enabled)
{
    m_autoPlay = enabled;
}

void QAmbientSoundPrivate::applyVolume()
{
    withResonanceApi([&](vraudio::ResonanceAudioApi *api) {
        api->SetSourceVolume(sourceId, m_volume);
    });
}

void QAmbientSoundPrivate::play()
{
    switch (m_state) {
    case State::Stopped: {
        m_state = State::Playing;
        if (m_buffer) {
            setState(std::make_shared<QSpatialAudioPlaybackState>(*m_buffer, /*playing=*/true,
                                                                  m_loops));
        }
        return;
    }
    case State::Paused:
    case State::Playing: {
        m_state = State::Playing;
        if (m_playbackState)
            m_playbackState->resume();
        return;
    }
    }
}

void QAmbientSoundPrivate::pause()
{
    switch (m_state) {
    case State::Stopped: {
        m_state = State::Paused;
        setState(std::make_shared<QSpatialAudioPlaybackState>(*m_buffer, /*playing=*/false,
                                                              m_loops));
        return;
    }
    case State::Paused: {
        return;
    }
    case State::Playing: {
        m_state = State::Paused;
        setState(nullptr);

        if (m_playbackState)
            m_playbackState->pause();
        return;
    }
    }
}

void QAmbientSoundPrivate::stop()
{
    switch (m_state) {
    case State::Stopped: {
        return;
    }
    case State::Paused:
    case State::Playing: {
        m_state = State::Stopped;
        setState({});
        return;
    }
    }
}

void QAmbientSoundPrivate::loadUrl(const QUrl &url)
{
    m_url = url;

    auto *ep = QAudioEnginePrivate::get(engine);
    if (!ep)
        return;

    Q_Q(QAmbientSound);

    m_loadFuture.cancel();

    setState(nullptr);

    QAudioFormat f;
    f.setSampleFormat(QAudioFormat::Float);
    f.setSampleRate(ep->sampleRate());
    f.setChannelConfig(nchannels == 2 ? QAudioFormat::ChannelConfigStereo
                                      : QAudioFormat::ChannelConfigMono);

    QUrl resolved = m_sourceResolver->resolve(url);
    m_loadFuture = load(resolved, f).then(q, [this](QAmbientSoundPrivate::LoadResult result) {
        if (result) {
            m_buffer = joinBuffers(*result);
            if (!m_buffer) {
                qWarning() << "QAmbientSound: failed to join audio buffers";
                return;
            }

            bool startingPlayback = m_autoPlay || m_state != State::Stopped;
            if (!startingPlayback)
                return;
            bool startPaused = (m_state == State::Paused) && !m_autoPlay;
            setState(std::make_shared<QSpatialAudioPlaybackState>(
                    *m_buffer, /*playing=*/!startPaused, m_loops));

            m_state = startPaused ? State::Paused : State::Playing;

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

vraudio::ResonanceAudioApi *QAmbientSoundPrivate::getAPI()
{
    auto *ep = QAudioEnginePrivate::get(engine);
    if (!ep)
        return nullptr;
    return ep->resonanceAudio->api.get();
}

void QAmbientSoundPrivate::setState(SharedPlaybackState state)
{
    auto *ep = QAudioEnginePrivate::get(engine);
    if (ep)
        ep->setSoundPlaybackData(this, state);
    m_playbackState = std::move(state);
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

    \note QAmbientSound is only active when the QAudioEngine is using OutputMode::Stereo or
          OutputMode::Headphone.
  */

/*!
    Creates a stereo sound source for \a engine.

    \note Must be called with a valid QAudioEngine
 */
QAmbientSound::QAmbientSound(QAudioEngine *engine) : QObject(*new QAmbientSoundPrivate(engine))
{
    if (!engine)
        qWarning() << "Cannot create QAmbientSound without a valid QAudioEngine";
}

QAmbientSound::~QAmbientSound()
{
    Q_D(QAmbientSound);
    if (d->state() != QAmbientSoundPrivate::State::Stopped)
        d->stop();
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
    return d->loops();
}

void QAmbientSound::setLoops(int loops)
{
    Q_D(QAmbientSound);
    if (loops != d->loops()) {
        d->setLoops(loops);
        emit loopsChanged();
    }
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
    return d->autoPlay();
}

void QAmbientSound::setAutoPlay(bool autoPlay)
{
    Q_D(QAmbientSound);
    if (autoPlay != d->autoPlay()) {
        d->setAutoPlay(autoPlay);
        emit autoPlayChanged();
    }
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
