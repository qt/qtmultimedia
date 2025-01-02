// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidimagecapture_p.h"

#include "qandroidcamerasession_p.h"
#include "qandroidmediacapturesession_p.h"

QT_BEGIN_NAMESPACE

QAndroidImageCapture::QAndroidImageCapture(QImageCapture *parent)
    : QPlatformImageCapture(parent)
{
}

bool QAndroidImageCapture::isReadyForCapture() const
{
    return m_session->isReadyForCapture();
}

int QAndroidImageCapture::capture(const QString &fileName)
{
    return m_session->capture(fileName);
}

int QAndroidImageCapture::captureToBuffer()
{
    return m_session->captureToBuffer();
}

QImageEncoderSettings QAndroidImageCapture::imageSettings() const
{
    return m_session->imageSettings();
}

void QAndroidImageCapture::setImageSettings(const QImageEncoderSettings &settings)
{
    m_session->setImageSettings(settings);
}

void QAndroidImageCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QAndroidMediaCaptureSession *captureSession = static_cast<QAndroidMediaCaptureSession *>(session);
    if (m_service == captureSession)
        return;

    m_service = captureSession;
    if (!m_service) {
        disconnect(m_session, nullptr, this, nullptr);
        return;
    }

    m_session = m_service->cameraSession();
    Q_ASSERT(m_session);

    connect(m_session, &QAndroidCameraSession::readyForCaptureChanged,
            this, &QAndroidImageCapture::readyForCaptureChanged);
    connect(m_session, &QAndroidCameraSession::imageExposed,
            this, &QAndroidImageCapture::imageExposed);
    connect(m_session, &QAndroidCameraSession::imageCaptured,
            this, &QAndroidImageCapture::imageCaptured);
    connect(m_session, &QAndroidCameraSession::imageMetadataAvailable,
            this, &QAndroidImageCapture::imageMetadataAvailable);
    connect(m_session, &QAndroidCameraSession::imageAvailable,
            this, &QAndroidImageCapture::imageAvailable);
    connect(m_session, &QAndroidCameraSession::imageSaved,
            this, &QAndroidImageCapture::imageSaved);
    connect(m_session, &QAndroidCameraSession::imageCaptureError,
            this, &QAndroidImageCapture::error);
}
QT_END_NAMESPACE

#include "moc_qandroidimagecapture_p.cpp"
