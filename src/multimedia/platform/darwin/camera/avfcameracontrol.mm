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
#include "avfcameracontrol_p.h"
#include "avfcamerasession_p.h"
#include "avfcameraservice_p.h"
#include "avfcamerautility_p.h"
#include "avfcamerarenderercontrol_p.h"
#include "avfcameraexposurecontrol_p.h"
#include "avfcamerafocuscontrol_p.h"
#include "avfcameraimageprocessingcontrol_p.h"
#include "qabstractvideosurface.h"

QT_USE_NAMESPACE

AVFCameraControl::AVFCameraControl(AVFCameraService *service, QObject *parent)
   : QPlatformCamera(parent)
   , m_session(service->session())
   , m_service(service)
   , m_active(false)
   , m_lastStatus(QCamera::InactiveStatus)
{
    m_cameraFocusControl = new AVFCameraFocusControl(m_service);
    m_cameraImageProcessingControl = new AVFCameraImageProcessingControl(m_service);
    m_cameraExposureControl = nullptr;
#ifdef Q_OS_IOS
    m_cameraExposureControl = new AVFCameraExposureControl(m_service);
#endif
    connect(m_session, SIGNAL(activeChanged(bool)), SLOT(updateStatus()));
}

AVFCameraControl::~AVFCameraControl()
{
    delete m_cameraFocusControl;
    delete m_cameraExposureControl;
    delete m_cameraImageProcessingControl;
}

bool AVFCameraControl::isActive() const
{
    return m_active;
}

void AVFCameraControl::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    m_session->setActive(active);

    Q_EMIT activeChanged(m_active);
    updateStatus();
}

QCamera::Status AVFCameraControl::status() const
{
    static QCamera::Status statusTable[2][2] = {
        { QCamera::InactiveStatus,    QCamera::StoppingStatus }, //Inactive state
        { QCamera::StartingStatus,  QCamera::ActiveStatus } //ActiveState
    };

    return statusTable[m_active ? 1 : 0][m_session->isActive() ? 1 : 0];
}

void AVFCameraControl::setCamera(const QCameraInfo &camera)
{
    m_session->setActiveCamera(camera);
}

void AVFCameraControl::updateStatus()
{
    QCamera::Status newStatus = status();

    if (m_lastStatus != newStatus) {
        qDebugCamera() << "Camera status changed: " << m_lastStatus << " -> " << newStatus;
        m_lastStatus = newStatus;
        Q_EMIT statusChanged(m_lastStatus);
    }
}

QVideoFrame::PixelFormat AVFCameraControl::QtPixelFormatFromCVFormat(unsigned avPixelFormat)
{
    // BGRA <-> ARGB "swap" is intentional:
    // to work correctly with GL_RGBA, color swap shaders
    // (in QSG node renderer etc.).
    switch (avPixelFormat) {
    case kCVPixelFormatType_32ARGB:
        return QVideoFrame::Format_BGRA32;
    case kCVPixelFormatType_32BGRA:
        return QVideoFrame::Format_ARGB32;
    case kCVPixelFormatType_24RGB:
        return QVideoFrame::Format_RGB24;
    case kCVPixelFormatType_24BGR:
        return QVideoFrame::Format_BGR24;
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
    case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
        return QVideoFrame::Format_NV12;
    case kCVPixelFormatType_422YpCbCr8:
        return QVideoFrame::Format_UYVY;
    case kCVPixelFormatType_422YpCbCr8_yuvs:
        return QVideoFrame::Format_YUYV;
    case kCMVideoCodecType_JPEG:
    case kCMVideoCodecType_JPEG_OpenDML:
        return QVideoFrame::Format_Jpeg;
    default:
        return QVideoFrame::Format_Invalid;
    }
}

bool AVFCameraControl::CVPixelFormatFromQtFormat(QVideoFrame::PixelFormat qtFormat, unsigned &conv)
{
    // BGRA <-> ARGB "swap" is intentional:
    // to work correctly with GL_RGBA, color swap shaders
    // (in QSG node renderer etc.).
    switch (qtFormat) {
    case QVideoFrame::Format_ARGB32:
        conv = kCVPixelFormatType_32BGRA;
        break;
    case QVideoFrame::Format_BGRA32:
        conv = kCVPixelFormatType_32ARGB;
        break;
    case QVideoFrame::Format_NV12:
        conv = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
        break;
    case QVideoFrame::Format_UYVY:
        conv = kCVPixelFormatType_422YpCbCr8;
        break;
    case QVideoFrame::Format_YUYV:
        conv = kCVPixelFormatType_422YpCbCr8_yuvs;
        break;
    // These two formats below are not supported
    // by QSGVideoNodeFactory_RGB, so for now I have to
    // disable them.
    /*
    case QVideoFrame::Format_RGB24:
        conv = kCVPixelFormatType_24RGB;
        break;
    case QVideoFrame::Format_BGR24:
        conv = kCVPixelFormatType_24BGR;
        break;
    */
    default:
        return false;
    }

    return true;
}

AVCaptureConnection *AVFCameraControl::videoConnection() const
{
    if (!m_session->videoOutput() || !m_session->videoOutput()->videoDataOutput())
        return nil;

    return [m_session->videoOutput()->videoDataOutput() connectionWithMediaType:AVMediaTypeVideo];
}


QPlatformCameraFocus *AVFCameraControl::focusControl()
{
    return m_cameraFocusControl;
}

QPlatformCameraExposure *AVFCameraControl::exposureControl()
{
    return m_cameraExposureControl;
}

QPlatformCameraImageProcessing *AVFCameraControl::imageProcessingControl()
{
    return m_cameraImageProcessingControl;
}

#include "moc_avfcameracontrol_p.cpp"
