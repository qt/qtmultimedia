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

#include "qandroidcaptureservice_p.h"

#include "qandroidmediaencoder_p.h"
#include "qandroidcapturesession_p.h"
#include "qandroidcameracontrol_p.h"
#include "qandroidcamerasession_p.h"
#include "qandroidcameravideorenderercontrol_p.h"
#include "qandroidcameraimagecapturecontrol_p.h"
#include "qmediadevicemanager.h"
#include "qaudiodeviceinfo.h"

QT_BEGIN_NAMESPACE

QAndroidCaptureService::QAndroidCaptureService(QMediaRecorder::CaptureMode mode)
    : m_videoEnabled(mode == QMediaRecorder::AudioAndVideo)
{
    if (m_videoEnabled) {
        m_cameraSession = new QAndroidCameraSession;
        m_cameraControl = new QAndroidCameraControl(m_cameraSession);
        m_imageCaptureControl = new QAndroidCameraImageCaptureControl(m_cameraSession);
    } else {
        m_cameraSession = 0;
        m_cameraControl = 0;
        m_imageCaptureControl = 0;
    }

    m_captureSession = new QAndroidCaptureSession(m_cameraSession);
    m_recorderControl = new QAndroidMediaEncoder(m_captureSession);
}

QAndroidCaptureService::~QAndroidCaptureService()
{
    delete m_recorderControl;
    delete m_captureSession;
    delete m_cameraControl;
    delete m_imageCaptureControl;
    delete m_cameraSession;
}

QPlatformCamera *QAndroidCaptureService::cameraControl()
{
    return m_cameraControl;
}

QPlatformCameraImageCapture *QAndroidCaptureService::imageCaptureControl()
{
    return m_imageCaptureControl;
}

QPlatformMediaEncoder *QAndroidCaptureService::mediaEncoder()
{
    return m_recorderControl;
}


bool QAndroidCaptureService::isMuted() const
{
    // No API for this in Android
    return false;
}

void QAndroidCaptureService::setMuted(bool muted)
{
    // No API for this in Android
    Q_UNUSED(muted);
    qWarning("QMediaRecorder::setMuted() is not supported on Android.");
}

qreal QAndroidCaptureService::volume() const
{
    // No API for this in Android
    return 1.0;
}

void QAndroidCaptureService::setVolume(qreal volume)
{
    // No API for this in Android
    Q_UNUSED(volume);
    qWarning("QMediaRecorder::setVolume() is not supported on Android.");
}

QAudioDeviceInfo QAndroidCaptureService::audioInput() const
{
    QMediaDeviceManager *manager = QMediaDeviceManager::instance();
    const auto devices = manager->audioInputs();
    QByteArray id = m_captureSession->audioInput().toLatin1();

    for (auto c : devices) {
        if (c.id() == id)
            return c;
    }
    return manager->defaultAudioInput();
}

bool QAndroidCaptureService::setAudioInput(const QAudioDeviceInfo &info)
{
    m_captureSession->setAudioInput(QString::fromLatin1(info.id()));
    return true;
}

void QAndroidCaptureService::setVideoPreview(QAbstractVideoSurface *surface)
{
    m_captureSession->setVideoSurface(surface);
}

QT_END_NAMESPACE
