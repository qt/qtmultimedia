// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohoscamera_p.h"

#include "qohoscamerasession_p.h"
#include "qohosmediacapturesession_p.h"

QT_BEGIN_NAMESPACE

QOhosCamera::QOhosCamera(QCamera *camera)
    : QPlatformCamera(camera), m_session(std::make_unique<QOhosCameraSession>(this))
{
    connect(m_session.get(), &QOhosCameraSession::activeChanged, this, &QOhosCamera::activeChanged);
}

QOhosCamera::~QOhosCamera() = default;

bool QOhosCamera::isActive() const
{
    return m_session->isActive();
}

void QOhosCamera::setActive(bool active)
{
    m_session->setActive(active);
}

void QOhosCamera::setCamera(const QCameraDevice &camera)
{
    m_session->setCamera(camera);
}

bool QOhosCamera::setCameraFormat(const QCameraFormat &format)
{
    m_cameraFormat = format;
    m_session->setCameraFormat(format);
    return true;
}

void QOhosCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    m_captureSession = static_cast<QOhosMediaCaptureSession *>(session);
    QVideoSink *sink = m_captureSession ? m_captureSession->videoSink() : nullptr;
    m_session->setVideoSink(sink);
}

QT_END_NAMESPACE

#include "moc_qohoscamera_p.cpp"
