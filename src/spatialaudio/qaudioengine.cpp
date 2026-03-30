// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#include "qaudioengine.h"

#include <QtCore/qspan.h>
#include <QtSpatialAudio/private/qaudioengine_p.h>
#include <QtSpatialAudio/private/qaudioengine_threaded_p.h>
#include <QtSpatialAudio/private/qaudioroom_p.h>

#include <resonance_audio.h>

QT_BEGIN_NAMESPACE

QAudioEnginePrivate::QAudioEnginePrivate(int sampleRate)
    : m_sampleRate(sampleRate),
      resonanceAudio{
          std::make_unique<vraudio::ResonanceAudio>(2, framesPerBuffer, sampleRate),
      }
{
}

QAudioEnginePrivate::~QAudioEnginePrivate() = default;

void QAudioEnginePrivate::setDistanceScale(float scale)
{
    if (scale == m_distanceScale)
        return;
    m_distanceScale = scale;
    Q_Q(QAudioEngine);
    emit q->distanceScaleChanged();
}

float QAudioEnginePrivate::distanceScale() const
{
    return m_distanceScale;
}

void QAudioEnginePrivate::setMasterVolume(float volume)
{
    if (m_masterVolume == volume)
        return;
    m_masterVolume = volume;
    resonanceAudio->api->SetMasterVolume(volume);
    Q_Q(QAudioEngine);
    emit q->masterVolumeChanged();
}

float QAudioEnginePrivate::masterVolume() const
{
    return m_masterVolume;
}

void QAudioEnginePrivate::setListenerPosition(std::optional<QVector3D> pos)
{
    m_position = pos;
}

void QAudioEnginePrivate::setListenerRotation(const QQuaternion &rotation)
{
    resonanceAudio->api->SetHeadRotation(rotation.x(), rotation.y(), rotation.z(),
                                         rotation.scalar());
}

QAudioEnginePrivate::SmallestRoomForListenerResult
QAudioEnginePrivate::findSmallestRoomForListener(QSpan<QAudioRoom *> rooms) const
{
    const std::optional<QVector3D> listenerPos = listenerPosition();

    if (!listenerPos)
        return SmallestRoomForListenerResult{
            nullptr,
            0.f,
        };

    std::optional<float> roomVolume;
    QAudioRoom *room = nullptr;

    for (QAudioRoom *r : std::as_const(rooms)) {
        QVector3D dim2 = r->dimensions() / 2.;
        float vol = dim2.x() * dim2.y() * dim2.z();
        if (roomVolume && vol > roomVolume)
            continue;
        QVector3D dist = r->position() - *listenerPos;
        // transform into room coordinates
        dist = r->rotation().rotatedVector(dist);
        if (qAbs(dist.x()) <= dim2.x() && qAbs(dist.y()) <= dim2.y()
            && qAbs(dist.z()) <= dim2.z()) {
            room = r;
            roomVolume = vol;
        }
    }

    return SmallestRoomForListenerResult{
        room,
        roomVolume.value_or(0.f),
    };
}

/*!
    \class QAudioEngine
    \inmodule QtSpatialAudio
    \ingroup spatialaudio
    \ingroup multimedia_audio

    \brief QAudioEngine manages a three dimensional sound field.

    You can use an instance of QAudioEngine to manage a sound field in
    three dimensions. A sound field is defined by several QSpatialSound
    objects that define a sound at a specified location in 3D space. You can also
    add stereo overlays using QAmbientSound.

    You can use QAudioListener to define the position of the person listening
    to the sound field relative to the sound sources. Sound sources will be less audible
    if the listener is further away from source. They will also get mapped to the corresponding
    loudspeakers depending on the direction between listener and source.

    QAudioEngine offers two output modes. The first mode renders the sound field to a set of
    speakers, either a stereo speaker pair or a surround configuration. The second mode provides
    an immersive 3D sound experience when using headphones.

    Perception of sound localization is driven mainly by two factors. The first factor is timing
    differences of the sound waves between left and right ear. The second factor comes from various
    ways how sounds coming from different direcations create different types of reflections from our
    ears and heads. See https://en.wikipedia.org/wiki/Sound_localization for more details.

    The spatial audio engine emulates those timing differences and reflections through
    Head related transfer functions (HRTF, see
    https://en.wikipedia.org/wiki/Head-related_transfer_function). The functions used emulates those
    effects for an average persons ears and head. It provides a good and immersive 3D sound localization
    experience for most persons when using headphones.

    The engine is rather versatile allowing you to define room properties and reverb settings to emulate
    different types of rooms.

    Sound sources can also be occluded dampening the sound coming from those sources.

    The audio engine uses a coordinate system that is in centimeters by default. The axes are aligned with the
    typical coordinate system used in 3D. Positive x points to the right, positive y points up and positive z points
    backwards.

*/

