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
#include "avfcameraimageprocessing_p.h"
#include <qmediacapturesession.h>

QT_USE_NAMESPACE

AVFCamera::AVFCamera(QCamera *camera)
   : QPlatformCamera(camera)
   , m_active(false)
   , m_lastStatus(QCamera::InactiveStatus)
{
    Q_ASSERT(camera);

    m_cameraImageProcessingControl = new AVFCameraImageProcessing(this);
    m_cameraExposureControl = nullptr;
#ifdef Q_OS_IOS
    m_cameraExposureControl = new AVFCameraExposure(this);
#endif
}

AVFCamera::~AVFCamera()
{
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
    if (m_cameraInfo.isNull() && active)
        return;

    m_active = active;
    if (m_session)
        m_session->setActive(active);

    Q_EMIT activeChanged(m_active);
    updateStatus();
    if (active)
        updateCameraConfiguration();
}

QCamera::Status AVFCamera::status() const
{
    static const QCamera::Status statusTable[2][2] = {
        { QCamera::InactiveStatus,    QCamera::StoppingStatus }, //Inactive state
        { QCamera::StartingStatus,  QCamera::ActiveStatus } //ActiveState
    };

    if (!m_session)
        return QCamera::InactiveStatus;

    return statusTable[m_active ? 1 : 0][m_session->isActive() ? 1 : 0];
}

void AVFCamera::setCamera(const QCameraInfo &camera)
{
    if (m_cameraInfo == camera)
        return;
    m_cameraInfo = camera;
    if (m_session)
        m_session->setActiveCamera(camera);
}

void AVFCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    AVFCameraService *captureSession = static_cast<AVFCameraService *>(session);
    if (m_service == captureSession)
        return;

    m_service = captureSession;
    if (!m_service) {
        m_session = nullptr;
        return;
    }

    m_session = m_service->session();
    Q_ASSERT(m_session);
    connect(m_session, SIGNAL(activeChanged(bool)), SLOT(updateStatus()));

    m_session->setActiveCamera(QCameraInfo());
    m_session->setActive(m_active);
    m_session->setActiveCamera(m_cameraInfo);
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

AVCaptureConnection *AVFCamera::videoConnection() const
{
    if (!m_session || !m_session->videoOutput() || !m_session->videoOutput()->videoDataOutput())
        return nil;

    return [m_session->videoOutput()->videoDataOutput() connectionWithMediaType:AVMediaTypeVideo];
}

AVCaptureDevice *AVFCamera::device() const
{
    AVCaptureDevice *device = nullptr;
    QByteArray deviceId = m_cameraInfo.id();
    if (!deviceId.isEmpty()) {
        device = [AVCaptureDevice deviceWithUniqueID:
                    [NSString stringWithUTF8String:
                        deviceId.constData()]];
    }
    return device;
}

QPlatformCameraExposure *AVFCamera::exposureControl()
{
    return m_cameraExposureControl;
}

QPlatformCameraImageProcessing *AVFCamera::imageProcessingControl()
{
    return m_cameraImageProcessingControl;
}

#ifdef Q_OS_IOS
namespace
{

bool qt_focus_mode_supported(QCamera::FocusMode mode)
{
    // Check if QCamera::FocusMode has counterpart in AVFoundation.

    // AVFoundation has 'Manual', 'Auto' and 'Continuous',
    // where 'Manual' is actually 'Locked' + writable property 'lensPosition'.
    return mode == QCamera::FocusModeAuto
           || mode == QCamera::FocusModeManual;
}

AVCaptureFocusMode avf_focus_mode(QCamera::FocusMode requestedMode)
{
    switch (requestedMode) {
        case QCamera::FocusModeHyperfocal:
        case QCamera::FocusModeInfinity:
        case QCamera::FocusModeManual:
            return AVCaptureFocusModeLocked;
        default:
            return AVCaptureFocusModeContinuousAutoFocus;
    }

}

}
#endif

void AVFCamera::setFocusMode(QCamera::FocusMode mode)
{
#ifdef Q_OS_IOS
    if (focusMode() == mode)
        return;

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        if (qt_focus_mode_supported(mode)) {
            focusModeChanged(mode);
        } else {
            qDebugCamera() << Q_FUNC_INFO
                           << "focus mode not supported";
        }
        return;
    }

    if (isFocusModeSupported(mode)) {
        const AVFConfigurationLock lock(captureDevice);
        if (!lock) {
            qDebugCamera() << Q_FUNC_INFO
                           << "failed to lock for configuration";
            return;
        }

        captureDevice.focusMode = avf_focus_mode(mode);
        m_focusMode = mode;
    } else {
        qDebugCamera() << Q_FUNC_INFO << "focus mode not supported";
        return;
    }

    Q_EMIT focusModeChanged(m_focusMode);
