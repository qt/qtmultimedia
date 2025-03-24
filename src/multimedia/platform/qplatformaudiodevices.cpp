// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformaudiodevices_p.h"

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qaudiosystem_p.h>

#if defined(Q_OS_ANDROID)
#  include <QtMultimedia/private/qandroidaudiodevices_p.h>
#endif
#if defined(Q_OS_DARWIN)
#  include <QtMultimedia/private/qdarwinaudiodevices_p.h>
#endif
#if defined(Q_OS_WINDOWS) && QT_CONFIG(wmf)
#  include <QtMultimedia/private/qwindowsaudiodevices_p.h>
#endif
#if QT_CONFIG(alsa)
#  include <QtMultimedia/private/qalsaaudiodevices_p.h>
#endif
#if QT_CONFIG(pulseaudio)
#  include <QtMultimedia/private/qpulseaudiodevices_p.h>
#endif
#if defined(Q_OS_QNX)
#  include <QtMultimedia/private/qqnxaudiodevices_p.h>
#endif
#if defined(Q_OS_WASM)
#  include <QtMultimedia/private/qwasmmediadevices_p.h>
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

QPlatformAudioSource *QPlatformAudioDevices::createAudioSource(const QAudioDevice &,
                                                               const QAudioFormat &, QObject *)
{
    return nullptr;
}
QPlatformAudioSink *QPlatformAudioDevices::createAudioSink(const QAudioDevice &,
                                                           const QAudioFormat &, QObject *)
{
    return nullptr;
}

QPlatformAudioSource *QPlatformAudioDevices::audioInputDevice(const QAudioFormat &format,
                                                              const QAudioDevice &deviceInfo,
                                                              QObject *parent)
{
    QAudioDevice device = deviceInfo;
    if (device.isNull())
        device = QMediaDevices::defaultAudioInput();

    if (device.isNull())
        return nullptr;

    return createAudioSource(device, format, parent);
}

QPlatformAudioSink *QPlatformAudioDevices::audioOutputDevice(const QAudioFormat &format,
                                                             const QAudioDevice &deviceInfo,
                                                             QObject *parent)
{
    QAudioDevice device = deviceInfo;
    if (device.isNull())
        device = QMediaDevices::defaultAudioOutput();

    if (device.isNull())
        return nullptr;

    return createAudioSink(device, format, parent);
}

QT_END_NAMESPACE

#include "moc_qplatformaudiodevices_p.cpp"
