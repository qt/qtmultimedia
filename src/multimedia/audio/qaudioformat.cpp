// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudioformat.h"

#include <QtCore/qdebug.h>
#include <QtMultimedia/private/qmultimedia_assume_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAudioFormat
    \brief The QAudioFormat class stores audio stream parameter information.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio

    An audio format specifies how data in a raw audio stream is arranged. For
    example, how the stream is to be interpreted.

    QAudioFormat contains parameters that specify how the audio sample data
    is arranged. These are the frequency, the number of channels, and the
    sample format. The following table describes these in more detail.

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

    This class is used in conjunction with QAudioSource or
    QAudioSink to allow you to specify the parameters of the audio
    stream being read or written, or with QAudioBuffer when dealing with
    samples in memory.

    You can obtain audio formats compatible with the audio device used
    through functions in QAudioDevice. This class also lets you
    query available parameter values for a device, so that you can set
    the parameters yourself. See the \l QAudioDevice class
    description for details. You need to know the format of the audio
    streams you wish to play or record.

    Samples for all channels will be interleaved.
    One sample for each channel for the same instant in time is referred
    to as a frame in Qt Multimedia (and other places).
*/

/*!
    \fn QAudioFormat::QAudioFormat()

    Constructs a new audio format.

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
    \fn bool QAudioFormat::operator==(const QAudioFormat &a, const QAudioFormat &b)

    Returns \c true if audio format \a a is equal to \a b, otherwise returns \c false.
*/

/*!
    \fn bool QAudioFormat::operator!=(const QAudioFormat &a, const QAudioFormat &b)

    Returns \c true if audio format \a a is not equal to \a b, otherwise returns \c false.
*/

/*!
    \fn bool QAudioFormat::isValid() const

    Returns \c true if all of the parameters are valid.
*/

/*!
    \fn void QAudioFormat::setSampleRate(int samplerate)

    Sets the sample rate to \a samplerate in Hertz.
*/

/*!
    \fn int QAudioFormat::sampleRate() const

    Returns the current sample rate in Hertz.
*/

/*!
    \enum QAudioFormat::AudioChannelPosition

    Describes the possible audio channel positions. These follow the standard
    definition used in the 22.2 surround sound configuration.

    \value UnknownPosition Unknown position
    \value FrontLeft
    \value FrontRight
    \value FrontCenter
    \value LFE Low Frequency Effect channel (Subwoofer)
    \value BackLeft
    \value BackRight
    \value FrontLeftOfCenter
    \value FrontRightOfCenter
    \value BackCenter
    \value LFE2
    \value SideLeft
    \value SideRight
    \value TopFrontLeft
    \value TopFrontRight
    \value TopFrontCenter
    \value TopCenter
    \value TopBackLeft
    \value TopBackRight
    \value TopSideLeft
    \value TopSideRight
    \value TopBackCenter
    \value BottomFrontCenter
    \value BottomFrontLeft
    \value BottomFrontRight
*/
/*!
    \variable QAudioFormat::NChannelPositions
    \internal
*/

/*!
    \enum QAudioFormat::ChannelConfig

    This enum describes a standardized audio channel layout. The most common
    configurations are Mono, Stereo, 2.1 (stereo plus low frequency), 5.1 surround,
    and 7.1 surround configurations.

    \value ChannelConfigUnknown The channel configuration is not known.
    \value ChannelConfigMono The audio has one Center channel.
    \value ChannelConfigStereo The audio has two channels, Left and Right.
    \value ChannelConfig2Dot1 The audio has three channels, Left, Right and
           LFE (low frequency effect).
    \value ChannelConfig3Dot0 The audio has three channels, Left, Right, and
           Center.
    \value ChannelConfig3Dot1 The audio has four channels, Left, Right, Center,
           and LFE (low frequency effect).
    \value ChannelConfigSurround5Dot0 The audio has five channels, Left, Right,
           Center, BackLeft, and BackRight.
    \value ChannelConfigSurround5Dot1 The audio has 6 channels, Left, Right,
           Center, LFE, BackLeft, and BackRight.
    \value ChannelConfigSurround7Dot0 The audio has 7 channels, Left, Right,
           Center, BackLeft, BackRight, SideLeft, and SideRight.
    \value ChannelConfigSurround7Dot1 The audio has 8 channels, Left, Right,
           Center, LFE, BackLeft, BackRight, SideLeft, and SideRight.
*/

/*!
    Sets the channel configuration to \a config.

    Sets the channel configuration of the audio format to one of the standard
    audio channel configurations.

    \note that this will also modify the channel count.
*/
void QAudioFormat::setChannelConfig(ChannelConfig config) noexcept
{
    m_channelConfig = config;
    if (config != ChannelConfigUnknown)
        m_channelCount = qPopulationCount(config);
}

/*!
    Returns the position of a certain audio \a channel inside an audio frame
    for the given format.
    Returns -1 if the channel does not exist for this format or the channel
    configuration is unknown.
*/
int QAudioFormat::channelOffset(AudioChannelPosition channel) const noexcept
{
    if (!(m_channelConfig & (1u << channel)))
        return -1;

    uint maskedChannels = m_channelConfig & ((1u << channel) - 1);
    return qPopulationCount(maskedChannels);
}

/*!
    \fn void QAudioFormat::setChannelCount(int channels)

    Sets the channel count to \a channels. Setting this also sets the channel
    config to ChannelConfigUnknown.
*/

