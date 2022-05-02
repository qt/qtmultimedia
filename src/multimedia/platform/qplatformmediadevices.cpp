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

#include "qplatformmediadevices_p.h"
#include "qmediadevices.h"
#include "qaudiodevice.h"
#include "qcameradevice.h"
#include "qaudiosystem_p.h"

QT_BEGIN_NAMESPACE

QPlatformMediaDevices::QPlatformMediaDevices() = default;

QPlatformMediaDevices::~QPlatformMediaDevices() = default;

QAudioDevice QPlatformMediaDevices::audioInput(const QByteArray &id) const
{
    const auto inputs = audioInputs();
    for (auto i : inputs) {
        if (i.id() == id)
            return i;
    }
    return {};
}

QAudioDevice QPlatformMediaDevices::audioOutput(const QByteArray &id) const
{
    const auto outputs = audioOutputs();
    for (auto o : outputs) {
        if (o.id() == id)
            return o;
    }
    return {};
}

QCameraDevice QPlatformMediaDevices::videoInput(const QByteArray &id) const
{
    const auto inputs = videoInputs();
    for (auto i : inputs) {
        if (i.id() == id)
            return i;
    }
    return QCameraDevice();
}

QPlatformAudioSource* QPlatformMediaDevices::audioInputDevice(const QAudioFormat &format, const QAudioDevice &deviceInfo)
{
    QAudioDevice info = deviceInfo;
    if (info.isNull())
        info = audioInputs().value(0);

    QPlatformAudioSource* p = !info.isNull() ? createAudioSource(info) : nullptr;
    if (p)
        p->setFormat(format);
    return p;
}

QPlatformAudioSink* QPlatformMediaDevices::audioOutputDevice(const QAudioFormat &format, const QAudioDevice &deviceInfo)
{
    QAudioDevice info = deviceInfo;
    if (info.isNull())
        info = audioOutputs().value(0);

    QPlatformAudioSink* p = !info.isNull() ? createAudioSink(info) : nullptr;
    if (p)
        p->setFormat(format);
    return p;
}

void QPlatformMediaDevices::audioInputsChanged() const
{
    for (auto m : m_devices)
        emit m->audioInputsChanged();
}

void QPlatformMediaDevices::audioOutputsChanged() const
{
    for (auto m : m_devices)
        emit m->audioOutputsChanged();
}

void QPlatformMediaDevices::videoInputsChanged() const
{
    for (auto m : m_devices)
        emit m->videoInputsChanged();
}


QT_END_NAMESPACE
