// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinaudiodevice_p.h"
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

#if defined(Q_OS_MACOS)
QCoreAudioDeviceInfo::QCoreAudioDeviceInfo(AudioDeviceID id, const QByteArray &device, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode),
    m_deviceId(id)
#else
QCoreAudioDeviceInfo::QCoreAudioDeviceInfo(const QByteArray &device, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode)
#endif
{
    description = getDescription();
    getChannelLayout();
    preferredFormat = determinePreferredFormat();
    minimumSampleRate = 1;
    maximumSampleRate = 96000;
    minimumChannelCount = 1;
    maximumChannelCount = 16;
    supportedSampleFormats << QAudioFormat::UInt8 << QAudioFormat::Int16 << QAudioFormat::Int32 << QAudioFormat::Float;

}


QAudioFormat QCoreAudioDeviceInfo::determinePreferredFormat() const
{
    QAudioFormat format;

#if defined(Q_OS_MACOS)
    UInt32  propSize = 0;
    AudioObjectPropertyScope audioDevicePropertyScope = mode == QAudioDevice::Input ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
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
                            qWarning() << "QAudioDevice: Unable to find perferedFormat for stream";
                        }
                    } else {
                        qWarning() << "QAudioDevice: Unable to find size of perferedFormat for stream";
                    }
                }
            }

            delete[] streams;
        }
    }
    if (!format.isValid())
#endif
    {
        format.setSampleRate(44100);
        format.setSampleFormat(QAudioFormat::Int16);
        format.setChannelCount(mode == QAudioDevice::Input ? 1 : 2);
    }
    format.setChannelConfig(channelConfiguration);

    return format;
}


QString QCoreAudioDeviceInfo::getDescription() const
{
#ifdef Q_OS_MACOS
    CFStringRef name;
    UInt32 size = sizeof(CFStringRef);
    AudioObjectPropertyScope audioPropertyScope = mode == QAudioDevice::Input ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;

    AudioObjectPropertyAddress audioDeviceNamePropertyAddress = { kAudioObjectPropertyName,
                                                                  audioPropertyScope,
                                                                  kAudioObjectPropertyElementMaster };

    if (AudioObjectGetPropertyData(m_deviceId, &audioDeviceNamePropertyAddress, 0, NULL, &size, &name) != noErr) {
        qWarning() << "QAudioDevice: Unable to find device description";
        return QString();
    }

    QString s = QString::fromCFString(name);
    CFRelease(name);
    return s;
#else
    return QString::fromUtf8(id);
#endif
}

void QCoreAudioDeviceInfo::getChannelLayout()
{
#ifdef Q_OS_MACOS
    AudioObjectPropertyAddress audioDeviceChannelLayoutPropertyAddress = { kAudioDevicePropertyPreferredChannelLayout,
                                                                    (mode == QAudioDevice::Input ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput),
                                                                    kAudioObjectPropertyElementMaster };
    UInt32 propSize;
    if (AudioObjectGetPropertyDataSize(m_deviceId, &audioDeviceChannelLayoutPropertyAddress, 0, nullptr, &propSize) == noErr) {
        AudioChannelLayout *layout = static_cast<AudioChannelLayout *>(malloc(propSize));
        if (AudioObjectGetPropertyData(m_deviceId, &audioDeviceChannelLayoutPropertyAddress, 0, nullptr, &propSize, layout) == noErr) {
            channelConfiguration = CoreAudioUtils::fromAudioChannelLayout(layout);
        }
        free(layout);
    }
#else
    channelConfiguration = (mode == QAudioDevice::Input) ? QAudioFormat::ChannelConfigMono : QAudioFormat::ChannelConfigStereo;
#endif
}

QT_END_NAMESPACE
