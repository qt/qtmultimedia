/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#include <qaudiodeviceinfo.h>

#include "qpulseaudioplugin.h"
#include "qaudiodeviceinfo_pulse.h"
#include "qaudiooutput_pulse.h"
#include "qaudioinput_pulse.h"
#include "qpulseaudioengine.h"

QT_BEGIN_NAMESPACE

QPulseAudioPlugin::QPulseAudioPlugin(QObject *parent)
    : QAudioSystemPlugin(parent)
    , m_pulseEngine(QPulseAudioEngine::instance())
{
}

QByteArray QPulseAudioPlugin::defaultDevice(QAudio::Mode mode) const
{
    return m_pulseEngine->defaultDevice(mode);
}

QList<QByteArray> QPulseAudioPlugin::availableDevices(QAudio::Mode mode) const
{
    return m_pulseEngine->availableDevices(mode);
}

QAbstractAudioInput *QPulseAudioPlugin::createInput(const QByteArray &device)
{
    QPulseAudioInput *input = new QPulseAudioInput(device);
    return input;
}

QAbstractAudioOutput *QPulseAudioPlugin::createOutput(const QByteArray &device)
{

    QPulseAudioOutput *output = new QPulseAudioOutput(device);
    return output;
}

QAbstractAudioDeviceInfo *QPulseAudioPlugin::createDeviceInfo(const QByteArray &device, QAudio::Mode mode)
{
    QPulseAudioDeviceInfo *deviceInfo = new QPulseAudioDeviceInfo(device, mode);
    return deviceInfo;
}

QT_END_NAMESPACE
