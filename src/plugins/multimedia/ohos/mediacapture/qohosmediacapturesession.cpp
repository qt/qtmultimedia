// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosmediacapturesession_p.h"

#include "qohoscamera_p.h"
#include "qohoscamerasession_p.h"
#include "qohosimagecapture_p.h"
#include "qohosmediarecorder_p.h"

QT_BEGIN_NAMESPACE

QOhosMediaCaptureSession::QOhosMediaCaptureSession() = default;

QOhosMediaCaptureSession::~QOhosMediaCaptureSession() = default;

QPlatformCamera *QOhosMediaCaptureSession::camera()
{
    return m_camera;
}

void QOhosMediaCaptureSession::setCamera(QPlatformCamera *camera)
{
    m_camera = static_cast<QOhosCamera *>(camera);
    if (m_camera)
        m_camera->setCaptureSession(this);
    if (m_imageCapture)
        m_imageCapture->setCaptureSession(this);
    if (m_recorder)
        m_recorder->setCaptureSession(this);
    emit cameraChanged();
}

QOhosCameraSession *QOhosMediaCaptureSession::cameraSession() const
{
    return m_camera ? m_camera->session() : nullptr;
}

QPlatformImageCapture *QOhosMediaCaptureSession::imageCapture()
{
    return m_imageCapture;
}

void QOhosMediaCaptureSession::setImageCapture(QPlatformImageCapture *imageCapture)
{
    if (m_imageCapture == imageCapture)
        return;
    if (m_imageCapture)
        m_imageCapture->setCaptureSession(nullptr);
    m_imageCapture = static_cast<QOhosImageCapture *>(imageCapture);
    if (m_imageCapture)
        m_imageCapture->setCaptureSession(this);
    emit imageCaptureChanged();
}

QPlatformMediaRecorder *QOhosMediaCaptureSession::mediaRecorder()
{
    return m_recorder;
}

void QOhosMediaCaptureSession::setMediaRecorder(QPlatformMediaRecorder *recorder)
{
    if (m_recorder == recorder)
        return;
    if (m_recorder)
        m_recorder->setCaptureSession(nullptr);
    m_recorder = static_cast<QOhosMediaRecorder *>(recorder);
    if (m_recorder)
        m_recorder->setCaptureSession(this);
    emit encoderChanged();
}

void QOhosMediaCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    m_audioInput = input;
}

void QOhosMediaCaptureSession::setAudioOutput(QPlatformAudioOutput *output)
{
    m_audioOutput = output;
}

void QOhosMediaCaptureSession::setVideoPreview(QVideoSink *sink)
{
    if (m_videoSink == sink)
        return;
    m_videoSink = sink;
    if (m_camera)
        m_camera->setCaptureSession(this);
}

QT_END_NAMESPACE

#include "moc_qohosmediacapturesession_p.cpp"
