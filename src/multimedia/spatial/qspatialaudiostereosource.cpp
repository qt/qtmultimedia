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
#include "qspatialaudiostereosource.h"
#include "qspatialaudiolistener.h"
#include "qspatialaudioengine_p.h"
#include "api/resonance_audio_api.h"
#include <qaudiosink.h>
#include <qurl.h>
#include <qdebug.h>
#include <qaudiodecoder.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSpatialAudioStereoSource
    \inmodule QtMultimedia
    \ingroup multimedia_spatialaudio

    \brief A stereo overlay sound.

    QSpatialAudioStereoSource represents a position and orientation independent sound.
    It's commonly used for background sounds (e.g. music) that is supposed to be independent
    of the listeners position and orientation.
  */

/*!
    Creates a stereo sound source for \a engine.
 */
QSpatialAudioStereoSource::QSpatialAudioStereoSource(QSpatialAudioEngine *engine)
    : d(new QSpatialAudioSound(this))
{
    setEngine(engine);
}

QSpatialAudioStereoSource::~QSpatialAudioStereoSource()
{
    setEngine(nullptr);
    delete d;
}

/*!
    \property QSpatialAudioStereoSource::volume

    Defines the volume of the sound.

    Values between 0 and 1 will attenuate the sound, while values above 1
    provide an additional gain boost.
 */
void QSpatialAudioStereoSource::setVolume(float volume)
{
    if (d->volume == volume)
        return;
    d->volume = volume;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep)
        ep->api->SetSourceVolume(d->sourceId, d->volume);
    emit volumeChanged();
}

float QSpatialAudioStereoSource::volume() const
{
    return d->volume;
}

void QSpatialAudioStereoSource::setSource(const QUrl &url)
{
    if (d->url == url)
        return;
    d->url = url;

    d->load();
    emit sourceChanged();
}

/*!
    \property QSpatialAudioStereoSource::source

    The source file for the sound to be played.
 */
QUrl QSpatialAudioStereoSource::source() const
{
    return d->url;
}

/*!
   \property QSpatialAudioStereoSource::loops

    Determines how many times the sound is played before the player stops.
    Set to QSpatialAudioSoundSource::Infinite to play the current sound in
    a loop forever.

    The default value is \c 1.
 */
int QSpatialAudioStereoSource::loops() const
{
    return d->m_loops.loadRelaxed();
}

void QSpatialAudioStereoSource::setLoops(int loops)
{
    int oldLoops = d->m_loops.fetchAndStoreRelaxed(loops);
    if (oldLoops != loops)
        emit loopsChanged();
}

/*!
   \property QSpatialAudioStereoSource::autoPlay

    Determines whether the sound should automatically start playing when a source
    gets specified.

    The default value is \c true.
 */
bool QSpatialAudioStereoSource::autoPlay() const
{
    return d->m_autoPlay.loadRelaxed();
}

void QSpatialAudioStereoSource::setAutoPlay(bool autoPlay)
{
    bool old = d->m_autoPlay.fetchAndStoreRelaxed(autoPlay);
    if (old != autoPlay)
        emit autoPlayChanged();
}

/*!
    Starts playing back the sound. Does nothing if the sound is already playing.
 */
void QSpatialAudioStereoSource::play()
{
    d->play();
}

/*!
    Pauses sound playback. Calling play() will continue playback.
 */
void QSpatialAudioStereoSource::pause()
{
    d->pause();
}

/*!
    Stops sound playback and resets the current position and current loop count to 0.
    Calling play() will start playback at the beginning of the sound file.
 */
void QSpatialAudioStereoSource::stop()
{
    d->stop();
}

/*!
    \internal
 */
void QSpatialAudioStereoSource::setEngine(QSpatialAudioEngine *engine)
{
    if (d->engine == engine)
        return;
    auto *ep = QSpatialAudioEnginePrivate::get(engine);

    if (ep)
        ep->removeStereoSound(this);
    d->engine = engine;

    ep = QSpatialAudioEnginePrivate::get(engine);
    if (ep) {
        ep->addStereoSound(this);
        ep->api->SetSourceVolume(d->sourceId, d->volume);
    }
}

/*!
    Returns the engine associated with this listener.
 */
QSpatialAudioEngine *QSpatialAudioStereoSource::engine() const
{
    return d->engine;
}

QT_END_NAMESPACE

#include "moc_qspatialaudiostereosource.cpp"
