/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

#include "avfcameraservice_p.h"
#include "avfcamera_p.h"
#include "avfcamerasession_p.h"
#include "avfcameraimagecapture_p.h"
#include "avfcamerarenderer_p.h"
#include "avfcameraimagecapture_p.h"
#include "avfcamerafocus_p.h"
#include "avfcameraexposure_p.h"
#include "avfcameraimageprocessing_p.h"
#include "avfmediaencoder_p.h"
#include <qmediadevices.h>

QT_USE_NAMESPACE

AVFCameraService::AVFCameraService()
{
    m_session = new AVFCameraSession(this);

    m_audioCaptureDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
}

AVFCameraService::~AVFCameraService()
{
    if (m_session)
        delete m_session;
}

QPlatformCamera *AVFCameraService::camera()
{
    return m_cameraControl;
}

void AVFCameraService::setCamera(QPlatformCamera *camera)
{
    AVFCamera *control = static_cast<AVFCamera *>(camera);
    if (m_cameraControl == control)
        return;

    if (m_cameraControl)
        m_cameraControl->setCaptureSession(nullptr);

    m_cameraControl = control;
    if (m_cameraControl)
        m_cameraControl->setCaptureSession(this);
    emit cameraChanged();
}

QPlatformCameraImageCapture *AVFCameraService::imageCapture()
{
    return m_imageCaptureControl;
}

void AVFCameraService::setImageCapture(QPlatformCameraImageCapture *imageCapture)
{
    AVFCameraImageCapture *control = static_cast<AVFCameraImageCapture *>(imageCapture);
    if (m_imageCaptureControl == control)
        return;

    if (m_imageCaptureControl)
        m_imageCaptureControl->setCaptureSession(nullptr);

    m_imageCaptureControl = control;
    if (m_imageCaptureControl)
        m_imageCaptureControl->setCaptureSession(this);
}

QPlatformMediaEncoder *AVFCameraService::mediaEncoder()
{
    return m_encoder;
}

void AVFCameraService::setMediaEncoder(QPlatformMediaEncoder *encoder)
{
    AVFMediaEncoder *control = static_cast<AVFMediaEncoder *>(encoder);
    if (m_encoder == control)
        return;

    if (m_encoder)
        m_encoder->setCaptureSession(nullptr);

    m_encoder = control;
    if (m_encoder)
        m_encoder->setCaptureSession(this);
    emit encoderChanged();
}

bool AVFCameraService::isMuted() const
{
    return m_muted;
}

void AVFCameraService::setMuted(bool muted)
{
    if (m_muted != muted) {
        m_muted = muted;
        Q_EMIT mutedChanged(muted);
    }
}

qreal AVFCameraService::volume() const
{
    return m_volume;
}

void AVFCameraService::setVolume(qreal volume)
{
    if (m_volume != volume) {
        m_volume = volume;
        Q_EMIT volumeChanged(volume);
    }
}

QAudioDeviceInfo AVFCameraService::audioInput() const
{
    QByteArray id = [[m_audioCaptureDevice uniqueID] UTF8String];
    const QList<QAudioDeviceInfo> devices = QMediaDevices::audioInputs();
    for (auto d : devices)
        if (d.id() == id)
            return d;
    return QMediaDevices::defaultAudioInput();
}

bool AVFCameraService::setAudioInput(const QAudioDeviceInfo &id)
{
    AVCaptureDevice *device = nullptr;

    if (!id.isNull()) {
        device = [AVCaptureDevice deviceWithUniqueID: [NSString stringWithUTF8String:id.id().constData()]];
    } else {
        device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
    }

    if (device) {
        m_audioCaptureDevice = device;
        return true;
    }
    return false;
}

void AVFCameraService::setVideoPreview(QVideoSink *sink)
{
    if (m_session)
        m_session->setVideoSink(sink);
}

#include "moc_avfcameraservice_p.cpp"