/*!
    \fn QAudioEngine::QAudioEngine()
    \fn QAudioEngine::QAudioEngine(QObject *parent)
    \fn QAudioEngine::QAudioEngine(int sampleRate, QObject *parent = nullptr)

    Constructs a spatial audio engine with \a parent, if any.

    The engine will operate with a sample rate given by \a sampleRate. The
    default sample rate, if none is provided, is 44100 (44.1kHz).

    Sound content that is not provided at that sample rate will automatically
    get resampled to \a sampleRate when being processed by the engine. The
    default sample rate is fine in most cases, but you can define a different
    rate if most of your sound files are sampled with a different rate, and
    avoid some CPU overhead for resampling.
 */
QAudioEngine::QAudioEngine(int sampleRate, QObject *parent)
    : QObject(*new QAudioEngineThreaded(sampleRate), parent)
{
}

/*!
    Destroys the spatial audio engine.
 */
QAudioEngine::~QAudioEngine()
{
    stop();
}

/*! \enum QAudioEngine::OutputMode
    \value Surround Map the sounds to the loudspeaker configuration of the output device.
        This is normally a stereo or surround speaker setup.
    \value Stereo Map the sounds to the stereo loudspeaker configuration of the output device.
        This will ignore any additional speakers and only use the left and right channels
        to create a stero rendering of the sound field.
    \value Headphone Use Headphone spatialization to create a 3D audio effect when listening
        to the sound field through headphones
*/

/*!
    \property QAudioEngine::outputMode

    Sets or retrieves the current output mode of the engine.

    \sa QAudioEngine::OutputMode
 */
void QAudioEngine::setOutputMode(OutputMode mode)
{
    Q_D(QAudioEngine);
    d->setOutputMode(mode);
}

QAudioEngine::OutputMode QAudioEngine::outputMode() const
{
    Q_D(const QAudioEngine);
    return d->outputMode();
}

/*!
    Returns the sample rate the engine has been configured with.
 */
int QAudioEngine::sampleRate() const
{
    Q_D(const QAudioEngine);
    return d->sampleRate();
}

/*!
    \property QAudioEngine::outputDevice

    Sets or returns the device that is being used for playing the sound field.
 */
void QAudioEngine::setOutputDevice(const QAudioDevice &device)
{
    Q_D(QAudioEngine);
    d->setOutputDevice(device);
}

QAudioDevice QAudioEngine::outputDevice() const
{
    Q_D(const QAudioEngine);
    return d->outputDevice();
}

/*!
    \property QAudioEngine::masterVolume

    Sets or returns volume being used to render the sound field.
 */
void QAudioEngine::setMasterVolume(float volume)
{
    Q_D(QAudioEngine);
    return d->setMasterVolume(volume);
}

float QAudioEngine::masterVolume() const
{
    Q_D(const QAudioEngine);
    return d->masterVolume();
}

/*!
    Starts the engine.
 */
void QAudioEngine::start()
{
    Q_D(QAudioEngine);
    d->start();
}

/*!
    Stops the engine.
 */
void QAudioEngine::stop()
{
    Q_D(QAudioEngine);
    d->stop();
}

/*!
    \property QAudioEngine::paused

    Pauses the spatial audio engine.
 */
void QAudioEngine::setPaused(bool paused)
{
    Q_D(QAudioEngine);
    d->setPaused(paused);
}

bool QAudioEngine::paused() const
{
    Q_D(const QAudioEngine);
    return d->isPaused();
}

/*!
    Enables room effects such as echos and reverb.

    Enables room effects if \a enabled is true.
    Room effects will only apply if you create one or more \l QAudioRoom objects
    and the listener is inside at least one of the rooms. If the listener is inside
    multiple rooms, the room with the smallest volume will be used.
 */
void QAudioEngine::setRoomEffectsEnabled(bool enabled)
{
    Q_D(QAudioEngine);
    d->setRoomEffectsEnabled(enabled);
}

/*!
    Returns true if room effects are enabled.
 */
bool QAudioEngine::roomEffectsEnabled() const
{
    Q_D(const QAudioEngine);
    return d->roomEffectsEnabled();
}

/*!
    \property QAudioEngine::distanceScale

    Defines the scale of the coordinate system being used by the spatial audio engine.
    By default, all units are in centimeters, in line with the default units being
    used by Qt Quick 3D.

    Set the distance scale to QAudioEngine::DistanceScaleMeter to get units in meters.
*/
void QAudioEngine::setDistanceScale(float scale)
{
    Q_D(QAudioEngine);
    // multiply with 100, to get the conversion to meters that resonance audio uses
    scale /= 100.f;
    if (scale <= 0.0f) {
        qWarning() << "QAudioEngine: Invalid distance scale.";
        return;
    }
    d->setDistanceScale(scale);
}

float QAudioEngine::distanceScale() const
{
    Q_D(const QAudioEngine);
    return d->distanceScale() * 100.f;
}

/*!
    \fn void QAudioEngine::pause()

    Pauses playback.
*/
/*!
    \fn void QAudioEngine::resume()

    Resumes playback.
*/
/*!
    \variable QAudioEngine::DistanceScaleCentimeter
    \internal
*/
/*!
    \variable QAudioEngine::DistanceScaleMeter
    \internal
*/

QT_END_NAMESPACE

#include "moc_qaudioengine.cpp"
