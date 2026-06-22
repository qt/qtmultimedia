// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#include "qambientsound.h"

#include <QtSpatialAudio/private/qaudioengine_p.h>
#include <QtSpatialAudio/private/qspatialaudiosound_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>

#include <resonance_audio.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace {

int addStereoSource(QAudioEngine *engine)
{
    auto *ep = QAudioEnginePrivate::get(engine);
    if (!ep)
        return -1;
    return ep->resonanceAudio->api->CreateStereoSource(2);
}

} // namespace

class QAmbientSoundPrivate final : public QSpatialAudioSoundPrivate
{
    Q_DECLARE_PUBLIC(QAmbientSound)

public:
    explicit QAmbientSoundPrivate(QAudioEngine *engine);
};

QAmbientSoundPrivate::QAmbientSoundPrivate(QAudioEngine *engine)
    : QSpatialAudioSoundPrivate{
          engine,
          2,
          addStereoSource(engine),
      }
{
    QSpatialAudioSoundPrivate::applyVolume();
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
    if (d->state() != QSpatialAudioSoundPrivate::State::Stopped)
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
