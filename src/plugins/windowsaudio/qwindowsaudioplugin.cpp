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

#include "qwindowsaudioplugin.h"
#include "qwindowsaudiodeviceinfo.h"
#include "qwindowsaudioinput.h"
#include "qwindowsaudiooutput.h"

QT_BEGIN_NAMESPACE

QWindowsAudioPlugin::QWindowsAudioPlugin(QObject *parent)
    : QAudioSystemPlugin(parent)
{
}

QByteArray QWindowsAudioPlugin::defaultDevice(QAudio::Mode mode) const
{
    return QWindowsAudioDeviceInfo::defaultDevice(mode);
}

QList<QByteArray> QWindowsAudioPlugin::availableDevices(QAudio::Mode mode) const
{
    return QWindowsAudioDeviceInfo::availableDevices(mode);
}

QAbstractAudioInput *QWindowsAudioPlugin::createInput(const QByteArray &device)
{
    return new QWindowsAudioInput(device);
}

QAbstractAudioOutput *QWindowsAudioPlugin::createOutput(const QByteArray &device)
{
    return new QWindowsAudioOutput(device);
}

QAbstractAudioDeviceInfo *QWindowsAudioPlugin::createDeviceInfo(const QByteArray &device, QAudio::Mode mode)
{
    return new QWindowsAudioDeviceInfo(device, mode);
}

QT_END_NAMESPACE
