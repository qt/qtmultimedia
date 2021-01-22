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


#ifndef QANDROIDCAMERACONTROL_H
#define QANDROIDCAMERACONTROL_H

#include <qcameracontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;

class QAndroidCameraControl : public QCameraControl
{
    Q_OBJECT
public:
    explicit QAndroidCameraControl(QAndroidCameraSession *session);
    virtual ~QAndroidCameraControl();

    QCamera::State state() const;
    void setState(QCamera::State state);

    QCamera::Status status() const;

    QCamera::CaptureModes captureMode() const;
    void setCaptureMode(QCamera::CaptureModes mode);
    bool isCaptureModeSupported(QCamera::CaptureModes mode) const;

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const;

private:
    QAndroidCameraSession *m_cameraSession;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERACONTROL_H
