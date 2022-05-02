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
#include "qquick3dspatialaudiostereosource_p.h"
#include "qquick3dspatialaudioengine_p.h"
#include "qspatialaudiostereosource.h"
#include <QAudioFormat>
#include <qdir.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SpatialAudioStereoSource
    \inqmlmodule QtQuick3D.SpatialAudio

    \brief A stereo overlay sound.

    A SpatialAudioStereoSource represents a position and orientation independent sound.
    It's commonly used for background sounds (e.g. music) that is supposed to be independent
    of the listeners position and orientation.
  */

QQuick3DSpatialAudioStereoSource::QQuick3DSpatialAudioStereoSource()
{
    m_sound = new QSpatialAudioStereoSource(QQuick3DSpatialAudioEngine::getEngine());

    connect(m_sound, &QSpatialAudioStereoSource::sourceChanged, this, &QQuick3DSpatialAudioStereoSource::sourceChanged);
    connect(m_sound, &QSpatialAudioStereoSource::volumeChanged, this, &QQuick3DSpatialAudioStereoSource::volumeChanged);
    connect(m_sound, &QSpatialAudioStereoSource::loopsChanged, this, &QQuick3DSpatialAudioStereoSource::loopsChanged);
    connect(m_sound, &QSpatialAudioStereoSource::autoPlayChanged, this, &QQuick3DSpatialAudioStereoSource::autoPlayChanged);
}

QQuick3DSpatialAudioStereoSource::~QQuick3DSpatialAudioStereoSource()
{
    delete m_sound;
}

/*!
    \qmlproperty url QSpatialAudioStereoSource::source

    The source file for the sound to be played.
 */
QUrl QQuick3DSpatialAudioStereoSource::source() const
{
    return m_sound->source();
}

void QQuick3DSpatialAudioStereoSource::setSource(QUrl source)
{
    QUrl url = QUrl::fromLocalFile(QDir::currentPath() + u"/");
    url = url.resolved(source);

    m_sound->setSource(url);
}

/*!
    \qmlproperty float QSpatialAudioStereoSource::volume

    Defines an overall volume for this sound source.
 */
void QQuick3DSpatialAudioStereoSource::setVolume(float volume)
{
    m_sound->setVolume(volume);
}

float QQuick3DSpatialAudioStereoSource::volume() const
{
    return m_sound->volume();
}

/*!
   \qmlproperty int QSpatialAudioStereoSource::loops

    Determines how often the sound is played before the player stops.
    Set to SpatialAudioSoundSource::Infinite to loop the current sound forever.

    The default value is \c 1.
 */
int QQuick3DSpatialAudioStereoSource::loops() const
{
    return m_sound->loops();
}

void QQuick3DSpatialAudioStereoSource::setLoops(int loops)
{
    m_sound->setLoops(loops);
}

/*!
   \qmlproperty bool SpatialAudioStereoSource::autoPlay

    Determines whether the sound should automatically start playing when a source
    gets specified.

    The default value is \c true.
 */
bool QQuick3DSpatialAudioStereoSource::autoPlay() const
{
    return m_sound->autoPlay();
}

void QQuick3DSpatialAudioStereoSource::setAutoPlay(bool autoPlay)
{
    m_sound->setAutoPlay(autoPlay);
}

/*!
    \qmlmethod SpatialAudioStereoSource::play()

    Starts playing back the sound. Does nothing if the sound is already playing.
 */
void QQuick3DSpatialAudioStereoSource::play()
{
    m_sound->play();
}

/*!
    \qmlmethod SpatialAudioStereoSource::pause()

    Pauses sound playback at the current position. Calling play() will continue playback.
 */
void QQuick3DSpatialAudioStereoSource::pause()
{
    m_sound->pause();
}

/*!
    \qmlmethod SpatialAudioStereoSource::stop()

    Stops sound playback and resets the current position and loop count to 0. Calling play() will
    begin playback at the beginning of the sound file.
 */
void QQuick3DSpatialAudioStereoSource::stop()
{
    m_sound->stop();
}

QT_END_NAMESPACE
