// Copyright (C) 2022 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfcameradebug_p.h"
#include "avfcamera_p.h"
#include "avfcamerasession_p.h"
#include "avfcameraservice_p.h"
#include "avfcamerautility_p.h"
#include "avfcamerarenderer_p.h"
#include <qmediacapturesession.h>

QT_USE_NAMESPACE

AVFCamera::AVFCamera(QCamera *camera)
   : QAVFCameraBase(camera)
{
    Q_ASSERT(camera);
}

AVFCamera::~AVFCamera()
{
}

void AVFCamera::setActive(bool active)
{
    if (m_active == active)
        return;
    if (m_cameraDevice.isNull() && active)
        return;

    m_active = active;
    if (m_session)
        m_session->setActive(active);

    if (active)
        updateCameraConfiguration();
    Q_EMIT activeChanged(m_active);
}

void AVFCamera::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;
    m_cameraDevice = camera;
    if (m_session)
        m_session->setActiveCamera(camera);
    setCameraFormat({});
}

bool AVFCamera::setCameraFormat(const QCameraFormat &format)
{
    if (!format.isNull() && !m_cameraDevice.videoFormats().contains(format))
        return false;

    m_cameraFormat = format.isNull() ? findBestCameraFormat(m_cameraDevice) : format;

    if (m_session)
        m_session->setCameraFormat(m_cameraFormat);

    return true;
}

void AVFCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    AVFCameraService *captureSession = static_cast<AVFCameraService *>(session);
    if (m_service == captureSession)
        return;

    if (m_session) {
        m_session->disconnect(this);
        m_session->setActiveCamera({});
        m_session->setCameraFormat({});
    }

    m_service = captureSession;
    if (!m_service) {
        m_session = nullptr;
        return;
    }

    m_session = m_service->session();
    Q_ASSERT(m_session);

    m_session->setActiveCamera(m_cameraDevice);
    m_session->setCameraFormat(m_cameraFormat);
    m_session->setActive(m_active);
}

#include "moc_avfcamera_p.cpp"
