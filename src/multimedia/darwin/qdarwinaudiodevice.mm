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
        preferredFormat = determinePreferredFormat();
        description = getDescription();
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

QT_END_NAMESPACE
