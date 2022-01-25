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
#include <qmediacapturesession.h>

QT_USE_NAMESPACE


namespace {

// All these methods to work with exposure/ISO/SS in custom mode do not support macOS.

#ifdef Q_OS_IOS

// Misc. helpers to check values/ranges:

bool qt_check_ISO_conversion(float isoValue)
{
    if (isoValue >= std::numeric_limits<int>::max())
        return false;
    if (isoValue <= std::numeric_limits<int>::min())
        return false;
    return true;
}

bool qt_check_ISO_range(AVCaptureDeviceFormat *format)
{
    // Qt is using int for ISO, AVFoundation - float. It looks like the ISO range
    // at the moment can be represented by int (it's max - min > 100, etc.).
    Q_ASSERT(format);
    if (format.maxISO - format.minISO < 1.) {
        // ISO is in some strange units?
        return false;
    }

    return qt_check_ISO_conversion(format.minISO)
           && qt_check_ISO_conversion(format.maxISO);
}

bool qt_check_exposure_duration(AVCaptureDevice *captureDevice, CMTime duration)
{
    Q_ASSERT(captureDevice);

    AVCaptureDeviceFormat *activeFormat = captureDevice.activeFormat;
    if (!activeFormat) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to obtain capture device format";
        return false;
    }

    return CMTimeCompare(duration, activeFormat.minExposureDuration) != -1
           && CMTimeCompare(activeFormat.maxExposureDuration, duration) != -1;
}

bool qt_check_ISO_value(AVCaptureDevice *captureDevice, int newISO)
{
    Q_ASSERT(captureDevice);

    AVCaptureDeviceFormat *activeFormat = captureDevice.activeFormat;
    if (!activeFormat) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to obtain capture device format";
        return false;
    }

    return !(newISO < activeFormat.minISO || newISO > activeFormat.maxISO);
}

bool qt_exposure_duration_equal(AVCaptureDevice *captureDevice, qreal qDuration)
{
    Q_ASSERT(captureDevice);
    const CMTime avDuration = CMTimeMakeWithSeconds(qDuration, captureDevice.exposureDuration.timescale);
    return !CMTimeCompare(avDuration, captureDevice.exposureDuration);
}

bool qt_iso_equal(AVCaptureDevice *captureDevice, int iso)
{
    Q_ASSERT(captureDevice);
    return qFuzzyCompare(float(iso), captureDevice.ISO);
}

bool qt_exposure_bias_equal(AVCaptureDevice *captureDevice, qreal bias)
{
    Q_ASSERT(captureDevice);
    return qFuzzyCompare(bias, qreal(captureDevice.exposureTargetBias));
}

// Converters:

bool qt_convert_exposure_mode(AVCaptureDevice *captureDevice, QCamera::ExposureMode mode,
                              AVCaptureExposureMode &avMode)
{
    // Test if mode supported and convert.
    Q_ASSERT(captureDevice);

    if (mode == QCamera::ExposureAuto) {
        if ([captureDevice isExposureModeSupported:AVCaptureExposureModeContinuousAutoExposure]) {
            avMode = AVCaptureExposureModeContinuousAutoExposure;
            return true;
        }
    }

    if (mode == QCamera::ExposureManual) {
        if ([captureDevice isExposureModeSupported:AVCaptureExposureModeCustom]) {
            avMode = AVCaptureExposureModeCustom;
            return true;
        }
    }

    return false;
}

#endif // defined(Q_OS_IOS)

} // Unnamed namespace.


AVFCamera::AVFCamera(QCamera *camera)
   : QAVFCameraBase(camera)
{
    Q_ASSERT(camera);
}

AVFCamera::~AVFCamera()
{
}

void AVFCamera::setActive(bool active)
{
    if (m_active == active)
        return;
    if (m_cameraDevice.isNull() && active)
        return;

    m_active = active;
    if (m_session)
        m_session->setActive(active);

    if (active)
        updateCameraConfiguration();
    Q_EMIT activeChanged(m_active);
}

void AVFCamera::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;
    m_cameraDevice = camera;
    if (m_session)
        m_session->setActiveCamera(camera);
    setCameraFormat({});
}

bool AVFCamera::setCameraFormat(const QCameraFormat &format)
{
    if (!format.isNull() && !m_cameraDevice.videoFormats().contains(format))
        return false;

    m_cameraFormat = format.isNull() ? findBestCameraFormat(m_cameraDevice) : format;

    if (m_session)
        m_session->setCameraFormat(m_cameraFormat);

    return true;
}

void AVFCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    AVFCameraService *captureSession = static_cast<AVFCameraService *>(session);
    if (m_service == captureSession)
        return;

    if (m_session) {
        m_session->disconnect(this);
        m_session->setActiveCamera({});
        m_session->setCameraFormat({});
    }

    m_service = captureSession;
    if (!m_service) {
        m_session = nullptr;
        return;
    }

    m_session = m_service->session();
    Q_ASSERT(m_session);

    m_session->setActiveCamera(m_cameraDevice);
    m_session->setCameraFormat(m_cameraFormat);
    m_session->setActive(m_active);
}

AVCaptureConnection *AVFCamera::videoConnection() const
{
    if (!m_session || !m_session->videoOutput() || !m_session->videoOutput()->videoDataOutput())
        return nil;

    return [m_session->videoOutput()->videoDataOutput() connectionWithMediaType:AVMediaTypeVideo];
}

#include "moc_avfcamera_p.cpp"
