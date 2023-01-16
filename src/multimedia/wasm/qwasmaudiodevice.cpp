// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmaudiodevice_p.h"
#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>

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
    minimumSampleRate = 8000;
    maximumSampleRate = 96000; // js AudioContext max according to docs

    // native openAL formats
    supportedSampleFormats.append(QAudioFormat::UInt8);
    supportedSampleFormats.append(QAudioFormat::Int16);

    // Browsers use 32bit floats as native, but emscripten reccomends checking for the exension.
    if (alIsExtensionPresent("AL_EXT_float32"))
        supportedSampleFormats.append(QAudioFormat::Float);

    preferredFormat.setChannelCount(2);

    // FIXME: firefox
    // An AudioContext was prevented from starting automatically.
    // It must be created or resumed after a user gesture on the page.
    emscripten::val audioContext = emscripten::val::global("window")["AudioContext"].new_();
    if (audioContext == emscripten::val::undefined())
        audioContext = emscripten::val::global("window")["webkitAudioContext"].new_();

    if (audioContext != emscripten::val::undefined()) {
        audioContext.call<void>("resume");
        int sRate = audioContext["sampleRate"].as<int>();
        audioContext.call<void>("close");
        preferredFormat.setSampleRate(sRate);
    }

    auto f = QAudioFormat::Float;

    if (!supportedSampleFormats.contains(f))
        f = QAudioFormat::Int16;
    preferredFormat.setSampleFormat(f);
}

QT_END_NAMESPACE
