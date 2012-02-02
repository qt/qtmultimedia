/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaudiobuffer.h"
#include "qaudiobuffer_p.h"

#include <QObject>
#include <QDebug>

QT_BEGIN_NAMESPACE

namespace
{
    class QAudioBufferPrivateRegisterMetaTypes
    {
    public:
        QAudioBufferPrivateRegisterMetaTypes()
        {
            qRegisterMetaType<QAudioBuffer>();
        }
    } _registerMetaTypes;
}


class QAudioBufferPrivate : public QSharedData
{
public:
    QAudioBufferPrivate(QAbstractAudioBuffer *provider)
        : mProvider(provider)
        , mCount(1)
    {
    }

    ~QAudioBufferPrivate()
    {
        if (mProvider)
            mProvider->release();
    }

    void ref()
    {
        mCount.ref();
    }

    void deref()
    {
        if (!mCount.deref())
            delete this;
    }

    QAudioBufferPrivate *clone();

    static QAudioBufferPrivate *acquire(QAudioBufferPrivate *other)
    {
        if (!other)
            return 0;

        // Ref the other (if there are extant data() pointers, they will
        // also point here - it's a feature, not a bug, like QByteArray)
        other->ref();
        return other;
    }

    QAbstractAudioBuffer *mProvider;
    QAtomicInt mCount;
};

// Private class to go in .cpp file
class QMemoryAudioBufferProvider : public QAbstractAudioBuffer {
public:
    QMemoryAudioBufferProvider(const void *data, int sampleCount, const QAudioFormat &format, qint64 startTime)
        : mStartTime(startTime)
        , mSampleCount(sampleCount)
        , mFormat(format)
    {
        int numBytes = (sampleCount * format.channelCount() * format.sampleSize()) / 8;
        if (numBytes > 0) {
            mBuffer = malloc(numBytes);
            if (!mBuffer) {
                // OOM, if that's likely
                mStartTime = -1;
                mSampleCount = 0;
                mFormat = QAudioFormat();
            } else {
                // Allocated, see if we have data to copy
                if (data) {
                    memcpy(mBuffer, data, numBytes);
                } else {
                    // We have to fill with the zero value..
                    switch (format.sampleType()) {
                        case QAudioFormat::SignedInt:
                            // Signed int means 0x80, 0x8000 is zero
                            // XXX this is not right for > 8 bits(0x8080 vs 0x8000)
                            memset(mBuffer, 0x80, numBytes);
                            break;
                        default:
                            memset(mBuffer, 0x0, numBytes);
                    }
                }
            }
        } else
            mBuffer = 0;
    }

    ~QMemoryAudioBufferProvider()
    {
        if (mBuffer)
            free(mBuffer);
    }

    void release() {delete this;}
    QAudioFormat format() const {return mFormat;}
    qint64 startTime() const {return mStartTime;}
    int sampleCount() const {return mSampleCount;}

    void *constData() const {return mBuffer;}

    void *writableData() {return mBuffer;}
    QAbstractAudioBuffer *clone() const
    {
        return new QMemoryAudioBufferProvider(mBuffer, mSampleCount, mFormat, mStartTime);
    }

    void *mBuffer;
    qint64 mStartTime;
    int mSampleCount;
    QAudioFormat mFormat;
};

QAudioBufferPrivate *QAudioBufferPrivate::clone()
{
    // We want to create a single bufferprivate with a
    // single qaab
    // This should only be called when the count is > 1
    Q_ASSERT(mCount.load() > 1);

    if (mProvider) {
        QAbstractAudioBuffer *abuf = mProvider->clone();

        if (!abuf) {
            abuf = new QMemoryAudioBufferProvider(mProvider->constData(), mProvider->sampleCount(), mProvider->format(), mProvider->startTime());
        }

        if (abuf) {
            return new QAudioBufferPrivate(abuf);
        }
    }

    return 0;
}

/*!
    \class QAbstractAudioBuffer
    \internal
*/

/*!
    \class QAudioBuffer
    \brief A class that represents a collection of audio samples.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio

    The QAudioBuffer class represents a collection of audio samples,
    with a specific format and sample rate.
*/
// ^ Mostly useful with probe or decoder

/*!
    Create a new, empty, invalid buffer.
 */
QAudioBuffer::QAudioBuffer()
    : d(0)
{
}

/*!
    \internal
    Create a new audio buffer from the supplied \a provider.  This
    constructor is typically only used when handling certain hardware
    or media framework specific buffers, and generally isn't useful
    in application code.
 */
QAudioBuffer::QAudioBuffer(QAbstractAudioBuffer *provider)
    : d(new QAudioBufferPrivate(provider))
{
}
/*!
    Creates a new audio buffer from \a other.  Generally
    this will have copy-on-write semantics - a copy will
    only be made when it has to be.
 */
QAudioBuffer::QAudioBuffer(const QAudioBuffer &other)
{
    d = QAudioBufferPrivate::acquire(other.d);
}

/*!
    Creates a new audio buffer from the supplied \a data, in the
    given \a format.  The format will determine how the number
    and sizes of the samples are interpreted from the \a data.

    If the supplied \a data is not an integer multiple of the
    calculated sample size, the excess data will not be used.

    This audio buffer will copy the contents of \a data.
 */
