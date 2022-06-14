/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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
#include "qquick3dambientsound_p.h"
#include "qquick3daudioengine_p.h"
#include "qambientsound.h"
#include <QAudioFormat>
#include <qdir.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype AmbientSound
    \inqmlmodule QtQuick3D.SpatialAudio
    \ingroup quick3d_spatialaudio

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
    \qmlproperty url QAmbientSound::source

    The source file for the sound to be played.
 */
QUrl QQuick3DAmbientSound::source() const
{
    return m_sound->source();
}

void QQuick3DAmbientSound::setSource(QUrl source)
{
    QUrl url = QUrl::fromLocalFile(QDir::currentPath() + u"/");
    url = url.resolved(source);

    m_sound->setSource(url);
}

/*!
    \qmlproperty float QAmbientSound::volume

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
   \qmlproperty int QAmbientSound::loops

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
