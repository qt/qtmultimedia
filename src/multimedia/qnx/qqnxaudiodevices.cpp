// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxaudiodevices_p.h"
#include "qmediadevices.h"
#include "private/qcameradevice_p.h"
#include "qcameradevice.h"

#include "qqnxaudiosource_p.h"
#include "qqnxaudiosink_p.h"
#include "qqnxaudiodevice_p.h"

#include <qdir.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

static QList<QAudioDevice> enumeratePcmDevices(QAudioDevice::Mode mode)
{
    if (mode == QAudioDevice::Null)
        return {};

    QDir dir(QStringLiteral("/dev/snd"));

    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name);

    // QNX PCM devices names start with the pcm prefix and end either with the
    // 'p' (playback) or 'c' (capture) suffix

    const QChar modeSuffix = mode == QAudioDevice::Input ? u'c' : u'p';

    QList<QAudioDevice> devices;

    for (const QString &entry : dir.entryList()) {
        if (entry.startsWith(QStringLiteral("pcm")) && entry.back() == modeSuffix)
            devices << QAudioDevicePrivate::createQAudioDevice(
                    std::make_unique<QnxAudioDeviceInfo>(entry.toUtf8(), mode));
    }

    return devices;
}

QQnxAudioDevices::QQnxAudioDevices()
    : QPlatformAudioDevices()
{
}

QList<QAudioDevice> QQnxAudioDevices::findAudioInputs() const
{
    return ::enumeratePcmDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QQnxAudioDevices::findAudioOutputs() const
{
    return ::enumeratePcmDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QQnxAudioDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                          const QAudioFormat &fmt,
                                                          QObject *parent)
{
    return new QQnxAudioSource(deviceInfo, fmt, parent);
}

QPlatformAudioSink *QQnxAudioDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                      const QAudioFormat &fmt,
                                                      QObject *parent)
{
    return new QQnxAudioSink(deviceInfo, fmt, parent);
}

QT_END_NAMESPACE
