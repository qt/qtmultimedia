/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
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
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QDebug>
#include <qaudioformat.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAudioFormat
    \brief The QAudioFormat class stores audio stream parameter information.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio

    An audio format specifies how data in a raw audio stream is arranged,
    i.e, how the stream is to be interpreted.

    QAudioFormat contains parameters that specify how the audio sample data
    is arranged. These are the frequency, the number of channels, and the
    sameple format. The following table describes
    these in more detail.

    \table
        \header
            \li Parameter
            \li Description
        \row
            \li Sample Rate
            \li Samples per second of audio data in Hertz.
        \row
            \li Number of channels
            \li The number of audio channels (typically one for mono
               or two for stereo). These are the amount of consecutive
               samples that together form one frame in the stream
        \row
            \li Sample format
            \li The format of the audio samples in the stream
    \endtable

    This class is used in conjunction with QAudioInput or
    QAudioOutput to allow you to specify the parameters of the audio
    stream being read or written, or with QAudioBuffer when dealing with
    samples in memory.

    You can obtain audio formats compatible with the audio device used
    through functions in QAudioDeviceInfo. This class also lets you
    query available parameter values for a device, so that you can set
    the parameters yourself. See the \l QAudioDeviceInfo class
    description for details. You need to know the format of the audio
    streams you wish to play or record.

    Samples for all channels will be interleaved.
    One sample for each channel for the same instant in time is referred
    to as a frame in Qt Multimedia (and other places).
*/

/*!
    \fn QAudioFormat::QAudioFormat()

    Construct a new audio format.

    Values are initialized as follows:
    \list
    \li sampleRate()  = 0
    \li channelCount() = 0
    \li sampleFormat() = QAudioFormat::Unknown
    \endlist
*/

/*!
    \fn QAudioFormat::QAudioFormat(const QAudioFormat &other)

    Construct a new audio format using \a other.
*/

/*!
    \fn QAudioFormat::~QAudioFormat()

    Destroy this audio format.
*/

/*!
    \fn QAudioFormat& QAudioFormat::operator=(const QAudioFormat &other)

    Assigns \a other to this QAudioFormat implementation.
*/

/*!
    \fn bool QAudioFormat::isValid() const

    Returns true if all of the parameters are valid.
*/

/*!
    \fn void QAudioFormat::setSampleRate(int samplerate)

    Sets the sample rate to \a samplerate Hertz.
*/

/*!
    \fn int QAudioFormat::sampleRate() const

    Returns the current sample rate in Hertz.
*/

/*!
    \fn void QAudioFormat::setChannelCount(int channels)

    Sets the channel count to \a channels.

*/

/*!
    \fn int QAudioFormat::channelCount() const

    Returns the current channel count value.

*/

/*!
    \fn void QAudioFormat::setSampleFormat(SampleFormat format)

    Sets the sample format to \a format.

    \sa QAudioFormat::SampleFormat
*/

/*!
    \fn QAudioFormat::SampleFormat QAudioFormat::sampleFormat() const
    Returns the current sample format.

    \sa setSampleFormat()
*/

/*!
    Returns the number of bytes required for this audio format for \a duration microseconds.

    Returns 0 if this format is not valid.

    Note that some rounding may occur if \a duration is not an exact fraction of the
    sampleRate().

    \sa durationForBytes()
 */
qint32 QAudioFormat::bytesForDuration(qint64 duration) const
{
    return bytesPerFrame() * framesForDuration(duration);
}

/*!
    Returns the number of microseconds represented by \a bytes in this format.

    Returns 0 if this format is not valid.

    Note that some rounding may occur if \a bytes is not an exact multiple
    of the number of bytes per frame.

    \sa bytesForDuration()
*/
qint64 QAudioFormat::durationForBytes(qint32 bytes) const
{
    // avoid compiler warnings about unused variables. [[maybe_unused]] in the header
    // gives compiler errors on older gcc versions
    Q_UNUSED(bitfields);
    Q_UNUSED(reserved);

    if (!isValid() || bytes <= 0)
        return 0;

    // We round the byte count to ensure whole frames
    return qint64(1000000LL * (bytes / bytesPerFrame())) / sampleRate();
}

