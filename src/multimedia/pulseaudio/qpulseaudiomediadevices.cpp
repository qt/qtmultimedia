// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpulseaudiomediadevices_p.h"
#include "qmediadevices.h"
#include "private/qcameradevice_p.h"

#include "qpulseaudiosource_p.h"
#include "qpulseaudiosink_p.h"
#include "qpulseaudiodevice_p.h"
#include "qaudioengine_pulse_p.h"

QT_BEGIN_NAMESPACE

QPulseAudioMediaDevices::QPulseAudioMediaDevices()
    : QPlatformMediaDevices()
{
    pulseEngine = new QPulseAudioEngine();

    // TODO: it might make sense to connect device changing signals
    // to each added QMediaDevices
    QObject::connect(pulseEngine, &QPulseAudioEngine::audioInputsChanged,
                     this, &QPulseAudioMediaDevices::audioInputsChanged, Qt::DirectConnection);
    QObject::connect(pulseEngine, &QPulseAudioEngine::audioOutputsChanged,
                     this, &QPulseAudioMediaDevices::audioOutputsChanged, Qt::DirectConnection);
}

QPulseAudioMediaDevices::~QPulseAudioMediaDevices()
{
    delete pulseEngine;
}

QList<QAudioDevice> QPulseAudioMediaDevices::audioInputs() const
{
    return pulseEngine->availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QPulseAudioMediaDevices::audioOutputs() const
{
    return pulseEngine->availableDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QPulseAudioMediaDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                                 QObject *parent)
{
    return new QPulseAudioSource(deviceInfo.id(), parent);
}

QPlatformAudioSink *QPulseAudioMediaDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                             QObject *parent)
{
    return new QPulseAudioSink(deviceInfo.id(), parent);
}

QT_END_NAMESPACE
