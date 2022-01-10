/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
