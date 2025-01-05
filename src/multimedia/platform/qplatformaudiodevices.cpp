// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformaudiodevices_p.h"
#include "qcameradevice.h"
#include "qaudiosystem_p.h"
#include "qaudiodevice.h"

#if defined(Q_OS_ANDROID)
#include <qandroidaudiodevices_p.h>
#elif defined(Q_OS_DARWIN)
#include <qdarwinaudiodevices_p.h>
#elif defined(Q_OS_WINDOWS) && QT_CONFIG(wmf)
#include <qwindowsaudiodevices_p.h>
#elif QT_CONFIG(alsa)
#include <qalsaaudiodevices_p.h>
#elif QT_CONFIG(pulseaudio)
#include <qpulseaudiodevices_p.h>
#elif defined(Q_OS_QNX)
#include <qqnxaudiodevices_p.h>
#elif defined(Q_OS_WASM)
#include <private/qwasmmediadevices_p.h>
#endif

QT_BEGIN_NAMESPACE

std::unique_ptr<QPlatformAudioDevices> QPlatformAudioDevices::create()
{
#ifdef Q_OS_DARWIN
    return std::make_unique<QDarwinAudioDevices>();
#elif defined(Q_OS_WINDOWS) && QT_CONFIG(wmf)
    return std::make_unique<QWindowsAudioDevices>();
#elif defined(Q_OS_ANDROID)
    return std::make_unique<QAndroidAudioDevices>();
#elif QT_CONFIG(alsa)
    return std::make_unique<QAlsaAudioDevices>();
#elif QT_CONFIG(pulseaudio)
    return std::make_unique<QPulseAudioDevices>();
#elif defined(Q_OS_QNX)
    return std::make_unique<QQnxAudioDevices>();
#elif defined(Q_OS_WASM)
    return std::make_unique<QWasmMediaDevices>();
#else
    return std::make_unique<QPlatformAudioDevices>();
#endif
}

QPlatformAudioDevices::QPlatformAudioDevices()
{
    qRegisterMetaType<PrivateTag>(); // for the case of queued connections
}

QPlatformAudioDevices::~QPlatformAudioDevices() = default;

QList<QAudioDevice> QPlatformAudioDevices::audioInputs() const
{
    return m_audioInputs.ensure([this]() {
        return findAudioInputs();
    });
}

QList<QAudioDevice> QPlatformAudioDevices::audioOutputs() const
{
    return m_audioOutputs.ensure([this]() {
        return findAudioOutputs();
    });
}

void QPlatformAudioDevices::onAudioInputsChanged()
{
    m_audioInputs.reset();
    emit audioInputsChanged(PrivateTag{});
}

void QPlatformAudioDevices::onAudioOutputsChanged()
{
    m_audioOutputs.reset();
    emit audioOutputsChanged(PrivateTag{});
}

void QPlatformAudioDevices::updateAudioInputsCache()
{
    if (m_audioInputs.update(findAudioInputs()))
        emit audioInputsChanged(PrivateTag{});
}

void QPlatformAudioDevices::updateAudioOutputsCache()
{
    if (m_audioOutputs.update(findAudioOutputs()))
        emit audioOutputsChanged(PrivateTag{});
}

QPlatformAudioSource *QPlatformAudioDevices::createAudioSource(const QAudioDevice &, QObject *)
{
    return nullptr;
}
QPlatformAudioSink *QPlatformAudioDevices::createAudioSink(const QAudioDevice &, QObject *)
{
    return nullptr;
}

QPlatformAudioSource *QPlatformAudioDevices::audioInputDevice(const QAudioFormat &format,
                                                              const QAudioDevice &deviceInfo,
                                                              QObject *parent)
{
    QAudioDevice info = deviceInfo;
    if (info.isNull())
        info = audioInputs().value(0);

    QPlatformAudioSource* p = !info.isNull() ? createAudioSource(info, parent) : nullptr;
    if (p)
        p->setFormat(format);
    return p;
}

QPlatformAudioSink *QPlatformAudioDevices::audioOutputDevice(const QAudioFormat &format,
                                                             const QAudioDevice &deviceInfo,
                                                             QObject *parent)
{
    QAudioDevice info = deviceInfo;
    if (info.isNull())
        info = audioOutputs().value(0);

    QPlatformAudioSink* p = !info.isNull() ? createAudioSink(info, parent) : nullptr;
    if (p)
        p->setFormat(format);
    return p;
}

void QPlatformAudioDevices::prepareAudio() { }

QT_END_NAMESPACE

#include "moc_qplatformaudiodevices_p.cpp"
