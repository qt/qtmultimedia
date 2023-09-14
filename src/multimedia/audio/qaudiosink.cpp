// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qaudio.h"
#include "qaudiodevice.h"
#include "qaudiosystem_p.h"
#include "qaudiosink.h"

#include <private/qplatformmediadevices_p.h>
#include <private/qplatformmediaintegration_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAudioSink
    \brief The QAudioSink class provides an interface for sending audio data to
    an audio output device.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio

    You can construct an audio output with the system's
    default audio output device. It is also possible to
    create QAudioSink with a specific QAudioDevice. When
    you create the audio output, you should also send in
    the QAudioFormat to be used for the playback (see
    the QAudioFormat class description for details).

    To play a file:

    Starting to play an audio stream is simply a matter of calling
    start() with a QIODevice. QAudioSink will then fetch the data it
    needs from the io device. So playing back an audio file is as
    simple as:

    \snippet multimedia-snippets/audio.cpp Audio output class members

    \snippet multimedia-snippets/audio.cpp Audio output setup

    The file will start playing assuming that the audio system and
    output device support it. If you run out of luck, check what's
    up with the error() function.

    After the file has finished playing, we need to stop the device:

    \snippet multimedia-snippets/audio.cpp Audio output stop

    At any given time, the QAudioSink will be in one of four states:
    active, suspended, stopped, or idle. These states are described
    by the QAudio::State enum.
    State changes are reported through the stateChanged() signal. You
    can use this signal to, for instance, update the GUI of the
    application; the mundane example here being changing the state of
    a \c { play/pause } button. You request a state change directly
    with suspend(), stop(), reset(), resume(), and start().

    If an error occurs, you can fetch the \l{QAudio::Error}{error
    type} with the error() function. Please see the QAudio::Error enum
    for a description of the possible errors that are reported. When
    QAudio::UnderrunError is encountered, the state changes to QAudio::IdleState,
    when another error is encountered, the state changes to QAudio::StoppedState.
    You can check for errors by connecting to the stateChanged()
    signal:

    \snippet multimedia-snippets/audio.cpp Audio output state changed

    \sa QAudioSource, QAudioDevice
*/

/*!
    Construct a new audio output and attach it to \a parent.
    The default audio output device is used with the output
    \a format parameters.
*/
QAudioSink::QAudioSink(const QAudioFormat &format, QObject *parent)
    : QAudioSink({}, format, parent)
{
}

/*!
    Construct a new audio output and attach it to \a parent.
    The device referenced by \a audioDevice is used with the output
    \a format parameters.
*/
QAudioSink::QAudioSink(const QAudioDevice &audioDevice, const QAudioFormat &format, QObject *parent):
    QObject(parent)
{
    d = QPlatformMediaDevices::instance()->audioOutputDevice(format, audioDevice, parent);
    if (d)
        connect(d, &QPlatformAudioSink::stateChanged, this, [this](QAudio::State state) {
            // if the signal has been emitted from another thread,
            // the state may be already changed by main one
            if (state == d->state())
                emit stateChanged(state);
        });
    else
        qWarning() << ("No audio device detected");
}

/*!
    \fn bool QAudioSink::isNull() const

    Returns \c true is the QAudioSink instance is \c null, otherwise returns
    \c false.
*/

/*!
    Destroys this audio output.

    This will release any system resources used and free any buffers.
*/
QAudioSink::~QAudioSink()
{
    delete d;
}

/*!
    Returns the QAudioFormat being used.

*/
QAudioFormat QAudioSink::format() const
{
    return d ? d->format() : QAudioFormat();
}

/*!
    Starts transferring audio data from the \a device to the system's audio output.
    The \a device must have been opened in the \l{QIODevice::ReadOnly}{ReadOnly} or
    \l{QIODevice::ReadWrite}{ReadWrite} modes.

    If the QAudioSink is able to successfully output audio data, state() returns
    QAudio::ActiveState, error() returns QAudio::NoError
    and the stateChanged() signal is emitted.

    If a problem occurs during this process, error() returns QAudio::OpenError,
    state() returns QAudio::StoppedState and the stateChanged() signal is emitted.

    \sa QIODevice
*/
void QAudioSink::start(QIODevice* device)
{
    if (!d)
        return;
    d->elapsedTime.restart();
    d->start(device);
}

