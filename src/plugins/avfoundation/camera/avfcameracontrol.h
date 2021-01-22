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

#ifndef AVFCAMERACONTROL_H
#define AVFCAMERACONTROL_H

#include <QtCore/qobject.h>

#include <QtMultimedia/qcameracontrol.h>

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;

class AVFCameraControl : public QCameraControl
{
Q_OBJECT
public:
    AVFCameraControl(AVFCameraService *service, QObject *parent = nullptr);
    ~AVFCameraControl();

    QCamera::State state() const override;
    void setState(QCamera::State state) override;

    QCamera::Status status() const override;

    QCamera::CaptureModes captureMode() const override;
    void setCaptureMode(QCamera::CaptureModes) override;
    bool isCaptureModeSupported(QCamera::CaptureModes mode) const override;

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const override;

private Q_SLOTS:
    void updateStatus();

private:
    AVFCameraSession *m_session;

    QCamera::State m_state;
    QCamera::Status m_lastStatus;
    QCamera::CaptureModes m_captureMode;
};

QT_END_NAMESPACE

#endif
