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

#include "qandroidcameracontrol.h"

#include "qandroidcamerasession.h"

QT_BEGIN_NAMESPACE

QAndroidCameraControl::QAndroidCameraControl(QAndroidCameraSession *session)
    : QCameraControl(0)
    , m_cameraSession(session)

{
    connect(m_cameraSession, SIGNAL(statusChanged(QCamera::Status)),
            this, SIGNAL(statusChanged(QCamera::Status)));

    connect(m_cameraSession, SIGNAL(stateChanged(QCamera::State)),
            this, SIGNAL(stateChanged(QCamera::State)));

    connect(m_cameraSession, SIGNAL(error(int,QString)), this, SIGNAL(error(int,QString)));

    connect(m_cameraSession, SIGNAL(captureModeChanged(QCamera::CaptureModes)),
            this, SIGNAL(captureModeChanged(QCamera::CaptureModes)));
}

QAndroidCameraControl::~QAndroidCameraControl()
{
}

QCamera::CaptureModes QAndroidCameraControl::captureMode() const
{
    return m_cameraSession->captureMode();
}

void QAndroidCameraControl::setCaptureMode(QCamera::CaptureModes mode)
{
    m_cameraSession->setCaptureMode(mode);
}

bool QAndroidCameraControl::isCaptureModeSupported(QCamera::CaptureModes mode) const
{
    return m_cameraSession->isCaptureModeSupported(mode);
}

void QAndroidCameraControl::setState(QCamera::State state)
{
    m_cameraSession->setState(state);
}

QCamera::State QAndroidCameraControl::state() const
{
    return m_cameraSession->state();
}

QCamera::Status QAndroidCameraControl::status() const
{
    return m_cameraSession->status();
}

bool QAndroidCameraControl::canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const
{
    Q_UNUSED(status);

    switch (changeType) {
    case QCameraControl::CaptureMode:
    case QCameraControl::ImageEncodingSettings:
    case QCameraControl::VideoEncodingSettings:
    case QCameraControl::Viewfinder:
        return true;
    default:
        return false;
    }
}

QT_END_NAMESPACE
