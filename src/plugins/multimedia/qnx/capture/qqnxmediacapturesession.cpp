// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqnxmediacapturesession_p.h"

#include "common/qqnxaudioinput_p.h"
#include "camera/qqnxplatformcamera_p.h"
#include "camera/qqnximagecapture_p.h"
#include "qqnxmediarecorder_p.h"
#include <QtMultimedia/private/qplatformvideosink_p.h>
#include "qvideosink.h"

QT_BEGIN_NAMESPACE

QQnxMediaCaptureSession::QQnxMediaCaptureSession()
    : QPlatformMediaCaptureSession()
{
}

QQnxMediaCaptureSession::~QQnxMediaCaptureSession()
{
}

QPlatformCamera *QQnxMediaCaptureSession::camera()
{
    return m_camera;
}

void QQnxMediaCaptureSession::setCamera(QPlatformCamera *camera)
{
    if (camera == m_camera)
        return;

    if (m_camera)
        m_camera->setCaptureSession(nullptr);

    m_camera = static_cast<QQnxPlatformCamera *>(camera);

    if (m_camera)
        m_camera->setCaptureSession(this);

    Q_EMIT cameraChanged();
}

QPlatformImageCapture *QQnxMediaCaptureSession::imageCapture()
{
    return m_imageCapture;
}

void QQnxMediaCaptureSession::setImageCapture(QPlatformImageCapture *imageCapture)
{
    if (m_imageCapture == imageCapture)
        return;

    if (m_imageCapture)
        m_imageCapture->setCaptureSession(nullptr);

    m_imageCapture = static_cast<QQnxImageCapture *>(imageCapture);

    if (m_imageCapture)
        m_imageCapture->setCaptureSession(this);

    Q_EMIT imageCaptureChanged();
}

QPlatformMediaRecorder *QQnxMediaCaptureSession::mediaRecorder()
{
    return m_mediaRecorder;
}

void QQnxMediaCaptureSession::setMediaRecorder(QPlatformMediaRecorder *mediaRecorder)
{
    if (m_mediaRecorder == mediaRecorder)
        return;

    if (m_mediaRecorder)
        m_mediaRecorder->setCaptureSession(nullptr);

    m_mediaRecorder = static_cast<QQnxMediaRecorder *>(mediaRecorder);

    if (m_mediaRecorder)
        m_mediaRecorder->setCaptureSession(this);

    Q_EMIT encoderChanged();
}

void QQnxMediaCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    if (m_audioInput == input)
        return;

    m_audioInput = static_cast<QQnxAudioInput*>(input);
}

void QQnxMediaCaptureSession::setVideoPreview(QVideoSink *sink)
{
    m_videoSink = sink->platformVideoSink();
}

void QQnxMediaCaptureSession::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;
    m_audioOutput = output;
}

QQnxAudioInput * QQnxMediaCaptureSession::audioInput() const
{
    return m_audioInput;
}

QPlatformVideoSink *QQnxMediaCaptureSession::videoSink() const
{
    return m_videoSink;
}

QT_END_NAMESPACE

#include "moc_qqnxmediacapturesession_p.cpp"
