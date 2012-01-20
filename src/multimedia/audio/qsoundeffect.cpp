/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsoundeffect.h"

#if defined(QT_MULTIMEDIA_PULSEAUDIO)
#include "qsoundeffect_pulse_p.h"
#elif(QT_MULTIMEDIA_QMEDIAPLAYER)
#include "qsoundeffect_qmedia_p.h"
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmlclass SoundEffect QSoundEffect
    \brief The SoundEffect element provides a way to play sound effects in QML.

    \inmodule QtMultimedia
    \ingroup multimedia_qml

    This element is part of the \bold{QtMultimedia 5.0} module.

    The following example plays a WAV file on mouse click.

    \snippet doc/src/snippets/multimedia-snippets/soundeffect.qml complete snippet
*/

/*!
    \class QSoundEffect
    \brief The QSoundEffect class provides a way to play low latency sound effects.

    \ingroup multimedia
    \ingroup multimedia_audio


*/

/*!
    \qmlproperty url SoundEffect::source
    \property QSoundEffect::source

    This property provides a way to control the sound to play.
*/

/*!
    \qmlproperty int SoundEffect::loops

    This property provides a way to control the number of times to repeat the sound on each play().

    Set to SoundEffect.Infinite to enable infinite looping.
*/

/*!
    \property QSoundEffect::loops
    This property provides a way to control the number of times to repeat the sound on each play().

    Set to QSoundEffect::Infinite to enable infinite looping.
*/

/*!
    \qmlproperty qreal SoundEffect::volume
    \property QSoundEffect::volume

    This property holds the volume of the playback, from 0.0 (silent) to 1.0 (maximum volume).
    Note: Currently this has no effect on Mac OS X.
*/

/*!
    \qmlproperty bool SoundEffect::muted
    \property QSoundEffect::muted

    This property provides a way to control muting. A value of \c true will mute this effect.
*/

/*!
    \qmlproperty bool SoundEffect::playing
    \property QSoundEffect::source

    This property indicates if the soundeffect is playing or not.
*/

/*!
    \qmlproperty int SoundEffect::status

    This property indicates the following status of the soundeffect.

    Null: no source has been set or is null.
    Loading: the soundeffect is trying to load the source.
    Ready: the source is loaded and ready for play.
    Error: some error happened during operation, such as failure of loading the source.
*/

/*!
    \qmlsignal SoundEffect::sourceChanged()
    \fn void QSoundEffect::sourceChanged()

    This handler is called when the source has changed.
*/
/*!
    \qmlsignal SoundEffect::loadedChanged()
    \fn void QSoundEffect::loadedChanged()

    This handler is called when the loading state has changed.
*/

/*!
    \qmlsignal SoundEffect::loopCountChanged()
    \fn void QSoundEffect::loopCountChanged()

    This handler is called when the initial number of loops has changed.
*/

/*!
    \qmlsignal SoundEffect::loopsRemainingChanged()
    \fn void QSoundEffect::loopsRemainingChanged()

    This handler is called when the remaining number of loops has changed.
*/

/*!
    \qmlsignal SoundEffect::volumeChanged()
    \fn void QSoundEffect::volumeChanged()

    This handler is called when the volume has changed.
*/

/*!
    \qmlsignal SoundEffect::mutedChanged()
    \fn void QSoundEffect::mutedChanged()

    This handler is called when the mute state has changed.
*/

/*!
    \qmlsignal SoundEffect::playingChanged()
    \fn void QSoundEffect::playingChanged()

    This handler is called when the playing property has changed.
*/

/*!
    \qmlsignal SoundEffect::statusChanged()
    \fn void QSoundEffect::statusChanged()

    This handler is called when the status property has changed.
*/


/*!
    Creates a QSoundEffect with the given \a parent.
*/
QSoundEffect::QSoundEffect(QObject *parent) :
    QObject(parent)
{
    d = new QSoundEffectPrivate(this);
    connect(d, SIGNAL(loopsRemainingChanged()), SIGNAL(loopsRemainingChanged()));
    connect(d, SIGNAL(volumeChanged()), SIGNAL(volumeChanged()));
    connect(d, SIGNAL(mutedChanged()), SIGNAL(mutedChanged()));
    connect(d, SIGNAL(loadedChanged()), SIGNAL(loadedChanged()));
    connect(d, SIGNAL(playingChanged()), SIGNAL(playingChanged()));
    connect(d, SIGNAL(statusChanged()), SIGNAL(statusChanged()));
}

/*!
    Destroys this sound effect.
 */
QSoundEffect::~QSoundEffect()
{
    d->release();
}

QStringList QSoundEffect::supportedMimeTypes()
{
    return QSoundEffectPrivate::supportedMimeTypes();
}

QUrl QSoundEffect::source() const
{
    return d->source();
}

void QSoundEffect::setSource(const QUrl &url)
{
    if (d->source() == url)
        return;

    d->setSource(url);

    emit sourceChanged();
}

int QSoundEffect::loopCount() const
{
    return d->loopCount();
}

int QSoundEffect::loopsRemaining() const
{
    return d->loopsRemaining();
}

void QSoundEffect::setLoopCount(int loopCount)
{
    if (loopCount < 0 && loopCount != Infinite) {
        qWarning("SoundEffect: loops should be SoundEffect.Infinite, 0 or positive integer");
        return;
    }
    if (loopCount == 0)
        loopCount = 1;
    if (d->loopCount() == loopCount)
        return;

    d->setLoopCount(loopCount);
    emit loopCountChanged();
}

qreal QSoundEffect::volume() const
{
    return qreal(d->volume()) / 100;
}

void QSoundEffect::setVolume(qreal volume)
{
    if (volume < 0 || volume > 1) {
        qWarning("SoundEffect: volume should be between 0.0 and 1.0");
        return;
    }
    int iVolume = qRound(volume * 100);
    if (d->volume() == iVolume)
        return;

    d->setVolume(iVolume);
}

bool QSoundEffect::isMuted() const
{
    return d->isMuted();
}

void QSoundEffect::setMuted(bool muted)
{
    if (d->isMuted() == muted)
        return;

    d->setMuted(muted);
}

bool QSoundEffect::isLoaded() const
{
    return d->isLoaded();
}

/*!
  \qmlmethod SoundEffect::play()

  Start playback of the sound effect, looping the effect for the number of
  times as specified in the loops property.

  This is the default method for SoundEffect.

  \snippet doc/src/snippets/multimedia-snippets/soundeffect.qml play sound on click
*/
void QSoundEffect::play()
{
    d->play();
}

bool QSoundEffect::isPlaying() const
{
    return d->isPlaying();
}

/*!
    \enum QSoundEffect::Status
    \value Null         This sound effect does not have a source set.
    \value Loading      The source is being loaded
    \value Ready        The source has been loaded, and can be played.
    \value Error        An error occurred while loading the source or during playback.
*/

/*!
    Returns the current status of this sound effect.
 */
QSoundEffect::Status QSoundEffect::status() const
{
    return d->status();
}


/*!
  \qmlmethod SoundEffect::stop()

  Stop current playback.
  Note that if the backend is PulseAudio, due to the limitation of the underlying API,
  tis stop will only prevent next looping but will not be able to stop current playback immediately.

 */
void QSoundEffect::stop()
{
    d->stop();
}

QT_END_NAMESPACE

#include "moc_qsoundeffect.cpp"
