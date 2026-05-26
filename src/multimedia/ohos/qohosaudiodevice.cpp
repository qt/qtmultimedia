// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosaudiodevice_p.h"

#include <private/qaudiodevice_p.h>
#include <private/qaudioformat_p.h>

QT_BEGIN_NAMESPACE

namespace {

QAudioDevicePrivate::AudioDeviceFormat
createOhosAudioDeviceFormatFromPreferred(const QAudioFormat &preferredFormat)
{
    QAudioDevicePrivate::AudioDeviceFormat format;

    format.preferredFormat = preferredFormat;

    format.minimumChannelCount = 1;
    format.maximumChannelCount = 32;
    format.minimumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.front();
    format.maximumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.back();
    format.supportedSampleFormats = qAllSupportedSampleFormats();
    format.channelConfiguration = preferredFormat.channelConfig();

    return format;
}

} // namespace

QOhosAudioDevice::QOhosAudioDevice(QByteArray id, QString description, QAudioDevice::Mode mode,
                                   QAudioFormat preferredFormat, bool isDefaultDevice)
    : QAudioDevicePrivate{ std::move(id), mode, std::move(description), isDefaultDevice,
                           createOhosAudioDeviceFormatFromPreferred(preferredFormat) }
{
}

QT_END_NAMESPACE
