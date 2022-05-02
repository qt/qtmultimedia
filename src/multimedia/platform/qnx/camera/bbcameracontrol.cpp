/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#include "bbcameracontrol_p.h"

#include "bbcamerasession_p.h"
#include <qcameradevice.h>

QT_BEGIN_NAMESPACE

BbCameraControl::BbCameraControl(BbCameraSession *session, QObject *parent)
    : QPlatformCamera(parent)
    , m_session(session)
{
    connect(m_session, SIGNAL(statusChanged(QCamera::Status)), this, SIGNAL(statusChanged(QCamera::Status)));
    connect(m_session, SIGNAL(stateChanged(QCamera::State)), this, SIGNAL(stateChanged(QCamera::State)));
    connect(m_session, SIGNAL(error(int,QString)), this, SIGNAL(error(int,QString)));
    connect(m_session, SIGNAL(captureModeChanged(QCamera::CaptureModes)), this, SIGNAL(captureModeChanged(QCamera::CaptureModes)));

    connect(m_session, SIGNAL(cameraOpened()), SLOT(cameraOpened()));
}

QCamera::State BbCameraControl::state() const
{
    return m_session->state();
}

void BbCameraControl::setState(QCamera::State state)
{
    m_session->setState(state);
}

QCamera::CaptureModes BbCameraControl::captureMode() const
{
    return m_session->captureMode();
}

void BbCameraControl::setCaptureMode(QCamera::CaptureModes mode)
{
    m_session->setCaptureMode(mode);
}

QCamera::Status BbCameraControl::status() const
{
    return m_session->status();
}

void BbCameraControl::setCamera(const QCameraDevice &camera)
{
    m_session->setDevice(camera.id());
}

bool BbCameraControl::isCaptureModeSupported(QCamera::CaptureModes mode) const
{
    return m_session->isCaptureModeSupported(mode);
}

void BbCameraControl::cameraOpened()
{
}

QT_END_NAMESPACE
