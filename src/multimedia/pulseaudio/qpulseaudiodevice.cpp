// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpulseaudiodevice_p.h"
#include "qaudioengine_pulse_p.h"
#include "qpulsehelpers_p.h"

QT_BEGIN_NAMESPACE

QPulseAudioDeviceInfo::QPulseAudioDeviceInfo(const char *device, const char *desc, bool isDef, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode)
{
    description = QString::fromUtf8(desc);
    isDefault = isDef;

    minimumChannelCount = 1;
    maximumChannelCount = PA_CHANNELS_MAX;
    minimumSampleRate = 1;
    maximumSampleRate = PA_RATE_MAX;

    constexpr bool isBigEndian = QSysInfo::ByteOrder == QSysInfo::BigEndian;

    const struct {
        pa_sample_format pa_fmt;
        QAudioFormat::SampleFormat qt_fmt;
    } formatMap[] = {
        { PA_SAMPLE_U8, QAudioFormat::UInt8 },
        { isBigEndian ? PA_SAMPLE_S16BE : PA_SAMPLE_S16LE, QAudioFormat::Int16 },
        { isBigEndian ? PA_SAMPLE_S32BE : PA_SAMPLE_S32LE, QAudioFormat::Int32 },
        { isBigEndian ? PA_SAMPLE_FLOAT32BE : PA_SAMPLE_FLOAT32LE, QAudioFormat::Float },
    };

    for (const auto &f : formatMap) {
        if (pa_sample_format_valid(f.pa_fmt) != 0)
            supportedSampleFormats.append(f.qt_fmt);
    }

    preferredFormat.setChannelCount(2);
    preferredFormat.setSampleRate(48000);
    QAudioFormat::SampleFormat f = QAudioFormat::Int16;
    if (!supportedSampleFormats.contains(f))
        f = supportedSampleFormats.value(0, QAudioFormat::Unknown);
    preferredFormat.setSampleFormat(f);
}

QT_END_NAMESPACE
