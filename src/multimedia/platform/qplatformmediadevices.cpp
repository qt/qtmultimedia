// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformmediadevices_p.h"
#include "qplatformmediaintegration_p.h"
#include "qmediadevices.h"
#include "qaudiodevice.h"
#include "qcameradevice.h"
#include "qaudiosystem_p.h"
#include "qaudiodevice.h"
#include "qplatformvideodevices_p.h"

#include <qmutex.h>
#include <qloggingcategory.h>

#if defined(Q_OS_ANDROID)
#include <qandroidmediadevices_p.h>
#elif defined(Q_OS_DARWIN)
#include <qdarwinmediadevices_p.h>
#elif defined(Q_OS_WINDOWS)
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

namespace {
struct DevicesHolder {
    ~DevicesHolder()
    {
        QMutexLocker locker(&mutex);
        delete nativeInstance;
        nativeInstance = nullptr;
        instance = nullptr;
    }
    QBasicMutex mutex;
    QPlatformMediaDevices *instance = nullptr;
    QPlatformMediaDevices *nativeInstance = nullptr;
} devicesHolder;

}

QPlatformMediaDevices *QPlatformMediaDevices::instance()
{
    QMutexLocker locker(&devicesHolder.mutex);
    if (devicesHolder.instance)
        return devicesHolder.instance;

#ifdef Q_OS_DARWIN
    devicesHolder.nativeInstance = new QDarwinMediaDevices;
#elif defined(Q_OS_WINDOWS)
    devicesHolder.nativeInstance = new QWindowsMediaDevices;
#elif defined(Q_OS_ANDROID)
    devicesHolder.nativeInstance = new QAndroidMediaDevices;
#elif QT_CONFIG(alsa)
    devicesHolder.nativeInstance = new QAlsaMediaDevices;
#elif QT_CONFIG(pulseaudio)
    devicesHolder.nativeInstance = new QPulseAudioMediaDevices;
#elif defined(Q_OS_QNX)
    devicesHolder.nativeInstance = new QQnxMediaDevices;
#elif defined(Q_OS_WASM)
    devicesHolder.nativeInstance = new QWasmMediaDevices;
#else
    devicesHolder.nativeInstance = new QPlatformMediaDevices;
#endif

    devicesHolder.instance = devicesHolder.nativeInstance;
    return devicesHolder.instance;
}


QPlatformMediaDevices::QPlatformMediaDevices() = default;

void QPlatformMediaDevices::initVideoDevicesConnection() {
    std::call_once(m_videoDevicesConnectionFlag, [this]() {
        QMetaObject::invokeMethod(this, [this]() {
            auto videoDevices = QPlatformMediaIntegration::instance()->videoDevices();
            if (videoDevices)
                connect(videoDevices, &QPlatformVideoDevices::videoInputsChanged, this,
                        &QPlatformMediaDevices::videoInputsChanged);
        }, Qt::QueuedConnection);
    });
}

void QPlatformMediaDevices::setDevices(QPlatformMediaDevices *devices)
{
    devicesHolder.instance = devices;
}

QPlatformMediaDevices::~QPlatformMediaDevices() = default;

QList<QAudioDevice> QPlatformMediaDevices::audioInputs() const
{
    return {};
}

QList<QAudioDevice> QPlatformMediaDevices::audioOutputs() const
{
    return {};
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
