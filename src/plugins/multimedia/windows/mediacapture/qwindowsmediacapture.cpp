// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsmediacapture_p.h"

#include "qwindowsmediaencoder_p.h"
#include "qwindowscamera_p.h"
#include "qwindowsmediadevicesession_p.h"
#include "qwindowsimagecapture_p.h"
#include "qmediadevices.h"
#include "qaudiodevice.h"
#include "private/qplatformaudioinput_p.h"
#include "private/qplatformaudiooutput_p.h"

QT_BEGIN_NAMESPACE

QWindowsMediaCaptureService::QWindowsMediaCaptureService()
{
    m_mediaDeviceSession = new QWindowsMediaDeviceSession(this);
}

QWindowsMediaCaptureService::~QWindowsMediaCaptureService()
{
    delete m_mediaDeviceSession;
}

QPlatformCamera *QWindowsMediaCaptureService::camera()
{
    return m_camera;
}

void QWindowsMediaCaptureService::setCamera(QPlatformCamera *camera)
{
    QWindowsCamera *control = static_cast<QWindowsCamera*>(camera);
    if (m_camera == control)
        return;

    if (m_camera)
        m_camera->setCaptureSession(nullptr);

    m_camera = control;
    if (m_camera)
        m_camera->setCaptureSession(this);
    emit cameraChanged();
}

QPlatformImageCapture *QWindowsMediaCaptureService::imageCapture()
{
    return m_imageCapture;
}

void QWindowsMediaCaptureService::setImageCapture(QPlatformImageCapture *imageCapture)
{
    QWindowsImageCapture *control = static_cast<QWindowsImageCapture *>(imageCapture);
    if (m_imageCapture == control)
        return;

    if (m_imageCapture)
        m_imageCapture->setCaptureSession(nullptr);

    m_imageCapture = control;
    if (m_imageCapture)
        m_imageCapture->setCaptureSession(this);
    emit imageCaptureChanged();
}

QPlatformMediaRecorder *QWindowsMediaCaptureService::mediaRecorder()
{
    return m_encoder;
}

void QWindowsMediaCaptureService::setMediaRecorder(QPlatformMediaRecorder *recorder)
{
    QWindowsMediaEncoder *control = static_cast<QWindowsMediaEncoder *>(recorder);
    if (m_encoder == control)
        return;

    if (m_encoder)
        m_encoder->setCaptureSession(nullptr);

    m_encoder = control;
    if (m_encoder)
        m_encoder->setCaptureSession(this);
    emit encoderChanged();
}

void QWindowsMediaCaptureService::setAudioInput(QPlatformAudioInput *input)
{
    m_mediaDeviceSession->setAudioInput(input ? input->q : nullptr);
}

void QWindowsMediaCaptureService::setAudioOutput(QPlatformAudioOutput *output)
{
    m_mediaDeviceSession->setAudioOutput(output ? output->q : nullptr);
}

void QWindowsMediaCaptureService::setVideoPreview(QVideoSink *sink)
{
    m_mediaDeviceSession->setVideoSink(sink);
}

QWindowsMediaDeviceSession *QWindowsMediaCaptureService::session() const
{
    return m_mediaDeviceSession;
}

QT_END_NAMESPACE

#include "moc_qwindowsmediacapture_p.cpp"
