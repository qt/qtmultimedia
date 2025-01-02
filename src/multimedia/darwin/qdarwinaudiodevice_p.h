// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef IOSAUDIODEVICEINFO_H
#define IOSAUDIODEVICEINFO_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qaudiosystem_p.h>
#include <private/qaudiodevice_p.h>

#if defined(Q_OS_MACOS)
# include <CoreAudio/CoreAudio.h>
#endif

QT_BEGIN_NAMESPACE

class QCoreAudioDeviceInfo : public QAudioDevicePrivate
{
public:
#if defined(Q_OS_MACOS)
    QCoreAudioDeviceInfo(AudioDeviceID id, const QByteArray &device, QAudioDevice::Mode mode);
#else
    QCoreAudioDeviceInfo(const QByteArray &device, QAudioDevice::Mode mode);
#endif
    ~QCoreAudioDeviceInfo() {}

    bool isFormatSupported(const QAudioFormat &format) const;

#if defined(Q_OS_MACOS)
    AudioDeviceID deviceID() const { return m_deviceId; }
#endif
private:
    QAudioFormat determinePreferredFormat() const;
    QString getDescription() const;
    void getChannelLayout();
#if defined(Q_OS_MACOS)
    AudioDeviceID m_deviceId;
#endif
};

QT_END_NAMESPACE

#endif
