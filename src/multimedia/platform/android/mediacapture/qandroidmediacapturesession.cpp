/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Ruslan Baratov
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
        if (m_cameraControl) {
            m_cameraControl->setCaptureSession(this);
            m_cameraControl->setActive(true);
        }

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
