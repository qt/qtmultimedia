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
#include "avfcameracontrol_p.h"
#include "avfcamerasession_p.h"
#include "avfimagecapturecontrol_p.h"
#include "avfcamerarenderercontrol_p.h"
#include "avfimagecapturecontrol_p.h"
#include "avfcamerafocuscontrol_p.h"
#include "avfcameraexposurecontrol_p.h"
#include "avfcameraimageprocessingcontrol_p.h"
#include "avfcamerawindowcontrol_p.h"
#include "avfmediarecordercontrol_p.h"
#include <qmediadevicemanager.h>

QT_USE_NAMESPACE

AVFCameraService::AVFCameraService()
{
    m_session = new AVFCameraSession(this);
    m_cameraControl = new AVFCameraControl(this);

    m_recorderControl = new AVFMediaRecorderControl(this);

    m_imageCaptureControl = new AVFImageCaptureControl(this);
    m_cameraFocusControl = new AVFCameraFocusControl(this);
    m_cameraImageProcessingControl = new AVFCameraImageProcessingControl(this);
    m_cameraExposureControl = nullptr;
#ifdef Q_OS_IOS
    m_cameraExposureControl = new AVFCameraExposureControl(this);
#endif

    m_audioCaptureDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
}

AVFCameraService::~AVFCameraService()
{
#ifdef Q_OS_IOS
    delete m_recorderControl;
#endif

    //delete controls before session,
    //so they have a chance to do deinitialization
    delete m_imageCaptureControl;
    //delete m_recorderControl;
    delete m_cameraControl;
    delete m_cameraFocusControl;
    delete m_cameraExposureControl;
    delete m_cameraImageProcessingControl;

    delete m_session;
}

QPlatformCamera *AVFCameraService::cameraControl()
{
    return m_cameraControl;
}

QPlatformCameraImageCapture *AVFCameraService::imageCaptureControl()
{
    return m_imageCaptureControl;
}

QPlatformMediaEncoder *AVFCameraService::mediaEncoder()
{
    return m_recorderControl;
}

QPlatformCameraImageProcessing *AVFCameraService::cameraImageProcessingControl() const
{
    return m_cameraImageProcessingControl;
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
    const QList<QAudioDeviceInfo> devices = QMediaDeviceManager::audioInputs();
    for (auto d : devices)
        if (d.id() == id)
            return d;
    return QMediaDeviceManager::defaultAudioInput();
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

void AVFCameraService::setVideoPreview(QAbstractVideoSurface *surface)
{
    m_session->setVideoSurface(surface);
}

#include "moc_avfcameraservice_p.cpp"
