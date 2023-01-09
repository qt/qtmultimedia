// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Ruslan Baratov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidmediacapturesession_p.h"

#include "qandroidmediaencoder_p.h"
#include "qandroidcapturesession_p.h"
#include "qandroidcamera_p.h"
#include "qandroidcamerasession_p.h"
#include "qandroidimagecapture_p.h"
#include "qmediadevices.h"
#include "qaudiodevice.h"

QT_BEGIN_NAMESPACE

QAndroidMediaCaptureSession::QAndroidMediaCaptureSession()
    : m_captureSession(new QAndroidCaptureSession())
    , m_cameraSession(new QAndroidCameraSession())
{
}

QAndroidMediaCaptureSession::~QAndroidMediaCaptureSession()
{
    delete m_captureSession;
    delete m_cameraSession;
}

QPlatformCamera *QAndroidMediaCaptureSession::camera()
{
    return m_cameraControl;
}

void QAndroidMediaCaptureSession::setCamera(QPlatformCamera *camera)
{
        if (camera) {
            m_captureSession->setCameraSession(m_cameraSession);
        } else {
            m_captureSession->setCameraSession(nullptr);
        }

        QAndroidCamera *control = static_cast<QAndroidCamera *>(camera);
        if (m_cameraControl == control)
            return;

        if (m_cameraControl)
            m_cameraControl->setCaptureSession(nullptr);

        m_cameraControl = control;
        if (m_cameraControl)
            m_cameraControl->setCaptureSession(this);

        emit cameraChanged();
}

QPlatformImageCapture *QAndroidMediaCaptureSession::imageCapture()
{
    return m_imageCaptureControl;
}

void QAndroidMediaCaptureSession::setImageCapture(QPlatformImageCapture *imageCapture)
{
    QAndroidImageCapture *control = static_cast<QAndroidImageCapture *>(imageCapture);
    if (m_imageCaptureControl == control)
        return;

    if (m_imageCaptureControl)
        m_imageCaptureControl->setCaptureSession(nullptr);

    m_imageCaptureControl = control;
    if (m_imageCaptureControl)
        m_imageCaptureControl->setCaptureSession(this);
}

QPlatformMediaRecorder *QAndroidMediaCaptureSession::mediaRecorder()
{
    return m_encoder;
}

void QAndroidMediaCaptureSession::setMediaRecorder(QPlatformMediaRecorder *recorder)
{
    QAndroidMediaEncoder *control = static_cast<QAndroidMediaEncoder *>(recorder);

    if (m_encoder == control)
        return;

    if (m_encoder)
        m_encoder->setCaptureSession(nullptr);

    m_encoder = control;
    if (m_encoder)
        m_encoder->setCaptureSession(this);

    emit encoderChanged();

}

void QAndroidMediaCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    m_captureSession->setAudioInput(input);
}

void QAndroidMediaCaptureSession::setAudioOutput(QPlatformAudioOutput *output)
{
    m_captureSession->setAudioOutput(output);
}

void QAndroidMediaCaptureSession::setVideoPreview(QVideoSink *sink)
{
    m_cameraSession->setVideoSink(sink);
}

QT_END_NAMESPACE

#include "moc_qandroidmediacapturesession_p.cpp"
