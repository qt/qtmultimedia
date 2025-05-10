// Copyright (C) 2022 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qavfcameradebug_p.h>
#include "avfcamera_p.h"
#include "avfcamerasession_p.h"
#include "avfcameraservice_p.h"
#include <QtMultimedia/private/qavfcamerautility_p.h>
#include "avfcamerarenderer_p.h"
#include <qmediacapturesession.h>

QT_USE_NAMESPACE

AVFCamera::AVFCamera(QCamera *camera)
   : QAVFCameraBase(camera)
{
    Q_ASSERT(camera);
}

AVFCamera::~AVFCamera() = default;

void AVFCamera::onActiveChanged(bool active)
{
    if (m_session)
        m_session->setActive(active);
}

void AVFCamera::onCameraDeviceChanged(const QCameraDevice &device)
{
    if (device.isNull() || !checkCameraPermission())
        return;

    if (m_session)
        m_session->setActiveCamera(m_cameraDevice);
}

bool AVFCamera::tryApplyCameraFormat(const QCameraFormat &newFormat)
{
    // TODO: In the future, we should be able to return false if we failed
    // to apply the format.
    if (m_session)
        m_session->setCameraFormat(newFormat);
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
