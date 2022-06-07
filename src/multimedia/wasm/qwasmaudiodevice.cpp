// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmaudiodevice_p.h"
#include <emscripten.h>
#include <AL/al.h>
#include <AL/alc.h>

QT_BEGIN_NAMESPACE

QWasmAudioDevice::QWasmAudioDevice(const char *device, const char *desc, bool isDef, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode)
{
    description = QString::fromUtf8(desc);
    isDefault = isDef;

    minimumChannelCount = 1;
    maximumChannelCount = 2;
    minimumSampleRate = 1;
    maximumSampleRate = 192'000;

    // native openAL formats
    supportedSampleFormats.append(QAudioFormat::UInt8);
    supportedSampleFormats.append(QAudioFormat::Int16);

    // Browsers use 32bit floats as native, but emscripten reccomends checking for the exension.
    if (alIsExtensionPresent("AL_EXT_float32"))
        supportedSampleFormats.append(QAudioFormat::Float);

    preferredFormat.setChannelCount(2);

    preferredFormat.setSampleRate(EM_ASM_INT({
        var AudioContext = window.AudioContext || window.webkitAudioContext;
        var ctx = new AudioContext();
        var sr = ctx.sampleRate;
        ctx.close();
        return sr;
    }));

    auto f = QAudioFormat::Float;

    if (!supportedSampleFormats.contains(f))
        f = QAudioFormat::Int16;
    preferredFormat.setSampleFormat(f);
}

QT_END_NAMESPACE
