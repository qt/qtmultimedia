/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

    pa_sample_spec spec;
    spec.channels = 1;
    spec.rate = 48000;

    for (const auto &f : formatMap) {
        spec.format = f.pa_fmt;
        if (pa_sample_spec_valid(&spec) != 0)
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
