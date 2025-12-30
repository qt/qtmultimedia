// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidaudiodevice_p.h"

#include <private/qaudioformat_p.h>
#include <private/qaudiodevice_p.h>

#include <QtCore/qjniobject.h>

QT_BEGIN_NAMESPACE

namespace {

QAudioDevicePrivate::AudioDeviceFormat
createAndroidAudioDeviceFormatFromPreferred(const QAudioFormat &preferredFormat)
{
    QAudioDevicePrivate::AudioDeviceFormat format;

    format.preferredFormat = preferredFormat;

    // Report support for everything that Qt supports, as Android should be able to resample and
    // up/downmix if needed
    format.minimumChannelCount = 1;
    format.maximumChannelCount = 32;
    format.minimumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.front();
    format.maximumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.back();
    format.supportedSampleFormats = qAllSupportedSampleFormats();
    format.channelConfiguration = preferredFormat.channelConfig();

    return format;
}

} // namespace

QAndroidAudioDevice::QAndroidAudioDevice(QByteArray device, QString desc, QAudioDevice::Mode mode,
                                         QAudioFormat format, bool isBluetoothDevice,
                                         bool isDefaultDevice)
    : QAudioDevicePrivate{ std::move(device), mode, std::move(desc), isDefaultDevice,
                           createAndroidAudioDeviceFormatFromPreferred(format) },
      m_isBluetoothDevice(isBluetoothDevice)
{
}

bool QAndroidAudioDevice::isBluetoothDevice() const
{
    return m_isBluetoothDevice;
}

QT_END_NAMESPACE
