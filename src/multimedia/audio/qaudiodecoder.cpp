// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiodecoder.h"

#include <private/qaudiodecoder_p.h>
#include <private/qmultimediautils_p.h>
#include <private/qplatformaudiodecoder_p.h>
#include <private/qplatformmediaintegration_p.h>

#include <QtCore/qcoreevent.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qtimer.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAudioDecoder
    \brief The QAudioDecoder class implements decoding audio.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio

    \preliminary

    The QAudioDecoder class is a high level class for decoding
    audio media files.  It is similar to the QMediaPlayer class except
    that audio is provided back through this API rather than routed
    directly to audio hardware.

    \sa QAudioBuffer
*/

/*!
    Construct an QAudioDecoder instance with \a parent.
*/
QAudioDecoder::QAudioDecoder(QObject *parent) : QObject{ *new QAudioDecoderPrivate, parent }
{
    Q_D(QAudioDecoder);

    auto maybeDecoder = QPlatformMediaIntegration::instance()->createAudioDecoder(this);
    if (maybeDecoder) {
        d->decoder.reset(maybeDecoder.value());
    } else {
        qWarning() << "Failed to initialize QAudioDecoder" << maybeDecoder.error();
    }
}

/*!
    Destroys the audio decoder object.
*/
QAudioDecoder::~QAudioDecoder() = default;

/*!
    Returns true is audio decoding is supported on this platform.
*/
bool QAudioDecoder::isSupported() const
{
    Q_D(const QAudioDecoder);

    return bool(d->decoder);
}

/*!
    \property QAudioDecoder::isDecoding
    \brief \c true if the decoder is currently running and decoding audio data.
*/
bool QAudioDecoder::isDecoding() const
{
    Q_D(const QAudioDecoder);

    return d->decoder && d->decoder->isDecoding();
}

/*!

    Returns the current error state of the QAudioDecoder.
*/
QAudioDecoder::Error QAudioDecoder::error() const
{
    Q_D(const QAudioDecoder);
    return d->decoder ? d->decoder->error() : NotSupportedError;
}

/*!
    \property QAudioDecoder::error

    Returns a human readable description of the current error, or
    an empty string is there is no error.
*/
QString QAudioDecoder::errorString() const
{
    Q_D(const QAudioDecoder);
    if (!d->decoder)
        return tr("QAudioDecoder not supported.");
    return d->decoder->errorString();
}

/*!
    Starts decoding the audio resource.

    As data gets decoded, the \l bufferReady() signal will be emitted
    when enough data has been decoded.  Calling \l read() will then return
    an audio buffer without blocking.

    If you call read() before a buffer is ready, an invalid buffer will
    be returned, again without blocking.

    \sa read()
*/
void QAudioDecoder::start()
{
    Q_D(QAudioDecoder);

    if (!d->decoder)
        return;

    // Reset error conditions
    d->decoder->clearError();
    d->decoder->start();
}

/*!
    Stop decoding audio.  Calling \l start() again will resume decoding from the beginning.
*/
void QAudioDecoder::stop()
{
    Q_D(QAudioDecoder);

    if (d->decoder)
        d->decoder->stop();
}

/*!
    Returns the current file name to decode.
    If \l setSourceDevice was called, this will
    be empty.
*/
QUrl QAudioDecoder::source() const
{
    Q_D(const QAudioDecoder);
    return d->unresolvedUrl;
}

/*!
    Sets the current audio file name to \a fileName.

    When this property is set any current decoding is stopped,
    and any audio buffers are discarded.

    You can only specify either a source filename or
    a source QIODevice.  Setting one will unset the other.
*/
void QAudioDecoder::setSource(const QUrl &fileName)
{
    Q_D(QAudioDecoder);

    if (!d->decoder)
        return;

    d->decoder->clearError();
    d->unresolvedUrl = fileName;
    d->decoder->setSourceDevice(nullptr);
    QUrl url = qMediaFromUserInput(fileName);
    d->decoder->setSource(url);
}

/*!
    Returns the current source QIODevice, if one was set.
    If \l setSource() was called, this will be a nullptr.
*/
QIODevice *QAudioDecoder::sourceDevice() const
{
    Q_D(const QAudioDecoder);
    return d->decoder ? d->decoder->sourceDevice() : nullptr;
}

/*!
    Sets the current audio QIODevice to \a device.

    When this property is set any current decoding is stopped,
    and any audio buffers are discarded.

    You can only specify either a source filename or
    a source QIODevice.  Setting one will unset the other.
*/
void QAudioDecoder::setSourceDevice(QIODevice *device)
{
    Q_D(QAudioDecoder);
    if (d->decoder) {
        d->unresolvedUrl = QUrl{};
        d->decoder->setSourceDevice(device);
    }
}

