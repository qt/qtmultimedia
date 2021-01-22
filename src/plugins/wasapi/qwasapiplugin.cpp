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

#include "qwasapiplugin.h"
#include "qwasapiaudiodeviceinfo.h"
#include "qwasapiaudioinput.h"
#include "qwasapiaudiooutput.h"
#include "qwasapiutils.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcMmPlugin, "qt.multimedia.plugin")

QWasapiPlugin::QWasapiPlugin(QObject *parent)
    : QAudioSystemPlugin(parent)
{
    qCDebug(lcMmPlugin) << __FUNCTION__;
}

QByteArray QWasapiPlugin::defaultDevice(QAudio::Mode mode) const
{
    return QWasapiUtils::defaultDevice(mode);
}

QList<QByteArray> QWasapiPlugin::availableDevices(QAudio::Mode mode) const
{
    qCDebug(lcMmPlugin) << __FUNCTION__ << mode;
    return QWasapiUtils::availableDevices(mode);
}

QAbstractAudioInput *QWasapiPlugin::createInput(const QByteArray &device)
{
    qCDebug(lcMmPlugin) << __FUNCTION__ << device;
    return new QWasapiAudioInput(device);
}

QAbstractAudioOutput *QWasapiPlugin::createOutput(const QByteArray &device)
{
    qCDebug(lcMmPlugin) << __FUNCTION__ << device;
    return new QWasapiAudioOutput(device);
}

QAbstractAudioDeviceInfo *QWasapiPlugin::createDeviceInfo(const QByteArray &device, QAudio::Mode mode)
{
    qCDebug(lcMmPlugin) << __FUNCTION__ << device << mode;
    return new QWasapiAudioDeviceInfo(device, mode);
}

QT_END_NAMESPACE