/*!
    \fn template <typename... Args> QAudioFormat::ChannelConfig QAudioFormat::channelConfig(Args... channels)

    Returns the current channel configuration for the given \a channels.
*/
/*!
    \fn QAudioFormat::ChannelConfig QAudioFormat::channelConfig() const noexcept

    Returns the current channel configuration.
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
    Returns the number of bytes required for this audio format for \a microseconds.

    Returns 0 if this format is not valid.

    Note that some rounding may occur if \a microseconds is not an exact fraction of the
    sampleRate().

    \sa durationForBytes()
 */
qint32 QAudioFormat::bytesForDuration(qint64 microseconds) const
{
    return bytesPerFrame() * framesForDuration(microseconds);
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
    if (!isValid() || bytes <= 0)
        return 0;

    // We round the byte count to ensure whole frames

    const int bytesPerFrame = this->bytesPerFrame();
    const int sampleRate = this->sampleRate();
    QT_MM_ASSUME(bytesPerFrame > 0);
    QT_MM_ASSUME(sampleRate > 0);
    return qint64(1000000LL * (bytes / bytesPerFrame)) / sampleRate;
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

    // bitshifting to avoid integer division
    switch (size) {
    case 0:
        return 0;
    case 1:
        return byteCount;
    case 2:
        return byteCount / 2;
    case 4:
        return byteCount / 4;
    case 8:
        return byteCount / 8;
    case 16:
        return byteCount / 16;
    default:
        return byteCount / size;
    }
}

/*!
    Returns the number of frames required to represent \a microseconds in this format.

    Note that some rounding may occur if \a microseconds is not an exact fraction of the
    \l sampleRate().
*/
qint32 QAudioFormat::framesForDuration(qint64 microseconds) const
{
    if (!isValid())
        return 0;

    return qint32((microseconds * sampleRate()) / 1000000LL);
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

    Returns the number of bytes required to represent one frame
    (a sample in each channel) in this format.

    Returns 0 if this format is invalid.
*/

/*!
    \fn int QAudioFormat::bytesPerSample() const
    Returns the number of bytes required to represent one sample in this format.

    Returns 0 if this format is invalid.
*/

/*!
    Normalizes the \a sample value to a number between -1 and 1.
    The method depends on the QaudioFormat.
*/
float QAudioFormat::normalizedSampleValue(const void *sample) const
{
    switch (m_sampleFormat) {
    case UInt8:
        return ((float)*reinterpret_cast<const quint8 *>(sample))
                / (float)std::numeric_limits<qint8>::max()
                - 1.;
    case Int16:
        return ((float)*reinterpret_cast<const qint16 *>(sample))
                / (float)std::numeric_limits<qint16>::max();
    case Int32:
        return ((float)*reinterpret_cast<const qint32 *>(sample))
                / (float)std::numeric_limits<qint32>::max();
    case Float:
        return *reinterpret_cast<const float *>(sample);
    case Unknown:
    case NSampleFormats:
        break;
    }

    return 0.;
}

/*!
    Returns a default channel configuration for \a channelCount.

    Default configurations are defined for up to 8 channels, and correspond to
    standard Mono, Stereo and Surround configurations. For higher channel counts,
    this simply uses the first \a channelCount audio channels defined in
    \l QAudioFormat::AudioChannelPosition.
*/
QAudioFormat::ChannelConfig QAudioFormat::defaultChannelConfigForChannelCount(int channelCount)
{
    QAudioFormat::ChannelConfig config;
    switch (channelCount) {
    case 0:
        config = QAudioFormat::ChannelConfigUnknown;
        break;
    case 1:
        config = QAudioFormat::ChannelConfigMono;
        break;
    case 2:
        config = QAudioFormat::ChannelConfigStereo;
        break;
    case 3:
        config = QAudioFormat::ChannelConfig2Dot1;
        break;
    case 4:
        config = QAudioFormat::channelConfig(QAudioFormat::FrontLeft, QAudioFormat::FrontRight,
                                             QAudioFormat::BackLeft, QAudioFormat::BackRight);
        break;
    case 5:
        config = QAudioFormat::ChannelConfigSurround5Dot0;
        break;
    case 6:
        config = QAudioFormat::ChannelConfigSurround5Dot1;
        break;
    case 7:
        config = QAudioFormat::ChannelConfigSurround7Dot0;
        break;
    case 8:
        config = QAudioFormat::ChannelConfigSurround7Dot1;
        break;
    default:
        // give up, simply use the first n channels
        config = QAudioFormat::ChannelConfig(((1 << channelCount) - 1) << 1);
    }
    return config;
}

/*!
    \enum QAudioFormat::SampleFormat

    Qt will always expect and use samples in the endianness of the host platform.
    When processing audio data from external sources yourself, ensure you convert
    them to the correct endianness before writing them to a QAudioSink or
    QAudioBuffer.

    \value Unknown        Not Set
    \value UInt8          Samples are 8 bit unsigned integers
    \value Int16          Samples are 16 bit signed integers
    \value Int32          Samples are 32 bit signed integers
    \value Float          Samples are floats
    \omitvalue NSampleFormats
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
    QDebugStateSaver s(dbg);
    dbg.nospace();
    dbg << "QAudioFormat(" << f.sampleRate() << "Hz, " << f.channelCount() << " Channels, "
        << f.sampleFormat() << "Format)";
    return dbg;
}
#endif

QT_END_NAMESPACE