/*!
    Returns the audio format the decoder is set to.

    \note This may be different than the format of the decoded
    samples, if the audio format was set to an invalid one.

    \sa setAudioFormat(), formatChanged()
*/
QAudioFormat QAudioDecoder::audioFormat() const
{
    Q_D(const QAudioDecoder);
    return d->decoder ? d->decoder->audioFormat() : QAudioFormat{};
}

/*!
    Set the desired audio format for decoded samples to \a format.

    This property can only be set while the decoder is stopped.
    Setting this property at other times will be ignored.

    If the decoder does not support this format, \l error() will
    be set to \c FormatError.

    If you do not specify a format, the format of the decoded
    audio itself will be used.  Otherwise, some format conversion
    will be applied.

    If you wish to reset the decoded format to that of the original
    audio file, you can specify an invalid \a format.

    \warning Setting a desired audio format is not yet supported
    on the Android backend. It does work with the default FFMPEG
    backend.
*/
void QAudioDecoder::setAudioFormat(const QAudioFormat &format)
{
    if (isDecoding())
        return;

    Q_D(QAudioDecoder);

    if (d->decoder)
        d->decoder->setAudioFormat(format);
}

/*!
    Returns true if a buffer is available to be read,
    and false otherwise.  If there is no buffer available, calling
    the \l read() function will return an invalid buffer.
*/
bool QAudioDecoder::bufferAvailable() const
{
    Q_D(const QAudioDecoder);
    return d->decoder && d->decoder->bufferAvailable();
}

/*!
    Returns position (in milliseconds) of the last buffer read from
    the decoder or -1 if no buffers have been read.
*/

qint64 QAudioDecoder::position() const
{
    Q_D(const QAudioDecoder);
    return d->decoder ? d->decoder->position() : -1;
}

/*!
    Returns total duration (in milliseconds) of the audio stream or -1
    if not available.
*/

qint64 QAudioDecoder::duration() const
{
    Q_D(const QAudioDecoder);
    return d->decoder ? d->decoder->duration() : -1;
}

/*!
    Read a buffer from the decoder, if one is available. Returns an invalid buffer
    if there are no decoded buffers currently available, or on failure.  In both cases
    this function will not block.

    You should either respond to the \l bufferReady() signal or check the
    \l bufferAvailable() function before calling read() to make sure
    you get useful data.
*/

QAudioBuffer QAudioDecoder::read() const
{
    Q_D(const QAudioDecoder);
    return d->decoder ? d->decoder->read() : QAudioBuffer{};
}

// Enums
/*!
    \enum QAudioDecoder::Error

    Defines a media player error condition.

    \value NoError No error has occurred.
    \value ResourceError A media resource couldn't be resolved.
    \value FormatError The format of a media resource isn't supported.
    \value AccessDeniedError There are not the appropriate permissions to play a media resource.
    \value NotSupportedError QAudioDecoder is not supported on this platform
*/

// Signals
/*!
    \fn void QAudioDecoder::error(QAudioDecoder::Error error)

    Signals that an \a error condition has occurred.

    \sa errorString()
*/

/*!
    \fn void QAudioDecoder::sourceChanged()

    Signals that the current source of the decoder has changed.

    \sa source(), sourceDevice()
*/

/*!
    \fn void QAudioDecoder::formatChanged(const QAudioFormat &format)

    Signals that the current audio format of the decoder has changed to \a format.

    \sa audioFormat(), setAudioFormat()
*/

/*!
    \fn void QAudioDecoder::bufferReady()

    Signals that a new decoded audio buffer is available to be read.

    \sa read(), bufferAvailable()
*/

/*!
    \fn void QAudioDecoder::bufferAvailableChanged(bool available)

    Signals the availability (if \a available is true) of a new buffer.

    If \a available is false, there are no buffers available.

    \sa bufferAvailable(), bufferReady()
*/

/*!
    \fn void QAudioDecoder::finished()

    Signals that the decoding has finished successfully.
    If decoding fails, error signal is emitted instead.

    \sa start(), stop(), error()
*/

/*!
    \fn void QAudioDecoder::positionChanged(qint64 position)

    Signals that the current \a position of the decoder has changed.

    \sa durationChanged()
*/

/*!
    \fn void QAudioDecoder::durationChanged(qint64 duration)

    Signals that the estimated \a duration of the decoded data has changed.

    \sa positionChanged()
*/

// Properties
/*!
    \property QAudioDecoder::source
    \brief the active filename being decoded by the decoder object.
*/

/*!
    \property QAudioDecoder::bufferAvailable
    \brief whether there is a decoded audio buffer available
*/

QT_END_NAMESPACE

#include "moc_qaudiodecoder.cpp"
