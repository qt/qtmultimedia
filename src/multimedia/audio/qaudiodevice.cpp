// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiosystem_p.h"
#include "qaudiodevice_p.h"
#include <private/qplatformmediadevices_p.h>
#include <private/qplatformmediaintegration_p.h>

#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

QAudioDevicePrivate::~QAudioDevicePrivate() = default;

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QAudioDevicePrivate);

/*!
    \class QAudioDevice
    \brief The QAudioDevice class provides an information about audio devices and their
           functionality.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio

    QAudioDevice describes an audio device available in the system, either for input or for
    playback.

    A QAudioDevice is used by Qt to construct
    classes that communicate with the device -- such as
    QAudioSource, and QAudioSink. It is also used to determine the
    input or output device to use in a capture session or during media playback.

    You can also query each device for the formats it supports. A
    format in this context is a set consisting of a channel count, sample rate, and sample type. A
    format is represented by the QAudioFormat class.

    The values supported by the device for each of these parameters can be
    fetched with minimumChannelCount(), maximumChannelCount(),
    minimumSampleRate(), maximumSampleRate() and supportedSampleFormats(). The
    combinations supported are dependent on the audio device capabilities. If
    you need a specific format, you can check if the device supports it with
    isFormatSupported(). For instance:

    \snippet multimedia-snippets/audio.cpp Audio output setup

    The set of available devices can be retrieved from the QMediaDevices class.

    For instance:

    \snippet multimedia-snippets/audio.cpp Dumping audio formats

    In this code sample, we loop through all devices that are able to output
    sound, i.e., play an audio stream in a supported format. For each device we
    find, we simply print the deviceName().

    \sa QAudioSink, QAudioSource, QAudioFormat
*/

/*!
    \qmlvaluetype audioDevice
    \inqmlmodule QtMultimedia
    \since 6.2
    //! \instantiates QAudioDevice
    \brief Describes an audio device.
    \ingroup multimedia_qml
    \ingroup multimedia_audio_qml
    \ingroup qmlvaluetypes

    The audioDevice value type describes the properties of an audio device that
    is connected to the system.

    The list of audio input or output devices can be queried from the \l{MediaDevices}
    type. To select a certain audio device for input or output set it as the device
    on \l{AudioInput} or \l{AudioOutput}.

    \qml
    MediaPlayer {
        audioOutput: AudioOutput {
            device: mediaDevices.defaultAudioOutput
        }
    }
    MediaDevices {
        id: mediaDevices
    }
    \endqml
*/

/*!
    Constructs a null QAudioDevice object.
*/
QAudioDevice::QAudioDevice() = default;

/*!
    Constructs a copy of \a other.
*/
QAudioDevice::QAudioDevice(const QAudioDevice &other) = default;

/*!
    \fn QAudioDevice::QAudioDevice(QAudioDevice &&other)

    Move constructs from \a other.
*/
/*!
    \fn void QAudioDevice::swap(QAudioDevice &other) noexcept

    Swaps the audio device with the \a other.
*/
/*!
    Destroy this audio device info.
*/
QAudioDevice::~QAudioDevice() = default;

/*!
    Sets the QAudioDevice object to be equal to \a other.
*/
QAudioDevice &QAudioDevice::operator=(const QAudioDevice &other) = default;

/*!
    \fn QAudioDevice& QAudioDevice::operator=(QAudioDevice &&other)

    Moves \a other into this QAudioDevice object.
*/

/*!
    Returns true if this QAudioDevice class represents the
    same audio device as \a other.
*/
bool QAudioDevice::operator==(const QAudioDevice &other) const
{
    if (d == other.d)
        return true;
    if (!d || !other.d)
        return false;
    if (d->mode == other.d->mode && d->id == other.d->id && d->isDefault == other.d->isDefault)
        return true;
    return false;
}

/*!
    Returns true if this QAudioDevice class represents a
    different audio device than \a other
*/
bool QAudioDevice::operator!=(const QAudioDevice &other) const
{
    return !operator==(other);
}

/*!
    Returns whether this QAudioDevice object holds a valid device definition.
*/
bool QAudioDevice::isNull() const
{
    return d == nullptr;
}

/*!
    \qmlproperty string QtMultimedia::audioDevice::id

    Holds an identifier for the audio device.

    Device names vary depending on the platform/audio plugin being used.

    They are a unique identifier for the audio device.
*/

/*!
    \property QAudioDevice::id

    Returns an identifier for the audio device.

    Device names vary depending on the platform/audio plugin being used.

    They are a unique identifier for the audio device.
*/
QByteArray QAudioDevice::id() const
{
    return isNull() ? QByteArray() : d->id;
}