/*!
    Returns a pointer to the internal QIODevice being used to transfer data to
    the system's audio output. The device will already be open and
    \l{QIODevice::write()}{write()} can write data directly to it.

    \note The pointer will become invalid after the stream is stopped or
    if you start another stream.

    If the QAudioSink is able to access the system's audio device, state() returns
    QAudio::IdleState, error() returns QAudio::NoError
    and the stateChanged() signal is emitted.

    If a problem occurs during this process, error() returns QAudio::OpenError,
    state() returns QAudio::StoppedState and the stateChanged() signal is emitted.

    \sa QIODevice
*/
QIODevice* QAudioSink::start()
{
    if (!d)
        return nullptr;
    d->elapsedTime.restart();
    return d->start();
}

/*!
    Stops the audio output, detaching from the system resource.

    Sets error() to QAudio::NoError, state() to QAudio::StoppedState and
    emit stateChanged() signal.
*/
void QAudioSink::stop()
{
    if (d)
        d->stop();
}

/*!
    Drops all audio data in the buffers, resets buffers to zero.

*/
void QAudioSink::reset()
{
    if (d)
        d->reset();
}

/*!
    Stops processing audio data, preserving buffered audio data.

    Sets error() to QAudio::NoError, state() to QAudio::SuspendedState and
    emits stateChanged() signal.
*/
void QAudioSink::suspend()
{
    if (d)
        d->suspend();
}

/*!
    Resumes processing audio data after a suspend().

    Sets state() to the state the sink had when suspend() was called, and sets
    error() to QAudioError::NoError. This function does nothing if the audio sink's
    state is not QAudio::SuspendedState.
*/
void QAudioSink::resume()
{
    if (d)
        d->resume();
}

/*!
    Returns the number of free bytes available in the audio buffer.

    \note The returned value is only valid while in QAudio::ActiveState or QAudio::IdleState
    state, otherwise returns zero.
*/
qsizetype QAudioSink::bytesFree() const
{
    return d ? d->bytesFree() : 0;
}

/*!
    Sets the audio buffer size to \a value in bytes.

    \note This function can be called anytime before start().  Calls to this
    are ignored after start(). It should not be assumed that the buffer size
    set is the actual buffer size used - call bufferSize() anytime after start()
    to return the actual buffer size being used.
*/
void QAudioSink::setBufferSize(qsizetype value)
{
    if (d)
        d->setBufferSize(value);
}

/*!
    Returns the audio buffer size in bytes.

    If called before start(), returns platform default value.
    If called before start() but setBufferSize() was called prior, returns value set by setBufferSize().
    If called after start(), returns the actual buffer size being used. This may not be what was set previously
    by setBufferSize().

*/
qsizetype QAudioSink::bufferSize() const
{
    return d ? d->bufferSize() : 0;
}

/*!
    Returns the amount of audio data processed since start()
    was called (in microseconds).
*/
qint64 QAudioSink::processedUSecs() const
{
    return d ? d->processedUSecs() : 0;
}

/*!
    Returns the microseconds since start() was called, including time in Idle and
    Suspend states.
*/
qint64 QAudioSink::elapsedUSecs() const
{
    return state() == QAudio::StoppedState ? 0 : d->elapsedTime.nsecsElapsed()/1000;
}

/*!
    Returns the error state.
*/
QAudio::Error QAudioSink::error() const
{
    return d ? d->error() : QAudio::OpenError;
}

/*!
    Returns the state of audio processing.
*/
QAudio::State QAudioSink::state() const
{
    return d ? d->state() : QAudio::StoppedState;
}

/*!
    Sets the output volume to \a volume.

    The volume is scaled linearly from \c 0.0 (silence) to \c 1.0 (full volume).
    Values outside this range will be clamped.

    The default volume is \c 1.0.

    \note Adjustments to the volume will change the volume of this audio stream,
    not the global volume.

    UI volume controls should usually be scaled non-linearly. For example, using
    a logarithmic scale will produce linear changes in perceived loudness, which
    is what a user would normally expect from a volume control. See
    QAudio::convertVolume() for more details.
*/
void QAudioSink::setVolume(qreal volume)
{
    if (!d)
        return;
    qreal v = qBound(qreal(0.0), volume, qreal(1.0));
    d->setVolume(v);
}

/*!
    Returns the volume between 0.0 and 1.0 inclusive.
*/
qreal QAudioSink::volume() const
{
    return d ? d->volume() : 1.0;
}

/*!
    \fn QAudioSink::stateChanged(QAudio::State state)
    This signal is emitted when the device \a state has changed.
    This is the current state of the audio output.
*/

QT_END_NAMESPACE

#include "moc_qaudiosink.cpp"
