// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidaudiodevice_p.h"

#include <private/qaudioformat_p.h>

#include <QtCore/qjniobject.h>

QT_BEGIN_NAMESPACE

QAndroidAudioDevice::QAndroidAudioDevice(QByteArray device, QString desc, QAudioDevice::Mode mode,
                                         QAudioFormat format, bool isBluetoothDevice,
                                         bool isDefaultDevice)
    : QAudioDevicePrivate(std::move(device), mode, std::move(desc)),
      m_isBluetoothDevice(isBluetoothDevice)
{
    isDefault = isDefaultDevice;
    preferredFormat = format;

    // Report support for everything that Qt supports, as Android should be able to resample and
    // up/downmix if needed
    minimumChannelCount = 1;
    maximumChannelCount = 32;
    minimumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.front();
    maximumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.back();
    supportedSampleFormats = QList<QAudioFormat::SampleFormat>{
        QtMultimediaPrivate::allSupportedSampleFormats.begin(),
        QtMultimediaPrivate::allSupportedSampleFormats.end()
    };
}

bool QAndroidAudioDevice::isBluetoothDevice() const
{
    return m_isBluetoothDevice;
}

QT_END_NAMESPACE
