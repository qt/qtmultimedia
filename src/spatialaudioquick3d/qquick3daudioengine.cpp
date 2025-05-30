// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#include <qquick3daudioengine_p.h>
#include <qaudiodevice.h>

QT_BEGIN_NAMESPACE

static QAudioEngine *globalEngine = nullptr;

/*!
    \qmltype AudioEngine
    \inqmlmodule QtQuick3D.SpatialAudio
    \ingroup quick3d_spatialaudio
    \ingroup multimedia_audio_qml

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

    As the rest of Qt Quick 3D, the audio engine uses a coordinate system that is in centimeters by default.
    The axes are defined so that positive x points to the right, positive y points up and positive z points
    backwards.
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
    \row \li Surround
        \li Map the sounds to the loudspeaker configuration of the output device.
            This is normally a stereo or surround speaker setup.
    \row \li Stereo
        \li Map the sounds to the stereo loudspeaker configuration of the output device.
            This will ignore any additional speakers and only use the left and right channels
            to create a stero rendering of the sound field.
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
