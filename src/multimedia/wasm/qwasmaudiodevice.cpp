// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmaudiodevice_p.h"
#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>

QT_BEGIN_NAMESPACE

namespace {

// Probe the hardware sample rate once and cache it. The rate is the same for
// all devices, so creating a temporary AudioContext per enumerated device is
// wasteful and can cause browser device errors from overlapping contexts.
int probeHardwareSampleRate()
{
    static int cachedRate = 0;
    if (cachedRate > 0)
        return cachedRate;

    // FIXME: firefox
    // An AudioContext was prevented from starting automatically.
    // It must be created or resumed after a user gesture on the page.
    emscripten::val ctx = emscripten::val::global("window")["AudioContext"].new_();
    if (ctx == emscripten::val::undefined())
        ctx = emscripten::val::global("window")["webkitAudioContext"].new_();

    if (ctx != emscripten::val::undefined()) {
        cachedRate = ctx["sampleRate"].as<int>();
        ctx.call<void>("close");
    }

    return cachedRate;
}

QAudioDevicePrivate::AudioDeviceFormat createDefaultWasmAudioDeviceFormat()
{
    QAudioDevicePrivate::AudioDeviceFormat format;

    format.minimumChannelCount = 1;
    format.maximumChannelCount = 2;
    format.minimumSampleRate = 8000;
    format.maximumSampleRate = 96000; // js AudioContext max according to docs

    // WebAudio natively supports all these formats via AudioWorklet.
    format.supportedSampleFormats.append(QAudioFormat::UInt8);
    format.supportedSampleFormats.append(QAudioFormat::Int16);
    format.supportedSampleFormats.append(QAudioFormat::Int32);
    format.supportedSampleFormats.append(QAudioFormat::Float);

    format.preferredFormat.setChannelCount(2);

    if (int sRate = probeHardwareSampleRate(); sRate > 0)
        format.preferredFormat.setSampleRate(sRate);

    format.preferredFormat.setSampleFormat(QAudioFormat::Float);

    return format;
}

} // namespace

QWasmAudioDevice::QWasmAudioDevice(const char *device, const char *desc, bool isDef,
                                   QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode, QString::fromUtf8(desc), isDef,
                          createDefaultWasmAudioDeviceFormat())
{
}

QT_END_NAMESPACE
