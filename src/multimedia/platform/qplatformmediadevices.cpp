// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformmediadevices_p.h"
#include "qcameradevice.h"
#include "qaudiosystem_p.h"
#include "qaudiodevice.h"

#if defined(Q_OS_ANDROID)
#include <qandroidmediadevices_p.h>
#elif defined(Q_OS_DARWIN)
#include <qdarwinmediadevices_p.h>
#elif defined(Q_OS_WINDOWS) && QT_CONFIG(wmf)
#include <qwindowsmediadevices_p.h>
#elif QT_CONFIG(alsa)
#include <qalsamediadevices_p.h>
#elif QT_CONFIG(pulseaudio)
#include <qpulseaudiomediadevices_p.h>
#elif defined(Q_OS_QNX)
#include <qqnxmediadevices_p.h>
#elif defined(Q_OS_WASM)
#include <private/qwasmmediadevices_p.h>
#endif

QT_BEGIN_NAMESPACE

std::unique_ptr<QPlatformMediaDevices> QPlatformMediaDevices::create()
{
#ifdef Q_OS_DARWIN
    return std::make_unique<QDarwinMediaDevices>();
#elif defined(Q_OS_WINDOWS) && QT_CONFIG(wmf)
    return std::make_unique<QWindowsMediaDevices>();
#elif defined(Q_OS_ANDROID)
    return std::make_unique<QAndroidMediaDevices>();
#elif QT_CONFIG(alsa)
    return std::make_unique<QAlsaMediaDevices>();
#elif QT_CONFIG(pulseaudio)
    return std::make_unique<QPulseAudioMediaDevices>();
#elif defined(Q_OS_QNX)
    return std::make_unique<QQnxMediaDevices>();
#elif defined(Q_OS_WASM)
    return std::make_unique<QWasmMediaDevices>();
#else
    return std::make_unique<QPlatformMediaDevices>();
#endif
}

QPlatformMediaDevices::QPlatformMediaDevices()
{
    qRegisterMetaType<PrivateTag>(); // for the case of queued connections
}

QPlatformMediaDevices::~QPlatformMediaDevices() = default;

QList<QAudioDevice> QPlatformMediaDevices::audioInputs() const
{
    return m_audioInputs.ensure([this]() {
        return findAudioInputs();
    });
}

QList<QAudioDevice> QPlatformMediaDevices::audioOutputs() const
{
    return m_audioOutputs.ensure([this]() {
        return findAudioOutputs();
    });
}

void QPlatformMediaDevices::onAudioInputsChanged()
{
    m_audioInputs.reset();
    emit audioInputsChanged(PrivateTag{});
}

void QPlatformMediaDevices::onAudioOutputsChanged()
{
    m_audioOutputs.reset();
    emit audioOutputsChanged(PrivateTag{});
}

void QPlatformMediaDevices::updateAudioInputsCache()
{
    if (m_audioInputs.update(findAudioInputs()))
        emit audioInputsChanged(PrivateTag{});
}

void QPlatformMediaDevices::updateAudioOutputsCache()
{
    if (m_audioOutputs.update(findAudioOutputs()))
        emit audioOutputsChanged(PrivateTag{});
}

QPlatformAudioSource *QPlatformMediaDevices::createAudioSource(const QAudioDevice &, QObject *)
{
    return nullptr;
}
QPlatformAudioSink *QPlatformMediaDevices::createAudioSink(const QAudioDevice &, QObject *)
{
    return nullptr;
}

QPlatformAudioSource *QPlatformMediaDevices::audioInputDevice(const QAudioFormat &format,
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

QPlatformAudioSink *QPlatformMediaDevices::audioOutputDevice(const QAudioFormat &format,
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

void QPlatformMediaDevices::prepareAudio() { }

QT_END_NAMESPACE

#include "moc_qplatformmediadevices_p.cpp"
