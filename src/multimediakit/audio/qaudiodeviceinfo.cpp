/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaudiodevicefactory_p.h"
#include "qaudiosystem.h"
#include "qaudiodeviceinfo.h"

#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

class QAudioDeviceInfoPrivate : public QSharedData
{
public:
    QAudioDeviceInfoPrivate():info(0) {}
    QAudioDeviceInfoPrivate(const QString &r, const QByteArray &h, QAudio::Mode m):
        realm(r), handle(h), mode(m)
    {
        if (!handle.isEmpty())
            info = QAudioDeviceFactory::audioDeviceInfo(realm, handle, mode);
        else
            info = NULL;
    }

    QAudioDeviceInfoPrivate(const QAudioDeviceInfoPrivate &other):
        QSharedData(other),
        realm(other.realm), handle(other.handle), mode(other.mode)
    {
        info = QAudioDeviceFactory::audioDeviceInfo(realm, handle, mode);
    }

    QAudioDeviceInfoPrivate& operator=(const QAudioDeviceInfoPrivate &other)
    {
        delete info;

        realm = other.realm;
        handle = other.handle;
        mode = other.mode;
        info = QAudioDeviceFactory::audioDeviceInfo(realm, handle, mode);
        return *this;
    }

    ~QAudioDeviceInfoPrivate()
    {
        delete info;
    }

    QString     realm;
    QByteArray  handle;
    QAudio::Mode mode;
    QAbstractAudioDeviceInfo*   info;
};


/*!
    \class QAudioDeviceInfo
    \brief The QAudioDeviceInfo class provides an interface to query audio devices and their functionality.
    \inmodule QtMultimediaKit
    \ingroup multimedia

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

    The values supported by the the device for each of these
    parameters can be fetched with
    supportedByteOrders(), supportedChannelCounts(), supportedCodecs(),
    supportedSampleRates(), supportedSampleSizes(), and
    supportedSampleTypes(). The combinations supported are dependent on the platform,
    audio plugins installed and the audio device capabilities. If you need a
    specific format, you can check if
    the device supports it with isFormatSupported(), or fetch a
    supported format that is as close as possible to the format with
    nearestFormat(). For instance:

    \snippet doc/src/snippets/multimedia-snippets/audio.cpp Setting audio format

    The static
    functions defaultInputDevice(), defaultOutputDevice(), and
    availableDevices() let you get a list of all available
    devices. Devices are fetched according to the value of mode
    this is specified by the \l {QAudio}::Mode enum.
    The QAudioDeviceInfo returned are only valid for the \l {QAudio}::Mode.

    For instance:

    \snippet doc/src/snippets/multimedia-snippets/audio.cpp Dumping audio formats

    In this code sample, we loop through all devices that are able to output
    sound, i.e., play an audio stream in a supported format. For each device we
    find, we simply print the deviceName().

    \sa QAudioOutput, QAudioInput
*/

/*!
    Constructs an empty QAudioDeviceInfo object.
*/
QAudioDeviceInfo::QAudioDeviceInfo():
    d(new QAudioDeviceInfoPrivate)
{
}

/*!
    Constructs a copy of \a other.
    \since 1.0
*/
QAudioDeviceInfo::QAudioDeviceInfo(const QAudioDeviceInfo& other):
    d(other.d)
{
}

/*!
    Destroy this audio device info.
*/
QAudioDeviceInfo::~QAudioDeviceInfo()
{
}

/*!
    Sets the QAudioDeviceInfo object to be equal to \a other.
    \since 1.0
*/
QAudioDeviceInfo& QAudioDeviceInfo::operator=(const QAudioDeviceInfo &other)
{
    d = other.d;
    return *this;
}

/*!
    Returns whether this QAudioDeviceInfo object holds a device definition.
    \since 1.0
*/
bool QAudioDeviceInfo::isNull() const
{
    return d->info == 0;
}

/*!
    Returns the human readable name of the audio device.

    Device names vary depending on the platform/audio plugin being used.

    XXX

    They are a unique string identifier for the audio device.

    eg. default, Intel, U0x46d0x9a4
    \since 1.0
*/
QString QAudioDeviceInfo::deviceName() const
{
    return isNull() ? QString() : d->info->deviceName();
}

