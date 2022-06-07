// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediacapturesession_p.h"

#include "private/qplatformaudioinput_p.h"
#include "private/qplatformaudiooutput_p.h"
#include "qffmpegimagecapture_p.h"
#include "qffmpegmediarecorder_p.h"
#include "private/qplatformcamera_p.h"
#include "qvideosink.h"

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcMediaCapture, "qt.multimedia.capture")



QFFmpegMediaCaptureSession::QFFmpegMediaCaptureSession()
{
}

QFFmpegMediaCaptureSession::~QFFmpegMediaCaptureSession()
{
}

QPlatformCamera *QFFmpegMediaCaptureSession::camera()
{
    return m_camera;
}

void QFFmpegMediaCaptureSession::setCamera(QPlatformCamera *camera)
{
    if (m_camera == camera)
        return;
    if (m_camera) {
        m_camera->disconnect(this);
        m_camera->setCaptureSession(nullptr);
    }

    m_camera = camera;

    if (m_camera) {
        connect(m_camera, &QPlatformCamera::newVideoFrame, this, &QFFmpegMediaCaptureSession::newVideoFrame);
        m_camera->setCaptureSession(this);
    }

    emit cameraChanged();
}

QPlatformImageCapture *QFFmpegMediaCaptureSession::imageCapture()
{
    return m_imageCapture;
}

void QFFmpegMediaCaptureSession::setImageCapture(QPlatformImageCapture *imageCapture)
{
    if (m_imageCapture == imageCapture)
        return;

    if (m_imageCapture)
        m_imageCapture->setCaptureSession(nullptr);

    m_imageCapture = static_cast<QFFmpegImageCapture *>(imageCapture);

    if (m_imageCapture)
        m_imageCapture->setCaptureSession(this);

    emit imageCaptureChanged();
}

void QFFmpegMediaCaptureSession::setMediaRecorder(QPlatformMediaRecorder *recorder)
{
    auto *r = static_cast<QFFmpegMediaRecorder *>(recorder);
    if (m_mediaRecorder == r)
        return;

    if (m_mediaRecorder)
        m_mediaRecorder->setCaptureSession(nullptr);
    m_mediaRecorder = r;
    if (m_mediaRecorder)
        m_mediaRecorder->setCaptureSession(this);

    emit encoderChanged();
}

QPlatformMediaRecorder *QFFmpegMediaCaptureSession::mediaRecorder()
{
    return m_mediaRecorder;
}

void QFFmpegMediaCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    if (m_audioInput == input)
        return;

    m_audioInput = input;
}

void QFFmpegMediaCaptureSession::setVideoPreview(QVideoSink *sink)
{
    if (m_videoSink == sink)
        return;

    m_videoSink = sink;
}

void QFFmpegMediaCaptureSession::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;

    m_audioOutput = output;
}

void QFFmpegMediaCaptureSession::newVideoFrame(const QVideoFrame &frame)
{
    if (m_videoSink)
        m_videoSink->setVideoFrame(frame);
}


QT_END_NAMESPACE
