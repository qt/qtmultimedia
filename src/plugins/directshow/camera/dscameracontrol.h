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

#ifndef DSCAMERACONTROL_H
#define DSCAMERACONTROL_H

#include <QtCore/qobject.h>
#include <qcameracontrol.h>

QT_BEGIN_NAMESPACE

class DSCameraSession;


class DSCameraControl : public QCameraControl
{
    Q_OBJECT
public:
    DSCameraControl(QObject *parent = nullptr);
    ~DSCameraControl() override;

    QCamera::State state() const override { return m_state; }

    QCamera::CaptureModes captureMode() const override { return m_captureMode; }
    void setCaptureMode(QCamera::CaptureModes mode) override;

    void setState(QCamera::State state) override;

    QCamera::Status status() const override;
    bool isCaptureModeSupported(QCamera::CaptureModes mode) const override;
    bool canChangeProperty(PropertyChangeType, QCamera::Status) const override
    { return false; }

private:
    DSCameraSession *m_session;
    QCamera::State m_state = QCamera::UnloadedState;
    QCamera::CaptureModes m_captureMode = QCamera::CaptureStillImage;
};

QT_END_NAMESPACE

#endif