/*!
    Returns true if the supplied \a settings are supported by the audio
    device described by this QAudioDeviceInfo.
    \since 1.0
*/
bool QAudioDeviceInfo::isFormatSupported(const QAudioFormat &settings) const
{
    return isNull() ? false : d->info->isFormatSupported(settings);
}

/*!
    Returns the default audio format settings for this device.

    These settings are provided by the platform/audio plugin being used.

    They are also dependent on the \l {QAudio}::Mode being used.

    A typical audio system would provide something like:
    \list
    \o Input settings: 8000Hz mono 8 bit.
    \o Output settings: 44100Hz stereo 16 bit little endian.
    \endlist
    \since 1.0
*/
QAudioFormat QAudioDeviceInfo::preferredFormat() const
{
    return isNull() ? QAudioFormat() : d->info->preferredFormat();
}

/*!
    Returns the closest QAudioFormat to the supplied \a settings that the system supports.

    These settings are provided by the platform/audio plugin being used.

    They are also dependent on the \l {QAudio}::Mode being used.
    \since 1.0
*/
QAudioFormat QAudioDeviceInfo::nearestFormat(const QAudioFormat &settings) const
{
    if (isFormatSupported(settings))
        return settings;

    QAudioFormat nearest = settings;

    QList<QString> testCodecs = supportedCodecs();
    QList<int> testChannels = supportedChannels();
    QList<QAudioFormat::Endian> testByteOrders = supportedByteOrders();
    QList<QAudioFormat::SampleType> testSampleTypes;
    QList<QAudioFormat::SampleType> sampleTypesAvailable = supportedSampleTypes();
    QMap<int,int> testFrequencies;
    QList<int> frequenciesAvailable = supportedFrequencies();
    QMap<int,int> testSampleSizes;
    QList<int> sampleSizesAvailable = supportedSampleSizes();

    // Get sorted lists for checking
    if (testCodecs.contains(settings.codec())) {
        testCodecs.removeAll(settings.codec());
        testCodecs.insert(0, settings.codec());
    }
    testChannels.removeAll(settings.channels());
    testChannels.insert(0, settings.channels());
    testByteOrders.removeAll(settings.byteOrder());
    testByteOrders.insert(0, settings.byteOrder());

    if (sampleTypesAvailable.contains(settings.sampleType()))
        testSampleTypes.append(settings.sampleType());
    if (sampleTypesAvailable.contains(QAudioFormat::SignedInt))
        testSampleTypes.append(QAudioFormat::SignedInt);
    if (sampleTypesAvailable.contains(QAudioFormat::UnSignedInt))
        testSampleTypes.append(QAudioFormat::UnSignedInt);
    if (sampleTypesAvailable.contains(QAudioFormat::Float))
        testSampleTypes.append(QAudioFormat::Float);

    if (sampleSizesAvailable.contains(settings.sampleSize()))
        testSampleSizes.insert(0,settings.sampleSize());
    sampleSizesAvailable.removeAll(settings.sampleSize());
    foreach (int size, sampleSizesAvailable) {
        int larger  = (size > settings.sampleSize()) ? size : settings.sampleSize();
        int smaller = (size > settings.sampleSize()) ? settings.sampleSize() : size;
        bool isMultiple = ( 0 == (larger % smaller));
        int diff = larger - smaller;
        testSampleSizes.insert((isMultiple ? diff : diff+100000), size);
    }
    if (frequenciesAvailable.contains(settings.frequency()))
        testFrequencies.insert(0,settings.frequency());
    frequenciesAvailable.removeAll(settings.frequency());
    foreach (int frequency, frequenciesAvailable) {
        int larger  = (frequency > settings.frequency()) ? frequency : settings.frequency();
        int smaller = (frequency > settings.frequency()) ? settings.frequency() : frequency;
        bool isMultiple = ( 0 == (larger % smaller));
        int diff = larger - smaller;
        testFrequencies.insert((isMultiple ? diff : diff+100000), frequency);
    }

    // Try to find nearest
    foreach (QString codec, testCodecs) {
        nearest.setCodec(codec);
        foreach (QAudioFormat::Endian order, testByteOrders) {
            nearest.setByteOrder(order);
            foreach (QAudioFormat::SampleType sample, testSampleTypes) {
                nearest.setSampleType(sample);
                QMapIterator<int, int> sz(testSampleSizes);
                while (sz.hasNext()) {
                    sz.next();
                    nearest.setSampleSize(sz.value());
                    foreach (int channel, testChannels) {
                        nearest.setChannels(channel);
                        QMapIterator<int, int> i(testFrequencies);
                        while (i.hasNext()) {
                            i.next();
                            nearest.setFrequency(i.value());
                            if (isFormatSupported(nearest))
                                return nearest;
                        }
                    }
                }
            }
        }
    }
    //Fallback
    return preferredFormat();
}

