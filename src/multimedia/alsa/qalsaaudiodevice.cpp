// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qalsaaudiodevice_p.h"

QT_BEGIN_NAMESPACE

namespace {

QAudioDevicePrivate::AudioDeviceFormat
createAlsaAudioDeviceFormatFromDeviceName(const QByteArray &dev, QAudioDevice::Mode mode)
{
    QAudioDevicePrivate::AudioDeviceFormat format;

    // Check surround capabilities
    bool surround40 = false;
    bool surround51 = false;
    bool surround71 = false;

    if (mode == QAudioDevice::Output) {
        if (dev.startsWith(QLatin1String("surround40")))
            surround40 = true;
        if (dev.startsWith(QLatin1String("surround51")))
            surround51 = true;
        if (dev.startsWith(QLatin1String("surround71")))
            surround71 = true;
    }

    format.minimumChannelCount = 1;
    format.maximumChannelCount = 2;
    if (surround71)
        format.maximumChannelCount = 8;
    else if (surround40)
        format.maximumChannelCount = 4;
    else if (surround51)
        format.maximumChannelCount = 6;

    format.minimumSampleRate = 8000;
    format.maximumSampleRate = 48000;

    format.supportedSampleFormats = {
        QAudioFormat::UInt8,
        QAudioFormat::Int16,
        QAudioFormat::Int32,
        QAudioFormat::Float,
    };

    format.preferredFormat.setChannelCount(mode == QAudioDevice::Input ? 1 : 2);
    format.preferredFormat.setSampleFormat(QAudioFormat::Float);
    format.preferredFormat.setSampleRate(48000);

    return format;
}

} // namespace

QAlsaAudioDeviceInfo::QAlsaAudioDeviceInfo(const QByteArray &dev, const QString &desc,
                                           QAudioDevice::Mode mode)
    : QAudioDevicePrivate(dev, mode, desc, false,
                          createAlsaAudioDeviceFormatFromDeviceName(dev, mode))
{
}

QAlsaAudioDeviceInfo::~QAlsaAudioDeviceInfo() = default;

QT_END_NAMESPACE
