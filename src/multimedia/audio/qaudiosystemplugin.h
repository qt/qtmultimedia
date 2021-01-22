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


#ifndef QAUDIOSYSTEMPLUGIN_H
#define QAUDIOSYSTEMPLUGIN_H

#include <QtCore/qstring.h>
#include <QtCore/qplugin.h>

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmultimedia.h>

#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qaudiodeviceinfo.h>
#include <QtMultimedia/qaudiosystem.h>

QT_BEGIN_NAMESPACE

struct Q_MULTIMEDIA_EXPORT QAudioSystemFactoryInterface
{
    virtual QList<QByteArray> availableDevices(QAudio::Mode) const = 0;
    virtual QAbstractAudioInput* createInput(const QByteArray& device) = 0;
    virtual QAbstractAudioOutput* createOutput(const QByteArray& device) = 0;
    virtual QAbstractAudioDeviceInfo* createDeviceInfo(const QByteArray& device, QAudio::Mode mode) = 0;
    virtual ~QAudioSystemFactoryInterface();
};

#define QAudioSystemFactoryInterface_iid \
    "org.qt-project.qt.audiosystemfactory/5.0"
Q_DECLARE_INTERFACE(QAudioSystemFactoryInterface, QAudioSystemFactoryInterface_iid)

class Q_MULTIMEDIA_EXPORT QAudioSystemPlugin : public QObject, public QAudioSystemFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QAudioSystemFactoryInterface)

public:
    explicit QAudioSystemPlugin(QObject *parent = nullptr);
    ~QAudioSystemPlugin();

    QList<QByteArray> availableDevices(QAudio::Mode) const override = 0;
    QAbstractAudioInput* createInput(const QByteArray& device) override = 0;
    QAbstractAudioOutput* createOutput(const QByteArray& device) override = 0;
    QAbstractAudioDeviceInfo* createDeviceInfo(const QByteArray& device, QAudio::Mode mode) override = 0;
};

QT_END_NAMESPACE

#endif // QAUDIOSYSTEMPLUGIN_H