/*!
    Returns a list of supported codecs.

    All platform and plugin implementations should provide support for:

    "audio/pcm" - Linear PCM

    For writing plugins to support additional codecs refer to:

    http://www.iana.org/assignments/media-types/audio/
    \since 1.0
*/
QStringList QAudioDeviceInfo::supportedCodecs() const
{
    return isNull() ? QStringList() : d->info->supportedCodecs();
}

/*!
    Returns a list of supported sample rates (in Hertz).

    \since 1.0
*/
QList<int> QAudioDeviceInfo::supportedSampleRates() const
{
    return supportedFrequencies();
}

/*!
    \obsolete

    Use supportedSampleRates() instead.
    \since 1.0
*/
QList<int> QAudioDeviceInfo::supportedFrequencies() const
{
    return isNull() ? QList<int>() : d->info->supportedSampleRates();
}

/*!
    Returns a list of supported channel counts.

    This is typically 1 for mono sound, or 2 for stereo sound.

    \since 1.0
*/
QList<int> QAudioDeviceInfo::supportedChannelCounts() const
{
    return supportedChannels();
}

/*!
    \obsolete

    Use supportedChannelCount() instead.
    \since 1.0
*/
QList<int> QAudioDeviceInfo::supportedChannels() const
{
    return isNull() ? QList<int>() : d->info->supportedChannelCounts();
}

/*!
    Returns a list of supported sample sizes (in bits).

    Typically this will include 8 and 16 bit sample sizes.

    \since 1.0
*/
QList<int> QAudioDeviceInfo::supportedSampleSizes() const
{
    return isNull() ? QList<int>() : d->info->supportedSampleSizes();
}

/*!
    Returns a list of supported byte orders.
    \since 1.0
*/
QList<QAudioFormat::Endian> QAudioDeviceInfo::supportedByteOrders() const
{
    return isNull() ? QList<QAudioFormat::Endian>() : d->info->supportedByteOrders();
}

/*!
    Returns a list of supported sample types.
    \since 1.0
*/
QList<QAudioFormat::SampleType> QAudioDeviceInfo::supportedSampleTypes() const
{
    return isNull() ? QList<QAudioFormat::SampleType>() : d->info->supportedSampleTypes();
}

/*!
    Returns the information for the default input audio device.
    All platform and audio plugin implementations provide a default audio device to use.
    \since 1.0
*/
QAudioDeviceInfo QAudioDeviceInfo::defaultInputDevice()
{
    return QAudioDeviceFactory::defaultInputDevice();
}

/*!
    Returns the information for the default output audio device.
    All platform and audio plugin implementations provide a default audio device to use.
    \since 1.0
*/
QAudioDeviceInfo QAudioDeviceInfo::defaultOutputDevice()
{
    return QAudioDeviceFactory::defaultOutputDevice();
}

/*!
    Returns a list of audio devices that support \a mode.
    \since 1.0
*/
QList<QAudioDeviceInfo> QAudioDeviceInfo::availableDevices(QAudio::Mode mode)
{
    return QAudioDeviceFactory::availableDevices(mode);
}


/*!
    \internal
    \since 1.0
*/
QAudioDeviceInfo::QAudioDeviceInfo(const QString &realm, const QByteArray &handle, QAudio::Mode mode):
    d(new QAudioDeviceInfoPrivate(realm, handle, mode))
{
}

/*!
    \internal
    \since 1.0
*/
QString QAudioDeviceInfo::realm() const
{
    return d->realm;
}

/*!
    \internal
    \since 1.0
*/
QByteArray QAudioDeviceInfo::handle() const
{
    return d->handle;
}


/*!
    \internal
    \since 1.0
*/
QAudio::Mode QAudioDeviceInfo::mode() const
{
    return d->mode;
}

QT_END_NAMESPACE

