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
