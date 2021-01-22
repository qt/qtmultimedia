/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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
#include "coreaudioplugin.h"

#include "coreaudiodeviceinfo.h"
#include "coreaudioinput.h"
#include "coreaudiooutput.h"

QT_BEGIN_NAMESPACE

CoreAudioPlugin::CoreAudioPlugin(QObject *parent)
    : QAudioSystemPlugin(parent)
{
}

QByteArray CoreAudioPlugin::defaultDevice(QAudio::Mode mode) const
{
    return CoreAudioDeviceInfo::defaultDevice(mode);
}

QList<QByteArray> CoreAudioPlugin::availableDevices(QAudio::Mode mode) const
{
    return CoreAudioDeviceInfo::availableDevices(mode);
}


QAbstractAudioInput *CoreAudioPlugin::createInput(const QByteArray &device)
{
    return new CoreAudioInput(device);
}


QAbstractAudioOutput *CoreAudioPlugin::createOutput(const QByteArray &device)
{
    return new CoreAudioOutput(device);
}


QAbstractAudioDeviceInfo *CoreAudioPlugin::createDeviceInfo(const QByteArray &device, QAudio::Mode mode)
{
    return new CoreAudioDeviceInfo(device, mode);
}

QT_END_NAMESPACE

#include "moc_coreaudioplugin.cpp"
