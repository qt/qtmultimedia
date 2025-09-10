// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#include "qquick3dambientsound_p.h"
#include "qquick3daudioengine_p.h"
#include "qambientsound.h"
#include <QAudioFormat>
#include <qdir.h>
#include <QQmlContext>
#include <QQmlFile>

QT_BEGIN_NAMESPACE

/*!
    \qmltype AmbientSound
    \inqmlmodule QtQuick3D.SpatialAudio
    \ingroup quick3d_spatialaudio
    \ingroup multimedia_audio_qml

    \brief A stereo overlay sound.

    A AmbientSound represents a position and orientation independent sound.
    It's commonly used for background sounds (e.g. music) that is supposed to be independent
    of the listeners position and orientation.
  */

QQuick3DAmbientSound::QQuick3DAmbientSound()
{
    m_sound = new QAmbientSound(QQuick3DAudioEngine::getEngine());

    connect(m_sound, &QAmbientSound::sourceChanged, this, &QQuick3DAmbientSound::sourceChanged);
    connect(m_sound, &QAmbientSound::volumeChanged, this, &QQuick3DAmbientSound::volumeChanged);
    connect(m_sound, &QAmbientSound::loopsChanged, this, &QQuick3DAmbientSound::loopsChanged);
    connect(m_sound, &QAmbientSound::autoPlayChanged, this, &QQuick3DAmbientSound::autoPlayChanged);
}

QQuick3DAmbientSound::~QQuick3DAmbientSound()
{
    delete m_sound;
}

/*!
    \qmlproperty url AmbientSound::source

    The source file for the sound to be played.
 */
QUrl QQuick3DAmbientSound::source() const
{
    return m_sound->source();
}

void QQuick3DAmbientSound::setSource(QUrl source)
{
    const QQmlContext *context = qmlContext(this);
    QUrl url;
    if (context) {
        url = context->resolvedUrl(source);
    } else {
        url = QUrl::fromLocalFile(QDir::currentPath() + u"/");
        url = url.resolved(source);
    }
    m_sound->setSource(url);
}

/*!
    \qmlproperty real AmbientSound::volume

    Defines an overall volume for this sound source.
 */
void QQuick3DAmbientSound::setVolume(float volume)
{
    m_sound->setVolume(volume);
}

float QQuick3DAmbientSound::volume() const
{
    return m_sound->volume();
}

/*!
   \qmlproperty int AmbientSound::loops

    Determines how often the sound is played before the player stops.
    Set to QAmbienSound::Infinite to loop the current sound forever.

    The default value is \c 1.
 */
int QQuick3DAmbientSound::loops() const
{
    return m_sound->loops();
}

void QQuick3DAmbientSound::setLoops(int loops)
{
    m_sound->setLoops(loops);
}

/*!
   \qmlproperty bool AmbientSound::autoPlay

    Determines whether the sound should automatically start playing when a source
    gets specified.

    The default value is \c true.
 */
bool QQuick3DAmbientSound::autoPlay() const
{
    return m_sound->autoPlay();
}

void QQuick3DAmbientSound::setAutoPlay(bool autoPlay)
{
    m_sound->setAutoPlay(autoPlay);
}

/*!
    \qmlmethod AmbientSound::play()

    Starts playing back the sound. Does nothing if the sound is already playing.
 */
void QQuick3DAmbientSound::play()
{
    m_sound->play();
}

/*!
    \qmlmethod AmbientSound::pause()

    Pauses sound playback at the current position. Calling play() will continue playback.
 */
void QQuick3DAmbientSound::pause()
{
    m_sound->pause();
}

/*!
    \qmlmethod AmbientSound::stop()

    Stops sound playback and resets the current position and loop count to 0. Calling play() will
    begin playback at the beginning of the sound file.
 */
void QQuick3DAmbientSound::stop()
{
    m_sound->stop();
}

QT_END_NAMESPACE
