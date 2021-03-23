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

#include "avfcameradebug_p.h"
#include "avfcamera_p.h"
#include "avfcamerasession_p.h"
#include "avfcameraservice_p.h"
#include "avfcamerautility_p.h"
#include "avfcamerarenderer_p.h"
#include "avfcameraexposure_p.h"
#include "avfcamerafocus_p.h"
#include "avfcameraimageprocessing_p.h"
#include "qabstractvideosurface.h"

QT_USE_NAMESPACE

AVFCamera::AVFCamera(AVFCameraService *service, QObject *parent)
   : QPlatformCamera(parent)
   , m_session(service->session())
   , m_service(service)
   , m_active(false)
   , m_lastStatus(QCamera::InactiveStatus)
{
    m_cameraFocusControl = new AVFCameraFocus(m_service);
    m_cameraImageProcessingControl = new AVFCameraImageProcessing(m_service);
    m_cameraExposureControl = nullptr;
#ifdef Q_OS_IOS
    m_cameraExposureControl = new AVFCameraExposure(m_service);
#endif
    connect(m_session, SIGNAL(activeChanged(bool)), SLOT(updateStatus()));
}

AVFCamera::~AVFCamera()
{
    delete m_cameraFocusControl;
    delete m_cameraExposureControl;
    delete m_cameraImageProcessingControl;
}

bool AVFCamera::isActive() const
{
    return m_active;
}

void AVFCamera::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    m_session->setActive(active);

    Q_EMIT activeChanged(m_active);
    updateStatus();
}

QCamera::Status AVFCamera::status() const
{
    static QCamera::Status statusTable[2][2] = {
        { QCamera::InactiveStatus,    QCamera::StoppingStatus }, //Inactive state
        { QCamera::StartingStatus,  QCamera::ActiveStatus } //ActiveState
    };

    return statusTable[m_active ? 1 : 0][m_session->isActive() ? 1 : 0];
}

void AVFCamera::setCamera(const QCameraInfo &camera)
{
    m_session->setActiveCamera(camera);
}

void AVFCamera::updateStatus()
{
    QCamera::Status newStatus = status();

    if (m_lastStatus != newStatus) {
        qDebugCamera() << "Camera status changed: " << m_lastStatus << " -> " << newStatus;
        m_lastStatus = newStatus;
        Q_EMIT statusChanged(m_lastStatus);
    }
}

QVideoSurfaceFormat::PixelFormat AVFCamera::QtPixelFormatFromCVFormat(unsigned avPixelFormat)
{
    // BGRA <-> ARGB "swap" is intentional:
    // to work correctly with GL_RGBA, color swap shaders
    // (in QSG node renderer etc.).
    switch (avPixelFormat) {
    case kCVPixelFormatType_32ARGB:
        return QVideoSurfaceFormat::Format_BGRA32;
    case kCVPixelFormatType_32BGRA:
        return QVideoSurfaceFormat::Format_ARGB32;
    case kCVPixelFormatType_24RGB:
        return QVideoSurfaceFormat::Format_RGB24;
    case kCVPixelFormatType_24BGR:
        return QVideoSurfaceFormat::Format_BGR24;
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
    case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
        return QVideoSurfaceFormat::Format_NV12;
    case kCVPixelFormatType_422YpCbCr8:
        return QVideoSurfaceFormat::Format_UYVY;
    case kCVPixelFormatType_422YpCbCr8_yuvs:
        return QVideoSurfaceFormat::Format_YUYV;
    case kCMVideoCodecType_JPEG:
    case kCMVideoCodecType_JPEG_OpenDML:
        return QVideoSurfaceFormat::Format_Jpeg;
    default:
        return QVideoSurfaceFormat::Format_Invalid;
    }
}

bool AVFCamera::CVPixelFormatFromQtFormat(QVideoSurfaceFormat::PixelFormat qtFormat, unsigned &conv)
{
    // BGRA <-> ARGB "swap" is intentional:
    // to work correctly with GL_RGBA, color swap shaders
    // (in QSG node renderer etc.).
    switch (qtFormat) {
    case QVideoSurfaceFormat::Format_ARGB32:
        conv = kCVPixelFormatType_32BGRA;
        break;
    case QVideoSurfaceFormat::Format_BGRA32:
        conv = kCVPixelFormatType_32ARGB;
        break;
    case QVideoSurfaceFormat::Format_NV12:
        conv = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
        break;
    case QVideoSurfaceFormat::Format_UYVY:
        conv = kCVPixelFormatType_422YpCbCr8;
        break;
    case QVideoSurfaceFormat::Format_YUYV:
        conv = kCVPixelFormatType_422YpCbCr8_yuvs;
        break;
    // These two formats below are not supported
    // by QSGVideoNodeFactory_RGB, so for now I have to
    // disable them.
    /*
    case QVideoSurfaceFormat::Format_RGB24:
        conv = kCVPixelFormatType_24RGB;
        break;
    case QVideoSurfaceFormat::Format_BGR24:
        conv = kCVPixelFormatType_24BGR;
        break;
    */
    default:
        return false;
    }

    return true;
}

AVCaptureConnection *AVFCamera::videoConnection() const
{
    if (!m_session->videoOutput() || !m_session->videoOutput()->videoDataOutput())
        return nil;

    return [m_session->videoOutput()->videoDataOutput() connectionWithMediaType:AVMediaTypeVideo];
}


QPlatformCameraFocus *AVFCamera::focusControl()
{
    return m_cameraFocusControl;
}

QPlatformCameraExposure *AVFCamera::exposureControl()
{
    return m_cameraExposureControl;
}

QPlatformCameraImageProcessing *AVFCamera::imageProcessingControl()
{
    return m_cameraImageProcessingControl;
}

#include "moc_avfcamera_p.cpp"
