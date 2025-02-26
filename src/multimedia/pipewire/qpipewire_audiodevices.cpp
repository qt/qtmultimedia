// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiodevices_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_instance_p.h"
#include "qpipewire_audiosink_p.h"
#include "qpipewire_audiosource_p.h"

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

QAudioDevices::QAudioDevices()
{
    Q_ASSERT(isSupported());

    QAudioDeviceMonitor::DeviceLists deviceLists =
            QAudioContextManager::deviceMonitor().getDeviceLists();

    m_sourceDeviceList = std::move(deviceLists.sources);
    m_sinkDeviceList = std::move(deviceLists.sinks);

    connect(&QAudioContextManager::deviceMonitor(), &QAudioDeviceMonitor::audioSinksChanged, this,
            [this](QList<QAudioDevice> sinks) {
        m_sinkDeviceList = std::move(sinks);
        onAudioOutputsChanged();
    });

    connect(&QAudioContextManager::deviceMonitor(), &QAudioDeviceMonitor::audioSourcesChanged, this,
            [this](QList<QAudioDevice> sources) {
        m_sourceDeviceList = std::move(sources);
        onAudioInputsChanged();
    });
}

bool QAudioDevices::isSupported()
{
    QByteArray requestedBackend = qgetenv("QT_AUDIO_BACKEND");
    if (requestedBackend == "pipewire") {
        bool pipewireAudioAvailable = QPipeWireInstance::isLoaded()
                && QAudioContextManager::minimumRequirementMet()
                && QAudioContextManager::instance()->isConnected();

        if (!pipewireAudioAvailable) {
            qDebug() << "PipeWire audio backend requested. not available. Using default backend";
            return false;
        }
        return true;
    }

    return false;
}

QList<QAudioDevice> QAudioDevices::findAudioInputs() const
{
    return m_sourceDeviceList;
}

QList<QAudioDevice> QAudioDevices::findAudioOutputs() const
{
    return m_sinkDeviceList;
}

QPlatformAudioSource *QAudioDevices::createAudioSource(const QAudioDevice &device, const QAudioFormat &format,
                                                       QObject *parent)
{
    return new QPipewireAudioSource(device, format, parent);
}

QPlatformAudioSink *QAudioDevices::createAudioSink(const QAudioDevice &device, const QAudioFormat &format,
                                                   QObject *parent)
{
    return new QPipewireAudioSink(device, format, parent);
}

} // namespace QtPipeWire

QT_END_NAMESPACE
