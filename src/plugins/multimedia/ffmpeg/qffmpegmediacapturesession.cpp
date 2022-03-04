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
