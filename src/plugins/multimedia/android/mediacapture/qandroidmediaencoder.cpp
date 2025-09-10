// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidmediaencoder_p.h"
#include "qandroidmultimediautils_p.h"
#include "qandroidcapturesession_p.h"
#include "qandroidmediacapturesession_p.h"

QT_BEGIN_NAMESPACE

QAndroidMediaEncoder::QAndroidMediaEncoder(QMediaRecorder *parent)
    : QPlatformMediaRecorder(parent)
{
}

bool QAndroidMediaEncoder::isLocationWritable(const QUrl &location) const
{
    return location.isValid()
            && (location.isLocalFile() || location.isRelative());
}

QMediaRecorder::RecorderState QAndroidMediaEncoder::state() const
{
    return m_session ? m_session->state() : QMediaRecorder::StoppedState;
}

qint64 QAndroidMediaEncoder::duration() const
{
    return m_session ? m_session->duration() : 0;

}

void QAndroidMediaEncoder::record(QMediaEncoderSettings &settings)
{
    if (m_session)
        m_session->start(settings, outputLocation());
}

void QAndroidMediaEncoder::stop()
{
    if (m_session)
        m_session->stop();
}

void QAndroidMediaEncoder::setOutputLocation(const QUrl &location)
{
    if (location.isLocalFile()) {
        qt_androidRequestWriteStoragePermission();
    }
    QPlatformMediaRecorder::setOutputLocation(location);
}

void QAndroidMediaEncoder::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QAndroidMediaCaptureSession *captureSession = static_cast<QAndroidMediaCaptureSession *>(session);
    if (m_service == captureSession)
        return;

    if (m_service)
        stop();
    if (m_session)
        m_session->setMediaEncoder(nullptr);

    m_service = captureSession;
    if (!m_service)
        return;
    m_session = m_service->captureSession();
    Q_ASSERT(m_session);
    m_session->setMediaEncoder(this);
}

QT_END_NAMESPACE
