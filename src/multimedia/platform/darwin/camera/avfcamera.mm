/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfcameradebug_p.h"
#include "avfcamera_p.h"
#include "avfcamerasession_p.h"
#include "avfcameraservice_p.h"
#include "avfcamerautility_p.h"
#include "avfcamerarenderer_p.h"
#include "avfcameraexposure_p.h"
#include "avfcamerafocus_p.h"
#include "avfcameraimageprocessing_p.h"
#include <qmediacapturesession.h>

QT_USE_NAMESPACE

AVFCamera::AVFCamera(QCamera *camera)
   : QPlatformCamera(camera)
   , m_active(false)
   , m_lastStatus(QCamera::InactiveStatus)
{
    Q_ASSERT(camera);
    Q_ASSERT(camera->captureSession());

    m_service = static_cast<AVFCameraService *>(camera->captureSession()->platformSession());
    Q_ASSERT(m_service);

    m_session = m_service->session();
    Q_ASSERT(m_session);

    setCamera(camera->cameraInfo());

    m_cameraFocusControl = new AVFCameraFocus(this);
    m_cameraImageProcessingControl = new AVFCameraImageProcessing(this);
    m_cameraExposureControl = nullptr;
#ifdef Q_OS_IOS
    m_cameraExposureControl = new AVFCameraExposure(this);
#endif
    connect(m_session, SIGNAL(activeChanged(bool)), SLOT(updateStatus()));
}

AVFCamera::~AVFCamera()
{
    delete m_cameraFocusControl;
    delete m_cameraExposureControl;
    delete m_cameraImageProcessingControl;
}

bool AVFCamera::isActive() const
{
    return m_active;
}

void AVFCamera::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    m_session->setActive(active);

    Q_EMIT activeChanged(m_active);
    updateStatus();
}

QCamera::Status AVFCamera::status() const
{
    static QCamera::Status statusTable[2][2] = {
        { QCamera::InactiveStatus,    QCamera::StoppingStatus }, //Inactive state
        { QCamera::StartingStatus,  QCamera::ActiveStatus } //ActiveState
    };

    return statusTable[m_active ? 1 : 0][m_session->isActive() ? 1 : 0];
}

void AVFCamera::setCamera(const QCameraInfo &camera)
{
    if (m_cameraInfo == camera)
        return;
    m_cameraInfo = camera;
    m_session->setActiveCamera(camera);
}

void AVFCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    AVFCameraService *captureSession = static_cast<AVFCameraService *>(session);
    if (m_service == captureSession)
        return;

    if (m_session) {
        m_session->setActiveCamera(QCameraInfo());
        m_session = nullptr;
    }

    m_service = captureSession;
    if (m_service) {
        m_session = m_service->session();
        m_session->setActiveCamera(m_cameraInfo);
    }
    captureSessionChanged();
}

void AVFCamera::updateStatus()
{
    QCamera::Status newStatus = status();

    if (m_lastStatus != newStatus) {
        qDebugCamera() << "Camera status changed: " << m_lastStatus << " -> " << newStatus;
        m_lastStatus = newStatus;
        Q_EMIT statusChanged(m_lastStatus);
    }
}

AVCaptureConnection *AVFCamera::videoConnection() const
{
    if (!m_session->videoOutput() || !m_session->videoOutput()->videoDataOutput())
        return nil;

    return [m_session->videoOutput()->videoDataOutput() connectionWithMediaType:AVMediaTypeVideo];
}

AVCaptureDevice *AVFCamera::device() const
{
    AVCaptureDevice *device = nullptr;
    QByteArray deviceId = m_cameraInfo.id();
    if (!deviceId.isEmpty()) {
        device = [AVCaptureDevice deviceWithUniqueID:
                    [NSString stringWithUTF8String:
                        deviceId.constData()]];
    }
    return device;
}

QPlatformCameraFocus *AVFCamera::focusControl()
{
    return m_cameraFocusControl;
}

QPlatformCameraExposure *AVFCamera::exposureControl()
{
    return m_cameraExposureControl;
}

QPlatformCameraImageProcessing *AVFCamera::imageProcessingControl()
{
    return m_cameraImageProcessingControl;
}

void AVFCamera::captureSessionChanged()
{

}

#include "moc_avfcamera_p.cpp"
