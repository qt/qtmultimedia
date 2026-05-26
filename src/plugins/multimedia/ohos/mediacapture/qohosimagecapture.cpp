// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosimagecapture_p.h"

#include "qohoscamerasession_p.h"
#include "qohosmediacapturesession_p.h"

#include <QtCore/qdatetime.h>
#include <QtGui/qimage.h>
#include <QtMultimedia/qmediametadata.h>

QT_BEGIN_NAMESPACE

QOhosImageCapture::QOhosImageCapture(QImageCapture *parent) : QPlatformImageCapture(parent) { }

bool QOhosImageCapture::isReadyForCapture() const
{
    return m_session && m_session->isReadyForCapture();
}

int QOhosImageCapture::capture(const QString &fileName)
{
    if (!m_session) {
        emit error(-1, QImageCapture::ResourceError, msgImageCaptureNotSet());
        return -1;
    }
    return m_session->capture(fileName, /*toBuffer=*/false);
}

int QOhosImageCapture::captureToBuffer()
{
    if (!m_session) {
        emit error(-1, QImageCapture::ResourceError, msgImageCaptureNotSet());
        return -1;
    }
    return m_session->capture(QString(), /*toBuffer=*/true);
}

QImageEncoderSettings QOhosImageCapture::imageSettings() const
{
    if (m_session)
        return m_session->imageSettings();
    return m_pendingSettings;
}

void QOhosImageCapture::setImageSettings(const QImageEncoderSettings &settings)
{
    m_pendingSettings = settings;
    if (m_session)
        m_session->setImageSettings(settings);
}

void QOhosImageCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    auto *ohosSession = static_cast<QOhosMediaCaptureSession *>(session);
    QOhosCameraSession *newCameraSession = ohosSession ? ohosSession->cameraSession() : nullptr;
    if (m_service == ohosSession && m_session == newCameraSession)
        return;

    disconnectFromSession();
    m_service = ohosSession;
    m_session = newCameraSession;
    if (m_session) {
        m_session->setImageSettings(m_pendingSettings);
        connectToSession();
        notifyReadyForCaptureChanged(m_session->isReadyForCapture());
    } else {
        notifyReadyForCaptureChanged(false);
    }
}

void QOhosImageCapture::notifyReadyForCaptureChanged(bool ready)
{
    if (m_lastReady == ready)
        return;
    m_lastReady = ready;
    emit readyForCaptureChanged(ready);
}

void QOhosImageCapture::connectToSession()
{
    if (!m_session)
        return;
    connect(m_session, &QOhosCameraSession::readyForCaptureChanged, this,
            &QOhosImageCapture::notifyReadyForCaptureChanged);
    connect(m_session, &QOhosCameraSession::imageExposed, this, &QOhosImageCapture::imageExposed);
    connect(m_session, &QOhosCameraSession::imageCaptured, this,
            &QOhosImageCapture::onSessionImageCaptured);
    connect(m_session, &QOhosCameraSession::imageAvailable, this,
            &QOhosImageCapture::imageAvailable);
    connect(m_session, &QOhosCameraSession::imageSaved, this, &QOhosImageCapture::imageSaved);
    connect(m_session, &QOhosCameraSession::imageCaptureError, this, &QOhosImageCapture::error);
}

void QOhosImageCapture::onSessionImageCaptured(int id, const QImage &preview)
{
    emit imageCaptured(id, preview);

    QMediaMetaData md = metaData();
    md.insert(QMediaMetaData::Date, QDateTime::currentDateTime());
    if (!preview.isNull())
        md.insert(QMediaMetaData::Resolution, preview.size());
    emit imageMetadataAvailable(id, md);
}

void QOhosImageCapture::disconnectFromSession()
{
    if (m_session)
        disconnect(m_session, nullptr, this, nullptr);
}

QT_END_NAMESPACE

#include "moc_qohosimagecapture_p.cpp"
