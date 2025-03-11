// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmockaudiodevices.h"
#include "private/qcameradevice_p.h"
#include "private/qaudiodevice_p.h"

QT_BEGIN_NAMESPACE

QMockAudioDevices::QMockAudioDevices() = default;

QMockAudioDevices::~QMockAudioDevices() = default;

void QMockAudioDevices::addAudioInput()
{
    auto devicePrivate =
            std::make_unique<QAudioDevicePrivate>(QString::number(m_inputDevices.size()).toLatin1(),
                                                  QAudioDevice::Input, "MockAudioInput");
    m_inputDevices.push_back(QAudioDevicePrivate::createQAudioDevice(std::move(devicePrivate)));
    onAudioInputsChanged();
}

void QMockAudioDevices::addAudioOutput()
{
    auto devicePrivate = std::make_unique<QAudioDevicePrivate>(
            QString::number(m_outputDevices.size()).toLatin1(), QAudioDevice::Output,
            "MockAudioOutput");
    m_outputDevices.push_back(QAudioDevicePrivate::createQAudioDevice(std::move(devicePrivate)));
    onAudioOutputsChanged();
}

QList<QAudioDevice> QMockAudioDevices::findAudioInputs() const
{
    ++m_findAudioInputsInvokeCount;
    return m_inputDevices;
}

QList<QAudioDevice> QMockAudioDevices::findAudioOutputs() const
{
    ++m_findAudioOutputsInvokeCount;
    return m_outputDevices;
}

QPlatformAudioSource *QMockAudioDevices::createAudioSource(const QAudioDevice &,
                                                           const QAudioFormat &, QObject *)
{
    return nullptr;
}

QPlatformAudioSink *QMockAudioDevices::createAudioSink(const QAudioDevice &, const QAudioFormat &,
                                                       QObject *)
{
    return nullptr;
}


QT_END_NAMESPACE
