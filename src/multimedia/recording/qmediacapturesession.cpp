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

#include "qmediacapturesession.h"
#include "qaudiodeviceinfo.h"
#include "qplatformmediaintegration_p.h"
#include "qcamera.h"
#include "qmediarecorder.h"
#include "qcameraimagecapture.h"

QT_BEGIN_NAMESPACE

class QMediaCaptureSessionPrivate
{
public:
    QPlatformMediaCaptureSession *captureSession;
    QCamera *camera;
    QAudioDeviceInfo audioInput;
    QCameraImageCapture *imageCapture;
    QMediaRecorder *recorder;
};


/*!
    Creates a session for media capture.
 */
QMediaCaptureSession::QMediaCaptureSession(QObject *parent)
    : QObject(parent),
    d_ptr(new QMediaCaptureSessionPrivate)
{
    d_ptr->captureSession = QPlatformMediaIntegration::instance()->createCaptureSession(QMediaRecorder::AudioAndVideo);
}

/*!
    Destroys the session.
 */
QMediaCaptureSession::~QMediaCaptureSession()
{
    delete d_ptr;
}

/*!
    Returns false if media capture is not supported.
 */
bool QMediaCaptureSession::isAvailable() const
{
    return d_ptr->captureSession != nullptr;
}

/*!
    Creates a session for media capture.
 */
QAudioDeviceInfo QMediaCaptureSession::audioInput() const
{
    return d_ptr->audioInput;
}

void QMediaCaptureSession::setAudioInput(const QAudioDeviceInfo &device)
{
    d_ptr->audioInput = device;
    emit audioInputChanged();
}

QCamera *QMediaCaptureSession::camera() const
{
    return d_ptr->camera;
}

void QMediaCaptureSession::setCamera(QCamera *camera)
{
    d_ptr->camera = camera;
    emit cameraChanged();
}

QCameraImageCapture *QMediaCaptureSession::imageCapture()
{
    return d_ptr->imageCapture;
}

void QMediaCaptureSession::setImageCapture(QCameraImageCapture *imageCapture)
{
    d_ptr->imageCapture = imageCapture;
    emit imageCaptureChanged();
}

QMediaRecorder *QMediaCaptureSession::recorder()
{
    return d_ptr->recorder;
}

void QMediaCaptureSession::setRecorder(QMediaRecorder *recorder)
{
    d_ptr->recorder = recorder;
    emit recorderChanged();
}

QPlatformMediaCaptureSession *QMediaCaptureSession::platformSession() const
{
    return d_ptr->captureSession;
}


QT_END_NAMESPACE

#include "moc_qmediacapturesession.cpp"
