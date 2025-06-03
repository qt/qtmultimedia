// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpulseaudiodevices_p.h"
#include "qmediadevices.h"
#include "private/qcameradevice_p.h"

#include "qpulseaudiosource_p.h"
#include "qpulseaudiosink_p.h"
#include "qpulseaudio_contextmanager_p.h"

QT_BEGIN_NAMESPACE

QPulseAudioDevices::QPulseAudioDevices()
{
    pulseEngine = new QPulseAudioContextManager();

    // TODO: it might make sense to connect device changing signals
    // to each added QMediaDevices
    QObject::connect(pulseEngine, &QPulseAudioContextManager::audioInputsChanged, this,
                     &QPulseAudioDevices::onAudioInputsChanged, Qt::DirectConnection);
    QObject::connect(pulseEngine, &QPulseAudioContextManager::audioOutputsChanged, this,
                     &QPulseAudioDevices::onAudioOutputsChanged, Qt::DirectConnection);
}

QPulseAudioDevices::~QPulseAudioDevices()
{
    delete pulseEngine;
}

QList<QAudioDevice> QPulseAudioDevices::findAudioInputs() const
{
    return pulseEngine->availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QPulseAudioDevices::findAudioOutputs() const
{
    return pulseEngine->availableDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QPulseAudioDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                            const QAudioFormat &fmt,
                                                            QObject *parent)
{
    return new QPulseAudioInternal::QPulseAudioSource(deviceInfo, fmt, parent);
}

QPlatformAudioSink *QPulseAudioDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                        const QAudioFormat &fmt,
                                                        QObject *parent)
{
    return new QPulseAudioInternal::QPulseAudioSink(deviceInfo, fmt, parent);
}

QT_END_NAMESPACE
