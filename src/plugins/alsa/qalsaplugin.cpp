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

#include "qalsaplugin.h"
#include "qalsaaudiodeviceinfo.h"
#include "qalsaaudioinput.h"
#include "qalsaaudiooutput.h"

QT_BEGIN_NAMESPACE

QAlsaPlugin::QAlsaPlugin(QObject *parent)
    : QAudioSystemPlugin(parent)
{
}

QByteArray QAlsaPlugin::defaultDevice(QAudio::Mode mode) const
{
    return QAlsaAudioDeviceInfo::defaultDevice(mode);
}

QList<QByteArray> QAlsaPlugin::availableDevices(QAudio::Mode mode) const
{
    return QAlsaAudioDeviceInfo::availableDevices(mode);
}

QAbstractAudioInput *QAlsaPlugin::createInput(const QByteArray &device)
{
    return new QAlsaAudioInput(device);
}

QAbstractAudioOutput *QAlsaPlugin::createOutput(const QByteArray &device)
{
    return new QAlsaAudioOutput(device);
}

QAbstractAudioDeviceInfo *QAlsaPlugin::createDeviceInfo(const QByteArray &device, QAudio::Mode mode)
{
    return new QAlsaAudioDeviceInfo(device, mode);
}

QT_END_NAMESPACE
