/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Quick3D Audio module of the Qt Toolkit.
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
#include <qquick3daudioengine_p.h>
#include <qaudiodevice.h>

QT_BEGIN_NAMESPACE

static QAudioEngine *globalEngine = nullptr;

/*!
    \qmltype AudioEngine
    \inqmlmodule QtQuick3D.Audio
    \ingroup quick3d_audio

    \brief AudioEngine manages sound objects inside a 3D scene.

    AudioEngine manages sound objects inside a 3D scene. You can add
    SpatialSound objects to the scene to define sounds that happen
    at a specified location in 3D space. AmbientSound allows you to add
    a stereo overlay (for example voice over or a sound track).

    You can use AudioListener to define the position of the person listening
    to the sound field relative to the sound sources. Sound sources will be less audible
    if the listener is further away from source. They will also get mapped to the corresponding
    loudspeakers depending on the direction between listener and source. In many cases, the
    AudioListener object can simply be instantiated as a child object of the QtQuick3D.Camera
    object.

    Create AudioRoom objcects to simulate the sound (reflections and reverb) of a room with
    certain dimensions and different types of walls.

    AudioEngine does offer a mode where Qt is using simulating the effects of the ear
    using head related impulse reponse functions (see also https://en.wikipedia.org/wiki/Sound_localization)
    to localize the sound in 3D space when using headphones and create a spatial audio effect through
    headphones.
*/


QQuick3DAudioEngine::QQuick3DAudioEngine()
{
    auto *e = getEngine();
    connect(e, &QAudioEngine::outputModeChanged, this, &QQuick3DAudioEngine::outputModeChanged);
    connect(e, &QAudioEngine::outputDeviceChanged, this, &QQuick3DAudioEngine::outputDeviceChanged);
    connect(e, &QAudioEngine::masterVolumeChanged, this, &QQuick3DAudioEngine::masterVolumeChanged);
}

QQuick3DAudioEngine::~QQuick3DAudioEngine()
{
}

/*!
    \qmlproperty enumeration AudioEngine::outputMode

    Sets or retrieves the current output mode of the engine.

    \table
    \header \li Property value
            \li Description
    \row \li Normal
        \li Map the sounds to the loudspeaker configuration of the output device.
            This is normally a stereo or surround speaker setup.
    \row \li Headphone
        \li Use Headphone spatialization to create a 3D audio effect when listening
            to the sound field through headphones.
    \endtable
 */

void QQuick3DAudioEngine::setOutputMode(OutputMode mode)
{
    globalEngine->setOutputMode(QAudioEngine::OutputMode(mode));
}

QQuick3DAudioEngine::OutputMode QQuick3DAudioEngine::outputMode() const
{
    return OutputMode(globalEngine->outputMode());
}

/*!
    \qmlproperty QtMultimedia.AudioDevice AudioEngine::outputDevice

    Sets or returns the device that is being used for outputting the sound field.
 */
void QQuick3DAudioEngine::setOutputDevice(const QAudioDevice &device)
{
    globalEngine->setOutputDevice(device);
}

QAudioDevice QQuick3DAudioEngine::outputDevice() const
{
    return globalEngine->outputDevice();
}

/*!
    \qmlproperty float AudioEngine::masterVolume

    Sets or returns overall volume being used to render the sound field.
 */
void QQuick3DAudioEngine::setMasterVolume(float volume)
{
    globalEngine->setMasterVolume(volume);
}

float QQuick3DAudioEngine::masterVolume() const
{
    return globalEngine->masterVolume();
}

QAudioEngine *QQuick3DAudioEngine::getEngine()
{
    if (!globalEngine) {
        globalEngine = new QAudioEngine;
        globalEngine->start();
    }
    return globalEngine;
}

QT_END_NAMESPACE

#include "moc_qquick3daudioengine_p.cpp"