/*!
    Returns the number of bytes required for \a frameCount frames of this format.

    Returns 0 if this format is not valid.

    \sa bytesForDuration()
*/
qint32 QAudioFormat::bytesForFrames(qint32 frameCount) const
{
    return frameCount * bytesPerFrame();
}

/*!
    Returns the number of frames represented by \a byteCount in this format.

    Note that some rounding may occur if \a byteCount is not an exact multiple
    of the number of bytes per frame.

    Each frame has one sample per channel.

    \sa framesForDuration()
*/
qint32 QAudioFormat::framesForBytes(qint32 byteCount) const
{
    int size = bytesPerFrame();
    if (size > 0)
        return byteCount / size;
    return 0;
}

/*!
    Returns the number of frames required to represent \a duration microseconds in this format.

    Note that some rounding may occur if \a duration is not an exact fraction of the
    \l sampleRate().
*/
qint32 QAudioFormat::framesForDuration(qint64 duration) const
{
    if (!isValid())
        return 0;

    return qint32((duration * sampleRate()) / 1000000LL);
}

/*!
    Return the number of microseconds represented by \a frameCount frames in this format.
*/
qint64 QAudioFormat::durationForFrames(qint32 frameCount) const
{
    if (!isValid() || frameCount <= 0)
        return 0;

    return (frameCount * 1000000LL) / sampleRate();
}

/*!
    \fn int QAudioFormat::bytesPerFrame() const

    Returns the number of bytes required to represent one frame (a sample in each channel) in this format.

    Returns 0 if this format is invalid.
*/

/*!
    \fn int QAudioFormat::bytesPerSample() const
    Returns the number of bytes required to represent one sample in this format.

    Returns 0 if this format is invalid.
*/

/*!
    Normalizes the sample value to a number between -1 and 1.
*/
float QAudioFormat::normalizedSampleValue(const void *sample) const
{
    switch (m_sampleFormat) {
    case UInt8:
        return ((float)*reinterpret_cast<const quint8 *>(sample))/(float)std::numeric_limits<qint8>::max() - 1.;
    case Int16:
        return ((float)*reinterpret_cast<const qint16 *>(sample))/(float)std::numeric_limits<qint16>::max();
    case Int32:
        return ((float)*reinterpret_cast<const qint32 *>(sample))/(float)std::numeric_limits<qint32>::max();
    case Float:
        return *reinterpret_cast<const float *>(sample);
    case Unknown:
    case NSampleFormats:
        break;
    }

    return 0.;
}

/*!
    \enum QAudioFormat::SampleFormat

    Qt will always expect and use samples in the endianness of the host platform. When processing audio data
    from external sources yourself, ensure you convert them to the correct endianness before writing them to
    a QAudioOutput or QAudioBuffer

    \value Unknown       Not Set
    \value Int16         Samples are 16 bit signed integers
    \value Int32         Samples are 32 bit signed intergers
    \value Float         Samples are floats
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, QAudioFormat::SampleFormat type)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (type) {
    case QAudioFormat::UInt8:
        dbg << "UInt8";
        break;
    case QAudioFormat::Int16:
        dbg << "Int16";
        break;
    case QAudioFormat::Int32:
        dbg << "Int32";
        break;
    case QAudioFormat::Float:
        dbg << "Float";
        break;
    default:
        dbg << "Unknown";
        break;
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, const QAudioFormat &f)
{
    dbg << "QAudioFormat(" << f.sampleRate() << "Hz, " << f.channelCount() << "Channels, " << f.sampleFormat() << "Format";
    return dbg;
}
#endif



QT_END_NAMESPACE

