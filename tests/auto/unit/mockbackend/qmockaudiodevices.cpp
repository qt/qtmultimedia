// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmockaudiodevices.h"
#include "private/qcameradevice_p.h"
#include "private/qaudiodevice_p.h"

QT_BEGIN_NAMESPACE

QMockAudioDevices::QMockAudioDevices()
    : QPlatformAudioDevices()
{
}

QMockAudioDevices::~QMockAudioDevices() = default;

void QMockAudioDevices::addAudioInput()
{
    QAudioDevicePrivate *devicePrivate = new QAudioDevicePrivate(
            QString::number(m_inputDevices.size()).toLatin1(), QAudioDevice::Input);
    m_inputDevices.push_back(devicePrivate->create());
    onAudioInputsChanged();
}

void QMockAudioDevices::addAudioOutput()
{
    QAudioDevicePrivate *devicePrivate = new QAudioDevicePrivate(
            QString::number(m_outputDevices.size()).toLatin1(), QAudioDevice::Output);
    m_outputDevices.push_back(devicePrivate->create());
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

QPlatformAudioSource *QMockAudioDevices::createAudioSource(const QAudioDevice &info,
                                                           QObject *parent)
{
    Q_UNUSED(info);
    Q_UNUSED(parent);
    return nullptr;// ###
}

QPlatformAudioSink *QMockAudioDevices::createAudioSink(const QAudioDevice &info,
                                                       QObject *parent)
{
    Q_UNUSED(info);
    Q_UNUSED(parent);
    return nullptr; //###
}


QT_END_NAMESPACE
