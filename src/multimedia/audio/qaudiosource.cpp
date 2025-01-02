// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qaudio.h"
#include "qaudiodevice.h"
#include "qaudiosystem_p.h"
#include "qaudiosource.h"

#include <private/qplatformmediadevices_p.h>
#include <private/qplatformmediaintegration_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAudioSource
    \brief The QAudioSource class provides an interface for receiving audio data from an audio input device.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio

    You can construct an audio input with the system's
    default audio input device. It is also possible to
    create QAudioSource with a specific QAudioDevice. When
    you create the audio input, you should also send in the
    QAudioFormat to be used for the recording (see the QAudioFormat
    class description for details).

    To record to a file:

    QAudioSource lets you record audio with an audio input device. The
    default constructor of this class will use the systems default
    audio device, but you can also specify a QAudioDevice for a
    specific device. You also need to pass in the QAudioFormat in
    which you wish to record.

    Starting up the QAudioSource is simply a matter of calling start()
    with a QIODevice opened for writing. For instance, to record to a
    file, you can:

    \snippet multimedia-snippets/audio.cpp Audio input class members

    \snippet multimedia-snippets/audio.cpp Audio input setup

    This will start recording if the format specified is supported by
    the input device (you can check this with
    QAudioDevice::isFormatSupported(). In case there are any
    snags, use the error() function to check what went wrong. We stop
    recording in the \c stopRecording() slot.

    \snippet multimedia-snippets/audio.cpp Audio input stop recording

    At any point in time, QAudioSource will be in one of four states:
    active, suspended, stopped, or idle. These states are specified by
    the QAudio::State enum. You can request a state change directly through
    suspend(), resume(), stop(), reset(), and start(). The current
    state is reported by state(). QAudioSink will also signal you
    when the state changes (stateChanged()).

    QAudioSource provides several ways of measuring the time that has
    passed since the start() of the recording. The \c processedUSecs()
    function returns the length of the stream in microseconds written,
    i.e., it leaves out the times the audio input was suspended or idle.
    The elapsedUSecs() function returns the time elapsed since start() was called regardless of
    which states the QAudioSource has been in.

    If an error should occur, you can fetch its reason with error().
    The possible error reasons are described by the QAudio::Error
    enum. The QAudioSource will enter the \l{QAudio::}{StoppedState} when
    an error is encountered.  Connect to the stateChanged() signal to
    handle the error:

    \snippet multimedia-snippets/audio.cpp Audio input state changed

    \sa QAudioSink, QAudioDevice
*/

/*!
    Construct a new audio input and attach it to \a parent.
    The default audio input device is used with the output
    \a format parameters.
*/

QAudioSource::QAudioSource(const QAudioFormat &format, QObject *parent)
    : QAudioSource({}, format, parent)
{
}

/*!
    Construct a new audio input and attach it to \a parent.
    The device referenced by \a audioDevice is used with the input
    \a format parameters.
*/

QAudioSource::QAudioSource(const QAudioDevice &audioDevice, const QAudioFormat &format, QObject *parent):
    QObject(parent)
{
    d = QPlatformMediaDevices::instance()->audioInputDevice(format, audioDevice, parent);
    if (d) {
        connect(d, &QPlatformAudioSource::stateChanged, this, [this](QAudio::State state) {
            // if the signal has been emitted from another thread,
            // the state may be already changed by main one
            if (state == d->state())
                emit stateChanged(state);
        });
    }
    else
        qWarning() << ("No audio device detected");

}

/*!
    \fn bool QAudioSource::isNull() const

    Returns \c true if the audio source is \c null, otherwise returns \c false.
*/

/*!
    Destroy this audio input.
*/

QAudioSource::~QAudioSource()
{
    delete d;
}

/*!
    Starts transferring audio data from the system's audio input to the \a device.
    The \a device must have been opened in the \l{QIODevice::WriteOnly}{WriteOnly},
    \l{QIODevice::Append}{Append} or \l{QIODevice::ReadWrite}{ReadWrite} modes.

    If the QAudioSource is able to successfully get audio data, state() returns
    either QAudio::ActiveState or QAudio::IdleState, error() returns QAudio::NoError
    and the stateChanged() signal is emitted.

    If a problem occurs during this process, error() returns QAudio::OpenError,
    state() returns QAudio::StoppedState and the stateChanged() signal is emitted.

    \sa QIODevice
*/

void QAudioSource::start(QIODevice* device)
{
    if (!d)
        return;
    d->elapsedTime.start();
    d->start(device);
}

/*!
    Returns a pointer to the internal QIODevice being used to transfer data from
    the system's audio input. The device will already be open and
    \l{QIODevice::read()}{read()} can read data directly from it.

    \note The pointer will become invalid after the stream is stopped or
    if you start another stream.

    If the QAudioSource is able to access the system's audio device, state() returns
    QAudio::IdleState, error() returns QAudio::NoError
    and the stateChanged() signal is emitted.

    If a problem occurs during this process, error() returns QAudio::OpenError,
    state() returns QAudio::StoppedState and the stateChanged() signal is emitted.

    \sa QIODevice
*/

QIODevice* QAudioSource::start()
{
    if (!d)
        return nullptr;
    d->elapsedTime.start();
    return d->start();
}

/*!
    Returns the QAudioFormat being used.
*/

QAudioFormat QAudioSource::format() const
{
    return d ? d->format() : QAudioFormat();
}

/*!
    Stops the audio input, detaching from the system resource.

    Sets error() to QAudio::NoError, state() to QAudio::StoppedState and
    emit stateChanged() signal.
*/

void QAudioSource::stop()
{
    if (d)
        d->stop();
}

/*!
    Drops all audio data in the buffers, resets buffers to zero.
*/

void QAudioSource::reset()
{
    if (d)
        d->reset();
}

/*!
    Stops processing audio data, preserving buffered audio data.

    Sets error() to QAudio::NoError, state() to QAudio::SuspendedState and
    emit stateChanged() signal.
*/

void QAudioSource::suspend()
{
    if (d)
        d->suspend();
}

/*!
    Resumes processing audio data after a suspend().

    Sets error() to QAudio::NoError.
    Sets state() to QAudio::ActiveState if you previously called start(QIODevice*).
    Sets state() to QAudio::IdleState if you previously called start().
    emits stateChanged() signal.
*/

void QAudioSource::resume()
{
    if (d)
        d->resume();
}

/*!
    Sets the audio buffer size to \a value bytes.

    Note: This function can be called anytime before start(), calls to this
    are ignored after start(). It should not be assumed that the buffer size
    set is the actual buffer size used, calling bufferSize() anytime after start()
    will return the actual buffer size being used.

*/

void QAudioSource::setBufferSize(qsizetype value)
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

qsizetype QAudioSource::bufferSize() const
{
    return d ? d->bufferSize() : 0;
}

/*!
    Returns the amount of audio data available to read in bytes.

    Note: returned value is only valid while in QAudio::ActiveState or QAudio::IdleState
    state, otherwise returns zero.
*/

qsizetype QAudioSource::bytesAvailable() const
{
    /*
    -If not ActiveState|IdleState, return 0
    -return amount of audio data available to read
    */
    return d ? d->bytesReady() : 0;
}

/*!
    Sets the input volume to \a volume.

    The volume is scaled linearly from \c 0.0 (silence) to \c 1.0 (full volume). Values outside this
    range will be clamped.

    If the device does not support adjusting the input
    volume then \a volume will be ignored and the input
    volume will remain at 1.0.

    The default volume is \c 1.0.

    Note: Adjustments to the volume will change the volume of this audio stream, not the global volume.
*/
void QAudioSource::setVolume(qreal volume)
{
    if (!d)
        return;
    qreal v = qBound(qreal(0.0), volume, qreal(1.0));
    d->setVolume(v);
}

/*!
    Returns the input volume.

    If the device does not support adjusting the input volume
    the returned value will be 1.0.
*/
qreal QAudioSource::volume() const
{
    return d ? d->volume() : 1.0;
}

/*!
    Returns the amount of audio data processed since start()
    was called in microseconds.
*/

qint64 QAudioSource::processedUSecs() const
{
    return d ? d->processedUSecs() : 0;
}

/*!
    Returns the microseconds since start() was called, including time in Idle and
    Suspend states.
*/

#include <qdebug.h>

qint64 QAudioSource::elapsedUSecs() const
{
    return state() == QAudio::StoppedState ? 0 : d->elapsedTime.nsecsElapsed()/1000;
}

/*!
    Returns the error state.
*/

QAudio::Error QAudioSource::error() const
{
    return d ? d->error() : QAudio::OpenError;
}

/*!
    Returns the state of audio processing.
*/

QAudio::State QAudioSource::state() const
{
    return d ? d->state() : QAudio::StoppedState;
}

/*!
    \fn QAudioSource::stateChanged(QAudio::State state)
    This signal is emitted when the device \a state has changed.
*/

QT_END_NAMESPACE

#include "moc_qaudiosource.cpp"

