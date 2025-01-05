// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmockaudiodevices.h"
#include "private/qcameradevice_p.h"

QT_BEGIN_NAMESPACE

QMockAudioDevices::QMockAudioDevices()
    : QPlatformAudioDevices()
{
}

QMockAudioDevices::~QMockAudioDevices() = default;

QList<QAudioDevice> QMockAudioDevices::findAudioInputs() const
{
    return m_inputDevices;
}

QList<QAudioDevice> QMockAudioDevices::findAudioOutputs() const
{
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
