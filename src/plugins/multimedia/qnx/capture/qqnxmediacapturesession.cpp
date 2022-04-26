/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#include "qqnxmediacapturesession_p.h"

#include "qqnxaudioinput_p.h"
#include "qqnxcamera_p.h"
#include "qqnximagecapture_p.h"
#include "qqnxmediarecorder_p.h"
#include "qqnxvideosink_p.h"
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
    m_camera = static_cast<QQnxCamera *>(camera);
    emit cameraChanged();
}

QPlatformImageCapture *QQnxMediaCaptureSession::imageCapture()
{
    return m_imageCapture;
}

void QQnxMediaCaptureSession::setImageCapture(QPlatformImageCapture *imageCapture)
{
    if (m_imageCapture == imageCapture)
        return;
    m_imageCapture = static_cast<QQnxImageCapture *>(imageCapture);
    emit imageCaptureChanged();
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

    emit encoderChanged();
}

void QQnxMediaCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    if (m_audioInput == input)
        return;

    m_audioInput = static_cast<QQnxAudioInput*>(input);
}

void QQnxMediaCaptureSession::setVideoPreview(QVideoSink *sink)
{
    auto qnxSink = sink ? static_cast<QQnxVideoSink *>(sink->platformVideoSink()) : nullptr;
    if (m_videoSink == qnxSink)
        return;
    m_videoSink = qnxSink;
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

QQnxVideoSink * QQnxMediaCaptureSession::videoSink() const
{
    return m_videoSink;
}

QT_END_NAMESPACE
