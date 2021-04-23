/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qwindowsmediacapture_p.h"

#include "qwindowsmediaencoder_p.h"
#include "qwindowscamera_p.h"
#include "qwindowscamerasession_p.h"
#include "qwindowscameraimagecapture_p.h"
#include "qmediadevicemanager.h"
#include "qaudiodeviceinfo.h"

QT_BEGIN_NAMESPACE

QWindowsMediaCaptureService::QWindowsMediaCaptureService()
{
    m_cameraSession = new QWindowsCameraSession(this);
    m_imageCapture = new QWindowsCameraImageCapture(m_cameraSession, this);
    m_recorder = new QWindowsMediaEncoder(m_cameraSession, this);
}

QWindowsMediaCaptureService::~QWindowsMediaCaptureService()
{
    delete m_recorder;
    delete m_imageCapture;
    delete m_cameraSession;
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
}

QPlatformCameraImageCapture *QWindowsMediaCaptureService::imageCapture()
{
    return m_imageCapture;
}

QPlatformMediaEncoder *QWindowsMediaCaptureService::mediaEncoder()
{
    return m_recorder;
}

bool QWindowsMediaCaptureService::isMuted() const
{
    return false;
}

void QWindowsMediaCaptureService::setMuted(bool muted)
{
}

qreal QWindowsMediaCaptureService::volume() const
{
    return 1.0;
}

void QWindowsMediaCaptureService::setVolume(qreal volume)
{
}

QAudioDeviceInfo QWindowsMediaCaptureService::audioInput() const
{
    return QMediaDeviceManager::defaultAudioInput();
}

bool QWindowsMediaCaptureService::setAudioInput(const QAudioDeviceInfo &info)
{
    return false;
}

void QWindowsMediaCaptureService::setVideoPreview(QVideoSink *sink)
{
    m_cameraSession->setVideoSink(sink);
}

QWindowsCameraSession *QWindowsMediaCaptureService::session() const
{
    return m_cameraSession;
}

QT_END_NAMESPACE
