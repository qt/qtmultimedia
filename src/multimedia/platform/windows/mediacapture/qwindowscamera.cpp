/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowscamera_p.h"

#include "qwindowscamerasession_p.h"
#include "qwindowsmediacapture_p.h"
#include <qcamerainfo.h>

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
    m_active = active;
    if (m_cameraSession)
        m_cameraSession->setActive(active);

    emit activeChanged(m_active);
    updateStatus();
}

QCamera::Status QWindowsCamera::status() const
{
    if (!m_cameraSession)
        return QCamera::InactiveStatus;

    if (m_active)
        return m_cameraSession->isActive() ? QCamera::ActiveStatus : QCamera::StartingStatus;

    return m_cameraSession->isActive() ? QCamera::StoppingStatus : QCamera::InactiveStatus;
}

void QWindowsCamera::updateStatus()
{
    QCamera::Status newStatus = status();

    if (m_lastStatus != newStatus) {
        m_lastStatus = newStatus;
        emit statusChanged(m_lastStatus);
    }
}

void QWindowsCamera::setCamera(const QCameraInfo &camera)
{
    if (m_cameraInfo == camera)
        return;
    m_cameraInfo = camera;
    if (m_cameraSession)
        m_cameraSession->setActiveCamera(camera);
}

void QWindowsCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QWindowsMediaCaptureService *captureService = static_cast<QWindowsMediaCaptureService *>(session);
    if (m_captureService == captureService)
        return;

    m_captureService = captureService;
    if (!m_captureService) {
        m_cameraSession = nullptr;
        return;
    }

    m_cameraSession = m_captureService->session();
    Q_ASSERT(m_cameraSession);
    connect(m_cameraSession, SIGNAL(activeChanged(bool)), SLOT(updateStatus()));

    m_cameraSession->setActiveCamera(QCameraInfo());
    m_cameraSession->setActive(m_active);
    m_cameraSession->setActiveCamera(m_cameraInfo);
}

QT_END_NAMESPACE
