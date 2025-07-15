// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiosource.h"

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>
#include <QtMultimedia/private/qplatformaudiodevices_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

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
    the QtAudio::State enum. You can request a state change directly through
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
    The possible error reasons are described by the QtAudio::Error
    enum. The QAudioSource will enter the \l{QtAudio::}{StoppedState} when
    an error is encountered. Connect to the stateChanged() signal to
    handle the error:

    \snippet multimedia-snippets/audio.cpp Audio input state changed

    \sa QAudioSink, QAudioDevice
*/

/*!
    Construct a new audio input and attach it to \a parent.
    The default audio input device is used with the output
    \a format parameters. If \a format is default-initialized,
    the format will be set to the preferred format of the audio device.
*/

QAudioSource::QAudioSource(const QAudioFormat &format, QObject *parent)
    : QAudioSource({}, format, parent)
{
}

/*!
    Construct a new audio input and attach it to \a parent.
    The device referenced by \a audioDevice is used with the input
    \a format parameters. If \a format is default-initialized,
    the format will be set to the preferred format of \a audioDevice.
*/

QAudioSource::QAudioSource(const QAudioDevice &audioDevice, const QAudioFormat &format, QObject *parent):
    QObject(parent)
{
    d = QPlatformMediaIntegration::instance()->audioDevices()->audioInputDevice(format, audioDevice,
                                                                                this);
    if (d)
        connect(d, &QPlatformAudioSource::stateChanged, this, &QAudioSource::stateChanged);
    else
        qWarning("No audio device detected");
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

static bool validateFormatAtStart(QPlatformAudioSource *d)
{
    if (!d->format().isValid()) {
        qWarning() << "QAudioSource::start: QAudioFormat not valid";
        d->setError(QAudio::OpenError);
        return false;
    }

    if (!d->isFormatSupported(d->format())) {
        qWarning() << "QAudioSource::start: QAudioFormat not supported by QAudioDevice";
        d->setError(QAudio::OpenError);
        return false;
    }
    return true;
};

/*!
    Starts transferring audio data from the system's audio input to the \a device.
    The \a device must have been opened in the \l{QIODevice::WriteOnly}{WriteOnly},
    \l{QIODevice::Append}{Append} or \l{QIODevice::ReadWrite}{ReadWrite} modes.

    If the QAudioSource is able to successfully get audio data, state() returns
    either QtAudio::ActiveState or QtAudio::IdleState, error() returns QtAudio::NoError
    and the stateChanged() signal is emitted.

    If a problem occurs during this process, error() returns QtAudio::OpenError,
    state() returns QtAudio::StoppedState and the stateChanged() signal is emitted.

    \sa QIODevice
*/

void QAudioSource::start(QIODevice* device)
{
    if (!d)
        return;

    d->setError(QAudio::NoError);

    if (!device->isWritable()) {
        qWarning() << "QAudioSource::start: QIODevice is not writable";
        d->setError(QAudio::OpenError);
        return;
    }

    if (!validateFormatAtStart(d))
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
    QtAudio::IdleState, error() returns QtAudio::NoError
    and the stateChanged() signal is emitted.

    If a problem occurs during this process, error() returns QtAudio::OpenError,
    state() returns QtAudio::StoppedState and the stateChanged() signal is emitted.

    \sa QIODevice
*/

QIODevice* QAudioSource::start()
{
    if (!d)
        return nullptr;

    d->setError(QAudio::NoError);

    if (!validateFormatAtStart(d))
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

    Sets error() to QtAudio::NoError, state() to QtAudio::StoppedState and
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

    Sets error() to QtAudio::NoError, state() to QtAudio::SuspendedState and
    emit stateChanged() signal.
*/

void QAudioSource::suspend()
{
    if (d)
        d->suspend();
}

/*!
    Resumes processing audio data after a suspend().

    Sets error() to QtAudio::NoError.
    Sets state() to QtAudio::ActiveState if you previously called start(QIODevice*).
    Sets state() to QtAudio::IdleState if you previously called start().
    emits stateChanged() signal.
*/

void QAudioSource::resume()
{
    if (d)
        d->resume();
}

/*!
    Sets the audio buffer size to \a value bytes.

    \note This function can be called anytime before start(), calls to this
    are ignored after start(). It should not be assumed that the buffer size
    set is the actual buffer size used, calling bufferSize() anytime after start()
    will return the actual buffer size being used.

    \sa setBufferFrameCount
    \since 6.10
*/

void QAudioSource::setBufferSize(qsizetype value)
{
    if (d)
        d->setBufferSize(value);
}

/*!
    Returns the audio buffer size in bytes.

    If called before \l start(), returns platform default value.
    If called before \c start() but \l setBufferSize() or \l setBufferFrameCount() was called prior, returns
    value set by \c setBufferSize() or \c setBufferFrameCount(). If called after \c start(), returns the actual
    buffer size being used. This may not be what was set previously by
    \c setBufferSize() or \c setBufferFrameCount().

    \sa bufferFrameCount
    \since 6.10
*/

qsizetype QAudioSource::bufferSize() const
{
    return d ? d->bufferSize() : 0;
}

/*!
    Sets the audio buffer size to \a value in frame count.

    \note This function can be called anytime before start().  Calls to this
    are ignored after start(). It should not be assumed that the buffer size
    set is the actual buffer size used - call bufferFrameCount() anytime
    after start() to return the actual buffer size being used.

    \sa setBufferSize
*/

void QAudioSource::setBufferFrameCount(qsizetype value)
{
    if (d)
        setBufferSize(d->format().bytesForFrames(value));
}

/*!
    Returns the audio buffer size in frames.

    If called before \l start(), returns platform default value.
    If called before \c start() but \l setBufferSize() or \l setBufferFrameCount() was called prior, returns
    value set by \c setBufferSize() or \c setBufferFrameCount(). If called after \c start(), returns the actual
    buffer size being used. This may not be what was set previously by
    \c setBufferSize() or \c setBufferFrameCount().

    \sa bufferSize
*/

qsizetype QAudioSource::bufferFrameCount() const
{
    return d ? d->format().framesForBytes(bufferSize()) : 0;
}

/*!
    Returns the amount of audio data available to read in bytes.

    \note returned value is only valid while in QtAudio::ActiveState or QtAudio::IdleState
    state, otherwise returns zero.

    \sa framesAvailable
*/

qsizetype QAudioSource::bytesAvailable() const
{
    return d ? d->bytesReady() : 0;
}

/*!
    Returns the amount of audio data available to read in frames.

    Note: returned value is only valid while in QtAudio::ActiveState or QtAudio::IdleState
    state, otherwise returns zero.

    \sa bytesAvailable
    \since 6.10
*/

qsizetype QAudioSource::framesAvailable() const
{

    return d ? d->format().framesForBytes(bytesAvailable()) : 0;
}

/*!
    Sets the input volume to \a volume.

    The volume is scaled linearly from \c 0.0 (silence) to \c 1.0 (full volume). Values outside this
    range will be clamped.

    If the device does not support adjusting the input
    volume then \a volume will be ignored and the input
    volume will remain at 1.0.

    The default volume is \c 1.0.

    \note Adjustments to the volume will change the volume of this audio stream, not the global
    volume.
*/
void QAudioSource::setVolume(qreal volume)
{
    if (!d)
        return;

    std::optional<float> newVolume = QAudioHelperInternal::sanitizeVolume(volume, this->volume());
    if (newVolume)
        d->setVolume(*newVolume);
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

qint64 QAudioSource::elapsedUSecs() const
{
    return state() == QAudio::StoppedState ? 0 : d->elapsedTime.nsecsElapsed()/1000;
}

/*!
    Returns the error state.
*/

QtAudio::Error QAudioSource::error() const
{
    return d ? d->error() : QAudio::OpenError;
}

/*!
    Returns the state of audio processing.
*/

QtAudio::State QAudioSource::state() const
{
    return d ? d->state() : QAudio::StoppedState;
}

/*!
    \fn QAudioSource::stateChanged(QtAudio::State state)
    This signal is emitted when the device \a state has changed.

    \note The QtAudio namespace was named QAudio up to and including Qt 6.6.
    String-based connections to this signal have to use \c{QAudio::State} as
    the parameter type: \c{connect(source, SIGNAL(stateChanged(QAudio::State)), ...);}
*/

QT_END_NAMESPACE

#include "moc_qaudiosource.cpp"

