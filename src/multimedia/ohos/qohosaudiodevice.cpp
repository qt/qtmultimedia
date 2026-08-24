// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosaudiodevice_p.h"

#include <private/qaudiodevice_p.h>
#include <private/qaudioformat_p.h>

#include "qohosaudiodevice_p.h"

QT_BEGIN_NAMESPACE

namespace {

QAudioDevicePrivate::AudioDeviceFormat
createOhosAudioDeviceFormatFromPreferred(const QAudioFormat &preferredFormat,
                                         int maximumChannelCount)
{
    QAudioDevicePrivate::AudioDeviceFormat format;

    format.preferredFormat = preferredFormat;

    // A mono stream is upmixed and the stream resamples, so only the maximum
    // channel count is taken from the device.
    format.minimumChannelCount = 1;
    format.maximumChannelCount = maximumChannelCount > 0 ? maximumChannelCount : 32;
    format.minimumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.front();
    format.maximumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.back();
    format.supportedSampleFormats = qAllSupportedSampleFormats();
    format.channelConfiguration = preferredFormat.channelConfig();

    return format;
}

} // namespace

QOhosAudioDevice::QOhosAudioDevice(QByteArray id, QString description, QAudioDevice::Mode mode,
                                   QAudioFormat preferredFormat, int maximumChannelCount,
                                   bool isDefaultDevice)
    : QAudioDevicePrivate{ std::move(id), mode, std::move(description), isDefaultDevice,
                           createOhosAudioDeviceFormatFromPreferred(preferredFormat, maximumChannelCount) }
{
}

QT_END_NAMESPACE
