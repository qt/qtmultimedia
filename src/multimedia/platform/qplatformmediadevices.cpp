// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformmediadevices_p.h"
#include "qmediadevices.h"
#include "qaudiodevice.h"
#include "qcameradevice.h"
#include "qaudiosystem_p.h"

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
struct Holder {
    ~Holder()
    {
        QMutexLocker locker(&mutex);
        delete nativeInstance;
        nativeInstance = nullptr;
        instance = nullptr;
    }
    QBasicMutex mutex;
    QPlatformMediaDevices *instance = nullptr;
    QPlatformMediaDevices *nativeInstance = nullptr;
} holder;

}

QPlatformMediaDevices *QPlatformMediaDevices::instance()
{
    QMutexLocker locker(&holder.mutex);
    if (holder.instance)
        return holder.instance;

#ifdef Q_OS_DARWIN
    holder.nativeInstance = new QDarwinMediaDevices;
#elif defined(Q_OS_WINDOWS)
    holder.nativeInstance = new QWindowsMediaDevices;
#elif defined(Q_OS_ANDROID)
    holder.nativeInstance = new QAndroidMediaDevices;
#elif QT_CONFIG(alsa)
    holder.nativeInstance = new QAlsaMediaDevices;
#elif QT_CONFIG(pulseaudio)
    holder.nativeInstance = new QPulseAudioMediaDevices;
#elif defined(Q_OS_QNX)
    holder.nativeInstance = new QQnxMediaDevices;
#elif defined(Q_OS_WASM)
    holder.nativeInstance = new QWasmMediaDevices;
#endif

    holder.instance = holder.nativeInstance;
    return holder.instance;
}


QPlatformMediaDevices::QPlatformMediaDevices()
{}

void QPlatformMediaDevices::setDevices(QPlatformMediaDevices *devices)
{
    holder.instance = devices;
}

QPlatformMediaDevices::~QPlatformMediaDevices() = default;

QList<QCameraDevice> QPlatformMediaDevices::videoInputs() const
{
    return {};
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

void QPlatformMediaDevices::audioInputsChanged() const
{
    const auto devices = allMediaDevices();
    for (auto m : devices)
        emit m->audioInputsChanged();
}

void QPlatformMediaDevices::audioOutputsChanged() const
{
    const auto devices = allMediaDevices();
    for (auto m : devices)
        emit m->audioOutputsChanged();
}

void QPlatformMediaDevices::videoInputsChanged() const
{
    const auto devices = allMediaDevices();
    for (auto m : devices)
        emit m->videoInputsChanged();
}


QT_END_NAMESPACE
