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

#ifndef QANDROIDAUDIOINPUTSELECTORCONTROL_H
#define QANDROIDAUDIOINPUTSELECTORCONTROL_H

#include <qaudioinputselectorcontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCaptureSession;

class QAndroidAudioInputSelectorControl : public QAudioInputSelectorControl
{
    Q_OBJECT
public:
    explicit QAndroidAudioInputSelectorControl(QAndroidCaptureSession *session);

    QList<QString> availableInputs() const;
    QString inputDescription(const QString& name) const;
    QString defaultInput() const;

    QString activeInput() const;
    void setActiveInput(const QString& name);

    static QList<QByteArray> availableDevices();
    static QString availableDeviceDescription(const QByteArray &device);

private:
    QAndroidCaptureSession *m_session;
};

QT_END_NAMESPACE

#endif // QANDROIDAUDIOINPUTSELECTORCONTROL_H