QAudioBuffer::QAudioBuffer(const QByteArray &data, const QAudioFormat &format)
{
    int sampleSize = (format.sampleSize() * format.channelCount()) / 8;
    int sampleCount = data.size() / sampleSize; // truncate
    d = new QAudioBufferPrivate(new QMemoryAudioBufferProvider(data.constData(), sampleCount, format, -1));
}

/*!
    Creates a new audio buffer with space for \a numSamples samples of
    the given \a format.  The samples will be initialized to the default
    for the format.
 */
QAudioBuffer::QAudioBuffer(int numSamples, const QAudioFormat &format)
    : d(new QAudioBufferPrivate(new QMemoryAudioBufferProvider(0, numSamples, format, -1)))
{
}

/*!
    Assigns the \a other buffer to this.
 */
QAudioBuffer &QAudioBuffer::operator =(const QAudioBuffer &other)
{
    if (this->d != other.d) {
        d = QAudioBufferPrivate::acquire(other.d);
    }
    return *this;
}

/*!
    Destroys this audio buffer.
 */
QAudioBuffer::~QAudioBuffer()
{
    if (d)
        d->deref();
}

/*!
    Returns true if this is a valid buffer.  A valid buffer
    has more than zero samples in it and a valid format.
 */
bool QAudioBuffer::isValid() const
{
    if (!d || !d->mProvider)
        return false;
    return d->mProvider->format().isValid() && (d->mProvider->sampleCount() > 0);
}

/*!
    Returns the \l {QAudioFormat}{format} of this buffer.

    Several properties of this format influence how
    the \l duration() or \l byteCount() are calculated
    from the \l sampleCount().
 */
QAudioFormat QAudioBuffer::format() const
{
    if (!isValid())
        return QAudioFormat();
    return d->mProvider->format();
}

/*!
    Returns the number of samples in this buffer.

    If the format of this buffer has multiple channels,
    then this count includes all channels.  This means
    that a stereo buffer with 1000 samples in total will
    have 500 left samples and 500 right samples (interleaved),
    and this function will return 1000.
 */
int QAudioBuffer::sampleCount() const
{
    if (!isValid())
        return 0;
    return d->mProvider->sampleCount();
}

/*!
    Returns the size of this buffer, in bytes.
 */
int QAudioBuffer::byteCount() const
{
    const QAudioFormat f(format());
    return (f.channelCount() * f.sampleSize() * sampleCount()) / 8; // sampleSize is in bits
}

/*!
    Returns the duration of audio in this buffer, in microseconds.

    This depends on the /l format(), and the \l sampleCount().
*/
qint64 QAudioBuffer::duration() const
{
    int divisor = format().sampleRate() * format().channelCount();
    if (divisor > 0)
        return (sampleCount() * 1000000LL) / divisor;
    else
        return 0;
}

/*!
    Returns the time in a stream that this buffer starts at (in microseconds).

    If this buffer is not part of a stream, this will return -1.
 */
qint64 QAudioBuffer::startTime() const
{
    if (!isValid())
        return -1;
    return d->mProvider->startTime();
}

/*!
    Returns a pointer to this buffer's data.  You can only read it.

    This method is preferred over the const version of \l data() to
    prevent unnecessary copying.
 */
const void* QAudioBuffer::constData() const
{
    if (!isValid())
        return 0;
    return d->mProvider->constData();
}

/*!
    Returns a pointer to this buffer's data.  You can only read it.

    You should use the \l constData() function rather than this
    to prevent accidental deep copying.
 */
const void* QAudioBuffer::data() const
{
    if (!isValid())
        return 0;
    return d->mProvider->constData();
}

/*!
    Returns a pointer to this buffer's data.  You can modify the
    data through the returned pointer.

    Since QAudioBuffers can share the actual sample data, calling
    this function will result in a deep copy being made if there
    are any other buffers using the sample.  You should avoid calling
    this unless you really need to modify the data.

    This pointer will remain valid until the underlying storage is
    detached.  In particular, if you obtain a pointer, and then
    copy this audio buffer, changing data through this pointer may
    change both buffer instances.  Calling \l data() on either instance
    will again cause a deep copy to be made, which may invalidate
    the pointers returned from this function previously.
 */
void *QAudioBuffer::data()
{
    if (!isValid())
        return 0;

    if (d->mCount.load() != 1) {
        // Can't share a writable buffer
        // so we need to detach
        QAudioBufferPrivate *newd = d->clone();

        // This shouldn't happen
        if (!newd)
            return 0;

        d->deref();
        d = newd;
    }

    // We're (now) the only user of this qaab, so
    // see if it's writable directly
    void *buffer = d->mProvider->writableData();
    if (buffer) {
        return buffer;
    }

    // Wasn't writable, so turn it into a memory provider
    QAbstractAudioBuffer *memBuffer = new QMemoryAudioBufferProvider(constData(), sampleCount(), format(), startTime());

    if (memBuffer) {
        d->mProvider->release();
        d->mCount.store(1);
        d->mProvider = memBuffer;

        return memBuffer->writableData();
    }

    return 0;
}

QT_END_NAMESPACE
