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

#include "qaudiosystem_p.h"
#include "qaudiodeviceinfo_p.h"
#include <private/qplatformmediadevices_p.h>
#include <private/qplatformmediaintegration_p.h>

#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

QAudioDeviceInfoPrivate::~QAudioDeviceInfoPrivate() = default;

/*!
    \class QAudioDeviceInfo
    \brief The QAudioDeviceInfo class provides an interface to query audio devices and their functionality.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio

    QAudioDeviceInfo lets you query for audio devices--such as sound
    cards and USB headsets--that are currently available on the system.
    The audio devices available are dependent on the platform or audio plugins installed.

    A QAudioDeviceInfo is used by Qt to construct
    classes that communicate with the device--such as
    QAudioInput, and QAudioOutput.

    You can also query each device for the formats it supports. A
    format in this context is a set consisting of a specific byte
    order, channel, codec, frequency, sample rate, and sample type.  A
    format is represented by the QAudioFormat class.

    The values supported by the device for each of these
    parameters can be fetched with
    supportedByteOrders(), minimumChannelCount(), maximumChannelCount(), supportedCodecs(),
    minimumSampleRate(), maximumSampleRate(), supportedSampleSizes(), and
    supportedSampleTypes(). The combinations supported are dependent on the platform,
    audio plugins installed and the audio device capabilities. If you need a
    specific format, you can check if
    the device supports it with isFormatSupported(), or fetch a
    supported format that is as close as possible to the format with
    nearestFormat(). For instance:

    \snippet multimedia-snippets/audio.cpp Setting audio format

    The static
    functions defaultInputDevice(), defaultOutputDevice(), and
    availableDevices() let you get a list of all available
    devices. Devices are fetched according to the value of mode
    this is specified by the \l {QAudio}::Mode enum.
    The QAudioDeviceInfo returned are only valid for the \l {QAudio}::Mode.

    For instance:

    \snippet multimedia-snippets/audio.cpp Dumping audio formats

    In this code sample, we loop through all devices that are able to output
    sound, i.e., play an audio stream in a supported format. For each device we
    find, we simply print the deviceName().

    \sa QAudioOutput, QAudioInput
*/

/*!
    Constructs an empty QAudioDeviceInfo object.
*/
QAudioDeviceInfo::QAudioDeviceInfo():
    d(nullptr)
{
}

/*!
    Constructs a copy of \a other.
*/
QAudioDeviceInfo::QAudioDeviceInfo(const QAudioDeviceInfo& other) = default;

/*!
    Destroy this audio device info.
*/
QAudioDeviceInfo::~QAudioDeviceInfo() = default;

/*!
    Sets the QAudioDeviceInfo object to be equal to \a other.
*/
QAudioDeviceInfo& QAudioDeviceInfo::operator=(const QAudioDeviceInfo &other) = default;

/*!
    Returns true if this QAudioDeviceInfo class represents the
    same audio device as \a other.
*/
bool QAudioDeviceInfo::operator ==(const QAudioDeviceInfo &other) const
{
    if (d == other.d)
        return true;
    if (!d || !other.d)
        return false;
    if (d->mode == other.d->mode && d->id == other.d->id)
        return true;
    return false;
}

/*!
    Returns true if this QAudioDeviceInfo class represents a
    different audio device than \a other
*/
bool QAudioDeviceInfo::operator !=(const QAudioDeviceInfo &other) const
{
    return !operator==(other);
}

/*!
    Returns whether this QAudioDeviceInfo object holds a valid device definition.
*/
bool QAudioDeviceInfo::isNull() const
{
    return d == nullptr;
}

/*!
    Returns an identifier for the audio device.

    Device names vary depending on the platform/audio plugin being used.

    They are a unique string identifier for the audio device.

    eg. default, Intel, U0x46d0x9a4
*/
QByteArray QAudioDeviceInfo::id() const
{
    return isNull() ? QByteArray() : d->id;
}

/*!
    Returns a human readable name of the audio device.

    Use this string to present the device to the user.
*/
QString QAudioDeviceInfo::description() const
{
    return isNull() ? QString() : d->description;
}

/*!
    Returns true if this is the default audio device for it's mode.
*/
bool QAudioDeviceInfo::isDefault() const
{
    return d ? d->isDefault : false;
}

/*!
    Returns true if the supplied \a settings are supported by the audio
    device described by this QAudioDeviceInfo.
*/
bool QAudioDeviceInfo::isFormatSupported(const QAudioFormat &settings) const
{
    if (isNull())
        return false;
    if (settings.sampleRate() < d->minimumSampleRate || settings.sampleRate() > d->maximumSampleRate)
        return false;
    if (settings.channelCount() < d->minimumChannelCount || settings.channelCount() > d->maximumChannelCount)
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
    \li Input settings: 8000Hz mono 8 bit.
    \li Output settings: 44100Hz stereo 16 bit little endian.
    \endlist
*/
QAudioFormat QAudioDeviceInfo::preferredFormat() const
{
    return isNull() ? QAudioFormat() : d->preferredFormat;
}

/*!
    Returns the minimum supported sample rate (in Hertz).
*/
int QAudioDeviceInfo::minimumSampleRate() const
{
    return isNull() ? 0 : d->minimumSampleRate;
}

/*!
    Returns the maximum supported sample rate (in Hertz).
*/
int QAudioDeviceInfo::maximumSampleRate() const
{
    return isNull() ? 0 : d->maximumSampleRate;
}

/*!
    Returns the minimum number of supported channel counts.

    This is typically 1 for mono sound, or 2 for stereo sound.
*/
int QAudioDeviceInfo::minimumChannelCount() const
{
    return isNull() ? 0 : d->minimumChannelCount;
}

/*!
    Returns the maximum number of supported channel counts.

    This is typically 1 for mono sound, or 2 for stereo sound.
*/
int QAudioDeviceInfo::maximumChannelCount() const
{
    return isNull() ? 0 : d->maximumChannelCount;
}

/*!
    Returns a list of supported sample types.
*/
QList<QAudioFormat::SampleFormat> QAudioDeviceInfo::supportedSampleFormats() const
{
    return isNull() ? QList<QAudioFormat::SampleFormat>() : d->supportedSampleFormats;
}

QAudioDeviceInfo::QAudioDeviceInfo(QAudioDeviceInfoPrivate *p)
    : d(p)
{}

/*!
    \internal
*/
QAudio::Mode QAudioDeviceInfo::mode() const
{
    return d->mode;
}

QT_END_NAMESPACE
