// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmockmediadevices.h"
#include "private/qcameradevice_p.h"

QT_BEGIN_NAMESPACE

QMockMediaDevices::QMockMediaDevices()
    : QPlatformMediaDevices()
{
}

QMockMediaDevices::~QMockMediaDevices() = default;

QList<QAudioDevice> QMockMediaDevices::audioInputs() const
{
    return m_inputDevices;
}

QList<QAudioDevice> QMockMediaDevices::audioOutputs() const
{
    return m_outputDevices;
}

QPlatformAudioSource *QMockMediaDevices::createAudioSource(const QAudioDevice &info,
                                                           QObject *parent)
{
    Q_UNUSED(info);
    Q_UNUSED(parent);
    return nullptr;// ###
}

QPlatformAudioSink *QMockMediaDevices::createAudioSink(const QAudioDevice &info,
                                                       QObject *parent)
{
    Q_UNUSED(info);
    Q_UNUSED(parent);
    return nullptr; //###
}


QT_END_NAMESPACE
