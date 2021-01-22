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

#ifndef QALSAPLUGIN_H
#define QALSAPLUGIN_H

#include <QtMultimedia/qaudiosystemplugin.h>
#include <QtMultimedia/private/qaudiosystempluginext_p.h>

QT_BEGIN_NAMESPACE

class QAlsaPlugin : public QAudioSystemPlugin, public QAudioSystemPluginExtension
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.qt-project.qt.audiosystemfactory/5.0" FILE "alsa.json")
    Q_INTERFACES(QAudioSystemPluginExtension)

public:
    QAlsaPlugin(QObject *parent = 0);
    ~QAlsaPlugin() {}

    QByteArray defaultDevice(QAudio::Mode mode) const override;
    QList<QByteArray> availableDevices(QAudio::Mode mode) const override;
    QAbstractAudioInput *createInput(const QByteArray &device) override;
    QAbstractAudioOutput *createOutput(const QByteArray &device) override;
    QAbstractAudioDeviceInfo *createDeviceInfo(const QByteArray &device, QAudio::Mode mode) override;
};

QT_END_NAMESPACE

#endif // QALSAPLUGIN_H
