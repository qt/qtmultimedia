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

#ifndef QMEDIADEVICES_H
#define QMEDIADEVICES_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QAudioDevice;
class QCameraDevice;

class QMediaDevicesPrivate;

class Q_MULTIMEDIA_EXPORT QMediaDevices : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QAudioDevice> audioInputs READ audioInputs NOTIFY audioInputsChanged)
    Q_PROPERTY(QList<QAudioDevice> audioOutputs READ audioOutputs NOTIFY audioOutputsChanged)
    Q_PROPERTY(QList<QCameraDevice> videoInputs READ videoInputs NOTIFY videoInputsChanged)
    Q_PROPERTY(QAudioDevice defaultAudioInput READ defaultAudioInput NOTIFY audioInputsChanged)
    Q_PROPERTY(QAudioDevice defaultAudioOutput READ defaultAudioOutput NOTIFY audioOutputsChanged)
    Q_PROPERTY(QCameraDevice defaultVideoInput READ defaultVideoInput NOTIFY videoInputsChanged)

public:
    QMediaDevices(QObject *parent = nullptr);
    ~QMediaDevices();

    static QList<QAudioDevice> audioInputs();
    static QList<QAudioDevice> audioOutputs();
    static QList<QCameraDevice> videoInputs();

    static QAudioDevice defaultAudioInput();
    static QAudioDevice defaultAudioOutput();
    static QCameraDevice defaultVideoInput();

Q_SIGNALS:
    void audioInputsChanged();
    void audioOutputsChanged();
    void videoInputsChanged();

private:
    friend class QMediaDevicesPrivate;
};

QT_END_NAMESPACE


#endif  // QABSTRACTMEDIASERVICE_H

