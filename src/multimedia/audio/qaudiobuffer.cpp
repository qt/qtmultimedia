// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiobuffer.h"

#include <QObject>
#include <QDebug>

QT_BEGIN_NAMESPACE

class QAudioBufferPrivate : public QSharedData
{
public:
    QAudioBufferPrivate(const QAudioFormat &f, const QByteArray &d, qint64 start)
        : format(f), data(d), startTime(start)
    {
    }

    QAudioFormat format;
    QByteArray data;
    qint64 startTime;
};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QAudioBufferPrivate);

/*!
    \class QAbstractAudioBuffer
    \internal
*/

/*!
    \class QAudioBuffer
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio
    \brief The QAudioBuffer class represents a collection of audio samples with a specific format
   and sample rate.

    QAudioBuffer is used by the QAudioDecoder class to hand decoded audio data over to the
   application. An audio buffer contains data in a certain QAudioFormat that can be queried using
   format(). It is also tagged with timing and duration information.

    To access the data stored inside the buffer, use the data() or constData() methods.

    Audio buffers are explicitly shared, in most cases, you should call detach() before
    modifying the data.
*/

/*!
    Create a new, empty, invalid buffer.
 */
QAudioBuffer::QAudioBuffer() noexcept = default;

/*!
    Creates a new audio buffer from \a other. Audio buffers are explicitly shared,
    you should call detach() on the buffer to make a copy that can then be modified.
 */
QAudioBuffer::QAudioBuffer(const QAudioBuffer &other) noexcept = default;

/*!
    Creates a new audio buffer from the supplied \a data, in the
    given \a format.  The format will determine how the number
    and sizes of the samples are interpreted from the \a data.

    If the supplied \a data is not an integer multiple of the
    calculated frame size, the excess data will not be used.

    This audio buffer will copy the contents of \a data.

    \a startTime (in microseconds) indicates when this buffer
    starts in the stream.
    If this buffer is not part of a stream, set it to -1.
 */
QAudioBuffer::QAudioBuffer(const QByteArray &data, const QAudioFormat &format, qint64 startTime)
{
    if (!format.isValid() || !data.size())
        return;
    d = new QAudioBufferPrivate(format, data, startTime);
}

/*!
    Creates a new audio buffer with space for \a numFrames frames of
    the given \a format.  The individual samples will be initialized
    to the default for the format.

    \a startTime (in microseconds) indicates when this buffer
    starts in the stream.
    If this buffer is not part of a stream, set it to -1.
 */
QAudioBuffer::QAudioBuffer(int numFrames, const QAudioFormat &format, qint64 startTime)
{
    if (!format.isValid() || !numFrames)
        return;

    QByteArray data(format.bytesForFrames(numFrames), '\0');
    d = new QAudioBufferPrivate(format, data, startTime);
}

/*!
    \fn QAudioBuffer::QAudioBuffer(QAudioBuffer &&other)

    Constructs a QAudioBuffer by moving from \a other.
*/

/*!
   \fn void QAudioBuffer::swap(QAudioBuffer &other) noexcept

   Swaps the audio buffer with \a other.
*/

/*!
    \fn QAudioBuffer &QAudioBuffer::operator=(QAudioBuffer &&other)

    Moves \a other into this QAudioBuffer.
*/

/*!
    Assigns the \a other buffer to this.
 */
QAudioBuffer &QAudioBuffer::operator=(const QAudioBuffer &other) = default;

/*!
    Destroys this audio buffer.
 */
QAudioBuffer::~QAudioBuffer() = default;

/*! \fn bool QAudioBuffer::isValid() const noexcept

    Returns true if this is a valid buffer.  A valid buffer
    has more than zero frames in it and a valid format.
 */

/*!
    Detaches this audio buffers from other copies that might share data with it.
*/
void QAudioBuffer::detach()
{
    if (!d)
        return;
    d = new QAudioBufferPrivate(*d);
}

/*!
    Returns the \l {QAudioFormat}{format} of this buffer.

    Several properties of this format influence how
    the \l duration() or \l byteCount() are calculated
    from the \l frameCount().
 */
QAudioFormat QAudioBuffer::format() const noexcept
{
    if (!d)
        return QAudioFormat();
    return d->format;
}

