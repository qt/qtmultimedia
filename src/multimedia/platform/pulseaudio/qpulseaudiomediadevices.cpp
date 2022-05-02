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

#include "qpulseaudiomediadevices_p.h"
#include "qmediadevices.h"
#include "qcameradevice_p.h"

#include "private/qpulseaudiosource_p.h"
#include "private/qpulseaudiosink_p.h"
#include "private/qpulseaudiodevice_p.h"
#include "private/qaudioengine_pulse_p.h"

QT_BEGIN_NAMESPACE

QPulseAudioMediaDevices::QPulseAudioMediaDevices(QPulseAudioEngine *engine)
    : QPlatformMediaDevices(),
      pulseEngine(engine)
{
}

QList<QAudioDevice> QPulseAudioMediaDevices::audioInputs() const
{
    return pulseEngine->availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QPulseAudioMediaDevices::audioOutputs() const
{
    return pulseEngine->availableDevices(QAudioDevice::Output);
}

QList<QCameraDevice> QPulseAudioMediaDevices::videoInputs() const
{
    return {};
}

QPlatformAudioSource *QPulseAudioMediaDevices::createAudioSource(const QAudioDevice &deviceInfo)
{
    return new QPulseAudioSource(deviceInfo.id());
}

QPlatformAudioSink *QPulseAudioMediaDevices::createAudioSink(const QAudioDevice &deviceInfo)
{
    return new QPulseAudioSink(deviceInfo.id());
}

QT_END_NAMESPACE