#else
    Q_UNUSED(mode);
#endif
}

bool AVFCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
#ifdef Q_OS_IOS
    AVCaptureDevice *captureDevice = device();
    if (captureDevice) {
        AVCaptureFocusMode avMode = avf_focus_mode(mode);
        switch (mode) {
            case QCamera::FocusModeAuto:
            case QCamera::FocusModeHyperfocal:
            case QCamera::FocusModeInfinity:
            case QCamera::FocusModeManual:
                return [captureDevice isFocusModeSupported:avMode];
        case QCamera::FocusModeAutoNear:
            Q_FALLTHROUGH();
        case QCamera::FocusModeAutoFar:
            return captureDevice.autoFocusRangeRestrictionSupported
                && [captureDevice isFocusModeSupported:avMode];
        }
    }
#endif
    return mode == QCamera::FocusModeAuto; // stupid builtin webcam doesn't do any focus handling, but hey it's usually focused :)
}

void AVFCamera::setCustomFocusPoint(const QPointF &point)
{
    if (customFocusPoint() == point)
        return;

    if (!QRectF(0.f, 0.f, 1.f, 1.f).contains(point)) {
        // ### release custom focus point, tell the camera to focus where it wants...
        qDebugCamera() << Q_FUNC_INFO << "invalid focus point (out of range)";
        return;
    }

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return;

    if ([captureDevice isFocusPointOfInterestSupported]) {
        const AVFConfigurationLock lock(captureDevice);
        if (!lock) {
            qDebugCamera() << Q_FUNC_INFO << "failed to lock for configuration";
            return;
        }

        const CGPoint focusPOI = CGPointMake(point.x(), point.y());
        [captureDevice setFocusPointOfInterest:focusPOI];
        if (focusMode() != QCamera::FocusModeAuto)
            [captureDevice setFocusMode:AVCaptureFocusModeAutoFocus];

        customFocusPointChanged(point);
    }
}

bool AVFCamera::isCustomFocusPointSupported() const
{
    return true;
}

void AVFCamera::setFocusDistance(float d)
{
#ifdef Q_OS_IOS
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return;

    if (captureDevice.lockingFocusWithCustomLensPositionSupported) {
        qDebugCamera() << Q_FUNC_INFO << "Setting custom focus distance not supported\n";
        return;
    }

    {
        AVFConfigurationLock lock(captureDevice);
        if (!lock) {
            qDebugCamera() << Q_FUNC_INFO << "failed to lock for configuration";
            return;
        }
        [captureDevice setFocusModeLockedWithLensPosition:d completionHandler:nil];
    }
    focusDistanceChanged(d);
#else
    Q_UNUSED(d);
#endif
}

void AVFCamera::updateCameraConfiguration()
{
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        qDebugCamera() << Q_FUNC_INFO << "capture device is nil in 'active' state";
        return;
    }

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qDebugCamera() << Q_FUNC_INFO << "failed to lock for configuration";
        return;
    }

    if ([captureDevice isFocusPointOfInterestSupported]) {
        auto point = customFocusPoint();
        const CGPoint focusPOI = CGPointMake(point.x(), point.y());
        [captureDevice setFocusPointOfInterest:focusPOI];
    }

#ifdef Q_OS_IOS
    if (m_focusMode != QCamera::FocusModeAuto) {
        const AVCaptureFocusMode avMode = avf_focus_mode(m_focusMode);
        if (captureDevice.focusMode != avMode) {
            if ([captureDevice isFocusModeSupported:avMode]) {
                [captureDevice setFocusMode:avMode];
            } else {
                qDebugCamera() << Q_FUNC_INFO << "focus mode not supported";
            }
        }
    }

    if (!captureDevice.activeFormat) {
        qDebugCamera() << Q_FUNC_INFO << "camera state is active, but active format is nil";
        return;
    }

    minimumZoomFactorChanged(captureDevice.minAvailableVideoZoomFactor);
    maximumZoomFactorChanged(captureDevice.maxAvailableVideoZoomFactor);

    captureDevice.videoZoomFactor = m_zoomFactor;
#endif
}

void AVFCamera::zoomTo(float factor, float rate)
{
    Q_UNUSED(factor);
    Q_UNUSED(rate);

#ifdef Q_OS_IOS
    if (zoomFactor() == factor)
        return;

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice || !captureDevice.activeFormat)
        return;

    factor = qBound(captureDevice.minAvailableVideoZoomFactor, factor, captureDevice.maxAvailableVideoZoomFactor);

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qDebugCamera() << Q_FUNC_INFO << "failed to lock for configuration";
        return;
    }

    if (rate < 0)
        captureDevice.videoZoomFactor = foom;
    else
        [AVCaptureDevice rampToVideoZoomFactor:factor withRate:rate];
#endif
}

#include "moc_avfcamera_p.cpp"
