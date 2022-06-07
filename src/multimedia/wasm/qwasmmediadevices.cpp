// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmmediadevices_p.h"
#include "private/qcameradevice_p.h"

#include "qwasmaudiosource_p.h"
#include "qwasmaudiosink_p.h"
#include "qwasmaudiodevice_p.h"
#include <AL/al.h>
#include <AL/alc.h>

QT_BEGIN_NAMESPACE

QWasmMediaDevices::QWasmMediaDevices()
    : QPlatformMediaDevices()
{
    auto capture = alcGetString(nullptr, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
    // present even if there is no capture device
    if (capture)
        m_ins.append((new QWasmAudioDevice(capture, "WebAssembly audio capture device", true,
                                               QAudioDevice::Input))->create());

    auto playback = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    // present even if there is no playback device
    if (playback)
        m_outs.append((new QWasmAudioDevice(playback, "WebAssembly audio playback device", true,
                                                QAudioDevice::Output))->create());
}

QList<QAudioDevice> QWasmMediaDevices::audioInputs() const
{
    return m_ins;
}

QList<QAudioDevice> QWasmMediaDevices::audioOutputs() const
{
    return m_outs;
}

QList<QCameraDevice> QWasmMediaDevices::videoInputs() const
{
    return {};
}

QPlatformAudioSource *QWasmMediaDevices::createAudioSource(const QAudioDevice &deviceInfo)
{
    return new QWasmAudioSource(deviceInfo.id());
}

QPlatformAudioSink *QWasmMediaDevices::createAudioSink(const QAudioDevice &deviceInfo)
{
    return new QWasmAudioSink(deviceInfo.id());
}

QT_END_NAMESPACE
