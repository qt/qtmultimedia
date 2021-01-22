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

#ifndef AVFAUDIOINPUTSELECTORCONTROL_H
#define AVFAUDIOINPUTSELECTORCONTROL_H

#include <QtMultimedia/qaudioinputselectorcontrol.h>
#include <QtCore/qstringlist.h>

#import <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;

class AVFAudioInputSelectorControl : public QAudioInputSelectorControl
{
Q_OBJECT
public:
    AVFAudioInputSelectorControl(AVFCameraService *service, QObject *parent = nullptr);
    ~AVFAudioInputSelectorControl();

    QList<QString> availableInputs() const override;
    QString inputDescription(const QString &name) const override;
    QString defaultInput() const override;
    QString activeInput() const override;

public Q_SLOTS:
    void setActiveInput(const QString &name) override;

public:
    //device changed since the last createCaptureDevice()
    bool isDirty() const { return m_dirty; }
    AVCaptureDevice *createCaptureDevice();

private:
    QString m_activeInput;
    bool m_dirty;
    QString m_defaultDevice;
    QStringList m_devices;
    QMap<QString, QString> m_deviceDescriptions;
};

QT_END_NAMESPACE

#endif
