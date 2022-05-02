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

#include "qwindowscamera_p.h"

#include "qwindowsmediadevicesession_p.h"
#include "qwindowsmediacapture_p.h"
#include <qcameradevice.h>

QT_BEGIN_NAMESPACE

QWindowsCamera::QWindowsCamera(QCamera *camera)
    : QPlatformCamera(camera)
{
}

QWindowsCamera::~QWindowsCamera() = default;

bool QWindowsCamera::isActive() const
{
    return m_active;
}

void QWindowsCamera::setActive(bool active)
{
    if (m_active == active)
        return;
    if (m_cameraDevice.isNull() && active)
        return;
    m_active = active;
    if (m_mediaDeviceSession)
        m_mediaDeviceSession->setActive(active);

    emit activeChanged(m_active);
}

void QWindowsCamera::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;
    m_cameraDevice = camera;
    if (m_mediaDeviceSession)
        m_mediaDeviceSession->setActiveCamera(camera);
}

void QWindowsCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QWindowsMediaCaptureService *captureService = static_cast<QWindowsMediaCaptureService *>(session);
    if (m_captureService == captureService)
        return;

    if (m_mediaDeviceSession) {
        m_mediaDeviceSession->disconnect(this);
        m_mediaDeviceSession->setActive(false);
        m_mediaDeviceSession->setCameraFormat({});
        m_mediaDeviceSession->setActiveCamera({});
    }

    m_captureService = captureService;
    if (!m_captureService) {
        m_mediaDeviceSession = nullptr;
        return;
    }

    m_mediaDeviceSession = m_captureService->session();
    Q_ASSERT(m_mediaDeviceSession);

    m_mediaDeviceSession->setActive(false);
    m_mediaDeviceSession->setActiveCamera(m_cameraDevice);
    m_mediaDeviceSession->setCameraFormat(m_cameraFormat);
    m_mediaDeviceSession->setActive(m_active);

    connect(m_mediaDeviceSession, &QWindowsMediaDeviceSession::activeChanged,
            this, &QWindowsCamera::onActiveChanged);
}

bool QWindowsCamera::setCameraFormat(const QCameraFormat &format)
{
    if (!format.isNull() && !m_cameraDevice.videoFormats().contains(format))
        return false;

    m_cameraFormat = format.isNull() ? findBestCameraFormat(m_cameraDevice) : format;

    if (m_mediaDeviceSession)
        m_mediaDeviceSession->setCameraFormat(m_cameraFormat);
    return true;
}

void QWindowsCamera::onActiveChanged(bool active)
{
    if (m_active == active)
        return;
    if (m_cameraDevice.isNull() && active)
        return;
    m_active = active;
    emit activeChanged(m_active);
}

QT_END_NAMESPACE
