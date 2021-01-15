/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "qcoreaudiodeviceinfo_p.h"
#include "qcoreaudioutils_p.h"
#include <private/qcore_mac_p.h>

#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
# include "qcoreaudiosessionmanager_p.h"
#endif

#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QSet>
#include <QIODevice>

QT_BEGIN_NAMESPACE

QCoreAudioDeviceInfo::QCoreAudioDeviceInfo(AudioDeviceID id, const QByteArray &device, QAudio::Mode mode)
    : QAudioDeviceInfoPrivate(device, mode),
      m_deviceId(id)
{
}


QAudioFormat QCoreAudioDeviceInfo::preferredFormat() const
{
    QAudioFormat format;

#if defined(Q_OS_OSX)
    UInt32  propSize = 0;
    AudioObjectPropertyScope audioDevicePropertyScope = mode == QAudio::AudioInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
    AudioObjectPropertyAddress audioDevicePropertyStreamsAddress = { kAudioDevicePropertyStreams,
                                                                     audioDevicePropertyScope,
                                                                     kAudioObjectPropertyElementMaster };

    if (AudioObjectGetPropertyDataSize(m_deviceId, &audioDevicePropertyStreamsAddress, 0, NULL, &propSize) == noErr) {

        const int sc = propSize / sizeof(AudioStreamID);

        if (sc > 0) {
            AudioStreamID*  streams = new AudioStreamID[sc];

            if (AudioObjectGetPropertyData(m_deviceId, &audioDevicePropertyStreamsAddress, 0, NULL, &propSize, streams) == noErr) {

                AudioObjectPropertyAddress audioDevicePhysicalFormatPropertyAddress = { kAudioStreamPropertyPhysicalFormat,
                                                                                        kAudioObjectPropertyScopeGlobal,
                                                                                        kAudioObjectPropertyElementMaster };

                for (int i = 0; i < sc; ++i) {
                    if (AudioObjectGetPropertyDataSize(streams[i], &audioDevicePhysicalFormatPropertyAddress, 0, NULL, &propSize) == noErr) {
                        AudioStreamBasicDescription sf;

                        if (AudioObjectGetPropertyData(streams[i], &audioDevicePhysicalFormatPropertyAddress, 0, NULL, &propSize, &sf) == noErr) {
                            format = CoreAudioUtils::toQAudioFormat(sf);
                            break;
                        } else {
                            qWarning() << "QAudioDeviceInfo: Unable to find perferedFormat for stream";
                        }
                    } else {
                        qWarning() << "QAudioDeviceInfo: Unable to find size of perferedFormat for stream";
                    }
                }
            }

            delete[] streams;
        }
    }
#else //iOS
    format.setSampleSize(16);
    if (mode == QAudio::AudioInput) {
        format.setChannelCount(1);
        format.setSampleRate(8000);
    } else {
        format.setChannelCount(2);
        format.setSampleRate(44100);
    }
    format.setCodec(QString::fromLatin1("audio/x-raw"));
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
#endif

    return format;
}


bool QCoreAudioDeviceInfo::isFormatSupported(const QAudioFormat &format) const
{
    QCoreAudioDeviceInfo *self = const_cast<QCoreAudioDeviceInfo*>(this);

    //Sample rates are more of a suggestion with CoreAudio so as long as we get a
    //sane value then we can likely use it.
    return format.isValid()
            && format.codec() == QString::fromLatin1("audio/x-raw")
            && format.sampleRate() > 0
            && self->supportedChannelCounts().contains(format.channelCount())
            && self->supportedSampleSizes().contains(format.sampleSize());
}


QString QCoreAudioDeviceInfo::description() const
{
#ifdef Q_OS_MACOS
    CFStringRef name;
    UInt32 size = sizeof(CFStringRef);
    AudioObjectPropertyScope audioPropertyScope = mode == QAudio::AudioInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;

    AudioObjectPropertyAddress audioDeviceNamePropertyAddress = { kAudioObjectPropertyName,
                                                                  audioPropertyScope,
                                                                  kAudioObjectPropertyElementMaster };

    if (AudioObjectGetPropertyData(m_deviceId, &audioDeviceNamePropertyAddress, 0, NULL, &size, &name) != noErr) {
        qWarning() << "QAudioDeviceInfo: Unable to find device description";
        return QString();
    }

    QString s = QString::fromCFString(name);
    CFRelease(name);
    return s;
#else
    return QString::fromUtf8(m_device);
#endif
}


QStringList QCoreAudioDeviceInfo::supportedCodecs() const
{
    return QStringList() << QString::fromLatin1("audio/x-raw");
}


QList<int> QCoreAudioDeviceInfo::supportedSampleRates() const
{
    QSet<int> sampleRates;

#if defined(Q_OS_OSX)
    UInt32  propSize = 0;
    AudioObjectPropertyScope scope = mode == QAudio::AudioInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
    AudioObjectPropertyAddress availableNominalSampleRatesAddress = { kAudioDevicePropertyAvailableNominalSampleRates,
                                                                      scope,
                                                                      kAudioObjectPropertyElementMaster };

    if (AudioObjectGetPropertyDataSize(m_deviceId, &availableNominalSampleRatesAddress, 0, NULL, &propSize) == noErr) {
        const int pc = propSize / sizeof(AudioValueRange);

        if (pc > 0) {
            AudioValueRange* vr = new AudioValueRange[pc];

            if (AudioObjectGetPropertyData(m_deviceId, &availableNominalSampleRatesAddress, 0, NULL, &propSize, vr) == noErr) {
                for (int i = 0; i < pc; ++i) {
                    sampleRates << vr[i].mMinimum << vr[i].mMaximum;
                }
            }

            delete[] vr;
        }
    }
#else //iOS
    //iOS doesn't have a way to query available sample rates
    //instead we provide reasonable targets
    //It may be necessary have CoreAudioSessionManger test combinations
    //with available hardware
    sampleRates << 8000 << 11025 << 22050 << 44100 << 48000;
#endif
    return sampleRates.values();
}


QList<int> QCoreAudioDeviceInfo::supportedChannelCounts() const
{
    static QList<int> supportedChannels;

    if (supportedChannels.isEmpty()) {
        // If the number of channels is not supported by an audio device, Core Audio will
        // automatically convert the audio data.
        for (int i = 1; i <= 16; ++i)
            supportedChannels.append(i);
    }

    return supportedChannels;
}


QList<int> QCoreAudioDeviceInfo::supportedSampleSizes() const
{
    return QList<int>() << 8 << 16 << 24 << 32 << 64;
}


QList<QAudioFormat::Endian> QCoreAudioDeviceInfo::supportedByteOrders() const
{
    return QList<QAudioFormat::Endian>() << QAudioFormat::LittleEndian << QAudioFormat::BigEndian;
}

QList<QAudioFormat::SampleType> QCoreAudioDeviceInfo::supportedSampleTypes() const
{
    return QList<QAudioFormat::SampleType>() << QAudioFormat::SignedInt << QAudioFormat::UnSignedInt << QAudioFormat::Float;
}

QT_END_NAMESPACE