/*!
    \qmlproperty string QtMultimedia::audioDevice::description

    Holds a human readable name of the audio device.

    Use this string to present the device to the user.
*/

/*!
    \property QAudioDevice::description

    Returns a human readable name of the audio device.

    Use this string to present the device to the user.
*/
QString QAudioDevice::description() const
{
    return isNull() ? QString() : d->description;
}

/*!
    \qmlproperty bool QtMultimedia::audioDevice::isDefault

    Is true if this is the default audio device.
*/

/*!
    \property QAudioDevice::isDefault

    Returns true if this is the default audio device.
*/
bool QAudioDevice::isDefault() const
{
    return d ? d->isDefault : false;
}

/*!
    Returns true if the supplied \a settings are supported by the audio
    device described by this QAudioDevice.
*/
bool QAudioDevice::isFormatSupported(const QAudioFormat &settings) const
{
    if (isNull())
        return false;
    if (settings.sampleRate() < d->minimumSampleRate
        || settings.sampleRate() > d->maximumSampleRate)
        return false;
    if (settings.channelCount() < d->minimumChannelCount
        || settings.channelCount() > d->maximumChannelCount)
        return false;
    if (!d->supportedSampleFormats.contains(settings.sampleFormat()))
        return false;
    return true;
}

/*!
    Returns the default audio format settings for this device.

    These settings are provided by the platform/audio plugin being used.

    They are also dependent on the \l {QAudio}::Mode being used.

    A typical audio system would provide something like:
    \list
    \li Input settings: 48000Hz mono 16 bit.
    \li Output settings: 48000Hz stereo 16 bit.
    \endlist
*/
QAudioFormat QAudioDevice::preferredFormat() const
{
    return isNull() ? QAudioFormat() : d->preferredFormat;
}

/*!
    Returns the minimum supported sample rate (in Hertz).
*/
int QAudioDevice::minimumSampleRate() const
{
    return isNull() ? 0 : d->minimumSampleRate;
}

/*!
    Returns the maximum supported sample rate (in Hertz).
*/
int QAudioDevice::maximumSampleRate() const
{
    return isNull() ? 0 : d->maximumSampleRate;
}

/*!
    Returns the minimum number of supported channel counts.

    This is typically 1 for mono sound, or 2 for stereo sound.
*/
int QAudioDevice::minimumChannelCount() const
{
    return isNull() ? 0 : d->minimumChannelCount;
}

/*!
    Returns the maximum number of supported channel counts.

    This is typically 1 for mono sound, or 2 for stereo sound.
*/
int QAudioDevice::maximumChannelCount() const
{
    return isNull() ? 0 : d->maximumChannelCount;
}

/*!
    Returns a list of supported sample types.
*/
QList<QAudioFormat::SampleFormat> QAudioDevice::supportedSampleFormats() const
{
    return isNull() ? QList<QAudioFormat::SampleFormat>() : d->supportedSampleFormats;
}

/*!
    Returns the channel configuration of the device.
*/
QAudioFormat::ChannelConfig QAudioDevice::channelConfiguration() const
{
    return isNull() ? QAudioFormat::ChannelConfigUnknown : d->channelConfiguration;
}

/*!
    \fn QAudioDevicePrivate QAudioDevice::handle() const
    \internal
*/
/*!
    \internal
*/
QAudioDevice::QAudioDevice(QAudioDevicePrivate *p) : d(p) { }

/*!
    \enum QAudioDevice::Mode

    Describes the mode of this device.

    \value Null
         A null device.
    \value Input
         An input device.
    \value Output
        An output device.
*/

/*!
    \qmlproperty enumeration QtMultimedia::audioDevice::mode

   Holds whether this device is an input or output device.

    The returned value can be one of the following:


    \value audioDevice.Null A null device.
    \value audioDevice.Input input device.
    \value audioDevice.Output An output device.

*/

/*!
    \property QAudioDevice::mode

    Returns whether this device is an input or output device.
*/
QAudioDevice::Mode QAudioDevice::mode() const
{
    return d ? d->mode : Null;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, QAudioDevice::Mode mode)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (mode) {
    case QAudioDevice::Input:
        dbg << "QAudioDevice::Input";
        break;
    case QAudioDevice::Output:
        dbg << "QAudioDevice::Output";
        break;
    case QAudioDevice::Null:
        dbg << "QAudioDevice::Null";
        break;
    }
    return dbg;
}
#endif

QT_END_NAMESPACE

#include "moc_qaudiodevice.cpp"
