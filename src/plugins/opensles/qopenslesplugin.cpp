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

#include "qopenslesplugin.h"

#include "qopenslesengine.h"
#include "qopenslesdeviceinfo.h"
#include "qopenslesaudioinput.h"
#include "qopenslesaudiooutput.h"

QT_BEGIN_NAMESPACE

QOpenSLESPlugin::QOpenSLESPlugin(QObject *parent)
    : QAudioSystemPlugin(parent)
    , m_engine(QOpenSLESEngine::instance())
{
}

QByteArray QOpenSLESPlugin::defaultDevice(QAudio::Mode mode) const
{
    return m_engine->defaultDevice(mode);
}

QList<QByteArray> QOpenSLESPlugin::availableDevices(QAudio::Mode mode) const
{
    return m_engine->availableDevices(mode);
}

QAbstractAudioInput *QOpenSLESPlugin::createInput(const QByteArray &device)
{
    return new QOpenSLESAudioInput(device);
}

QAbstractAudioOutput *QOpenSLESPlugin::createOutput(const QByteArray &device)
{
    return new QOpenSLESAudioOutput(device);
}

QAbstractAudioDeviceInfo *QOpenSLESPlugin::createDeviceInfo(const QByteArray &device, QAudio::Mode mode)
{
    return new QOpenSLESDeviceInfo(device, mode);
}

QT_END_NAMESPACE