/*!
    Returns the number of complete audio frames in this buffer.

    An audio frame is an interleaved set of one sample per channel
    for the same instant in time.
*/
qsizetype QAudioBuffer::frameCount() const noexcept
{
    if (!d)
        return 0;
    return d->format.framesForBytes(d->data.size());
}

/*!
    Returns the number of samples in this buffer.

    If the format of this buffer has multiple channels,
    then this count includes all channels.  This means
    that a stereo buffer with 1000 samples in total will
    have 500 left samples and 500 right samples (interleaved),
    and this function will return 1000.

    \sa frameCount()
*/
qsizetype QAudioBuffer::sampleCount() const noexcept
{
    return frameCount() * format().channelCount();
}

/*!
    Returns the size of this buffer, in bytes.
 */
qsizetype QAudioBuffer::byteCount() const noexcept
{
    return d ? d->data.size() : 0;
}

/*!
    Returns the duration of audio in this buffer, in microseconds.

    This depends on the \l format(), and the \l frameCount().
*/
qint64 QAudioBuffer::duration() const noexcept
{
    return format().durationForFrames(frameCount());
}

/*!
    Returns the time in a stream that this buffer starts at (in microseconds).

    If this buffer is not part of a stream, this will return -1.
 */
qint64 QAudioBuffer::startTime() const noexcept
{
    if (!d)
        return -1;
    return d->startTime;
}

/*!
    Returns a pointer to this buffer's data.  You can only read it.

    This method is preferred over the const version of \l data() to
    prevent unnecessary copying.

    There is also a templatized version of this constData() function that
    allows you to retrieve a specific type of read-only pointer to
    the data.  Note that there is no checking done on the format of
    the audio buffer - this is simply a convenience function.

    \code
    // With a 16bit sample buffer:
    const quint16 *data = buffer->constData<quint16>();
    \endcode

*/
const void *QAudioBuffer::constData() const noexcept
{
    if (!d)
        return nullptr;
    return d->data.constData();
}

/*!
    Returns a pointer to this buffer's data.  You can only read it.

    You should use the \l constData() function rather than this
    to prevent accidental deep copying.

    There is also a templatized version of this data() function that
    allows you to retrieve a specific type of read-only pointer to
    the data.  Note that there is no checking done on the format of
    the audio buffer - this is simply a convenience function.

    \code
    // With a 16bit sample const buffer:
    const quint16 *data = buffer->data<quint16>();
    \endcode
*/
const void *QAudioBuffer::data() const noexcept
{
    if (!d)
        return nullptr;
    return d->data.constData();
}

/*
    Template data/constData functions caused override problems with qdoc,
    so moved their docs into the non template versions.
*/

/*!
    Returns a pointer to this buffer's data.  You can modify the
    data through the returned pointer.

    Since QAudioBuffer objects are explicitly shared, you should usually
    call detach() before modifying the data through this function.

    There is also a templatized version of data() allows you to retrieve
    a specific type of pointer to the data.  Note that there is no
    checking done on the format of the audio buffer - this is
    simply a convenience function.

    \code
    // With a 16bit sample buffer:
    quint16 *data = buffer->data<quint16>(); // May cause deep copy
    \endcode
*/
void *QAudioBuffer::data()
{
    if (!d)
        return nullptr;
    return d->data.data();
}

/*!
    \typedef QAudioBuffer::S16S

    This is a predefined specialization for a signed stereo 16 bit sample.  Each
    channel is a \e {signed short}.
*/

/*!
    \typedef QAudioBuffer::U8M

    This is a predefined specialization for an unsigned 8 bit mono sample.
*/
/*!
    \typedef QAudioBuffer::S16M
    This is a predefined specialization for a signed 16 bit mono sample.
i*/
/*!
    \typedef QAudioBuffer::S32M
    This is a predefined specialization for a signed 32 bit mono sample.
*/
/*!
    \typedef QAudioBuffer::F32M
    This is a predefined specialization for a 32 bit float mono sample.
*/
/*!
    \typedef QAudioBuffer::U8S
    This is a predifined specialization for an unsiged 8 bit stereo sample.
*/
/*!
    \typedef QAudioBuffer::S32S
    This is a predifined specialization for a siged 32 bit stereo sample.
*/
/*!
    \typedef QAudioBuffer::F32S
    This is a predifined specialization for a 32 bit float stereo sample.
*/
QT_END_NAMESPACE
