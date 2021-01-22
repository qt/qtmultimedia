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

#ifndef QOPENSLESPLUGIN_H
#define QOPENSLESPLUGIN_H

#include <QtMultimedia/qaudiosystemplugin.h>
#include <QtMultimedia/private/qaudiosystempluginext_p.h>

QT_BEGIN_NAMESPACE

class QOpenSLESEngine;

class QOpenSLESPlugin : public QAudioSystemPlugin, public QAudioSystemPluginExtension
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.qt-project.qt.audiosystemfactory/5.0" FILE "opensles.json")
    Q_INTERFACES(QAudioSystemPluginExtension)

public:
    QOpenSLESPlugin(QObject *parent = 0);
    ~QOpenSLESPlugin() {}

    QByteArray defaultDevice(QAudio::Mode mode) const;
    QList<QByteArray> availableDevices(QAudio::Mode mode) const;
    QAbstractAudioInput *createInput(const QByteArray &device);
    QAbstractAudioOutput *createOutput(const QByteArray &device);
    QAbstractAudioDeviceInfo *createDeviceInfo(const QByteArray &device, QAudio::Mode mode);

private:
    QOpenSLESEngine *m_engine;
};

QT_END_NAMESPACE

#endif // QOPENSLESPLUGIN_H
