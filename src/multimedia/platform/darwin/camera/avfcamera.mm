/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd and/or its subsidiary(-ies).
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

bool qt_check_exposure_duration(AVCaptureDevice *captureDevice, CMTime duration)
{
    Q_ASSERT(captureDevice);

    AVCaptureDeviceFormat *activeFormat = captureDevice.activeFormat;
    if (!activeFormat) {
        qDebugCamera() << Q_FUNC_INFO << "failed to obtain capture device format";
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
        qDebugCamera() << Q_FUNC_INFO << "failed to obtain capture device format";
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

bool isFlashAvailable(AVCaptureDevice* captureDevice) {
    if (@available(macOS 10.15, *)) {
        return [captureDevice isFlashAvailable];
    }

    return true;
}

bool isTorchAvailable(AVCaptureDevice* captureDevice) {
    if (@available(macOS 10.15, *)) {
        return [captureDevice isTorchAvailable];
    }

    return true;
}

} // Unnamed namespace.


AVFCamera::AVFCamera(QCamera *camera)
   : QPlatformCamera(camera)
{
    Q_ASSERT(camera);
}

AVFCamera::~AVFCamera()
{
}

bool AVFCamera::isActive() const
{
    return m_active;
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

AVCaptureDevice *AVFCamera::device() const
{
    AVCaptureDevice *device = nullptr;
    QByteArray deviceId = m_cameraDevice.id();
    if (!deviceId.isEmpty()) {
        device = [AVCaptureDevice deviceWithUniqueID:
                    [NSString stringWithUTF8String:
                        deviceId.constData()]];
    }
    return device;
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
    } else {
        qDebugCamera() << Q_FUNC_INFO << "focus mode not supported";
        return;
    }

    Q_EMIT focusModeChanged(mode);
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
    if (focusMode() != QCamera::FocusModeAuto) {
        const AVCaptureFocusMode avMode = avf_focus_mode(focusMode());
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
    maximumZoomFactorChanged(captureDevice.activeFormat.videoMaxZoomFactor);

    captureDevice.videoZoomFactor = zoomFactor();

    CMTime newDuration = AVCaptureExposureDurationCurrent;
    bool setCustomMode = false;

    float exposureTime = manualExposureTime();
    if (exposureTime > 0
        && !qt_exposure_duration_equal(captureDevice, exposureTime)) {
        newDuration = CMTimeMakeWithSeconds(exposureTime, captureDevice.exposureDuration.timescale);
        if (!qt_check_exposure_duration(captureDevice, newDuration)) {
            qDebugCamera() << Q_FUNC_INFO << "requested exposure duration is out of range";
            return;
        }
        setCustomMode = true;
    }

    float newISO = AVCaptureISOCurrent;
    int iso = manualIsoSensitivity();
    if (iso > 0 && !qt_iso_equal(captureDevice, iso)) {
        newISO = iso;
        if (!qt_check_ISO_value(captureDevice, newISO)) {
            qDebugCamera() << Q_FUNC_INFO << "requested ISO value is out of range";
            return;
        }
        setCustomMode = true;
    }

    float bias = exposureCompensation();
    if (bias != 0 && !qt_exposure_bias_equal(captureDevice, bias)) {
        // TODO: mixed fpns.
        if (bias < captureDevice.minExposureTargetBias || bias > captureDevice.maxExposureTargetBias) {
            qDebugCamera() << Q_FUNC_INFO << "exposure compensation value is"
                           << "out of range";
            return;
        }
        [captureDevice setExposureTargetBias:bias completionHandler:nil];
    }

    // Setting shutter speed (exposure duration) or ISO values
    // also reset exposure mode into Custom. With this settings
    // we ignore any attempts to set exposure mode.

    if (setCustomMode) {
        [captureDevice setExposureModeCustomWithDuration:newDuration
                                                     ISO:newISO
                                       completionHandler:nil];
        return;
    }

    QCamera::ExposureMode qtMode = exposureMode();
    AVCaptureExposureMode avMode = AVCaptureExposureModeContinuousAutoExposure;
    if (!qt_convert_exposure_mode(captureDevice, qtMode, avMode)) {
        qDebugCamera() << Q_FUNC_INFO << "requested exposure mode is not supported";
        return;
    }

    captureDevice.exposureMode = avMode;
#endif

    isFlashSupported = isFlashAutoSupported = false;
    isTorchSupported = isTorchAutoSupported = false;

    if (captureDevice.hasFlash) {
        if ([captureDevice isFlashModeSupported:AVCaptureFlashModeOn])
            isFlashSupported = true;
        if ([captureDevice isFlashModeSupported:AVCaptureFlashModeAuto])
            isFlashAutoSupported = true;
    }

    if (captureDevice.hasTorch) {
        if ([captureDevice isTorchModeSupported:AVCaptureTorchModeOn])
            isTorchSupported = true;
        if ([captureDevice isTorchModeSupported:AVCaptureTorchModeAuto])
            isTorchAutoSupported = true;
    }

    applyFlashSettings();
    flashReadyChanged(isFlashSupported);
}

void AVFCamera::updateCameraProperties()
{
    QCamera::Features features;
    AVCaptureDevice *captureDevice = device();

#ifdef Q_OS_IOS
    features = QCamera::Feature::ColorTemperature | QCamera::Feature::ExposureCompensation |
                          QCamera::Feature::IsoSensitivity | QCamera::Feature::ManualExposureTime;

    if (captureDevice && [captureDevice isLockingFocusWithCustomLensPositionSupported])
        features |= QCamera::Feature::FocusDistance;
#endif

    if (captureDevice && [captureDevice isFocusPointOfInterestSupported])
        features |= QCamera::Feature::CustomFocusPoint;

    supportedFeaturesChanged(features);
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

    factor = qBound(captureDevice.minAvailableVideoZoomFactor, factor,
                    captureDevice.activeFormat.videoMaxZoomFactor);

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qDebugCamera() << Q_FUNC_INFO << "failed to lock for configuration";
        return;
    }

    if (rate <= 0)
        captureDevice.videoZoomFactor = factor;
    else
        [captureDevice rampToVideoZoomFactor:factor withRate:rate];
#endif
}

void AVFCamera::setFlashMode(QCamera::FlashMode mode)
{
    if (flashMode() == mode)
        return;

    if (isActive() && !isFlashModeSupported(mode)) {
        qDebugCamera() << Q_FUNC_INFO << "unsupported mode" << mode;
        return;
    }

    flashModeChanged(mode);

    if (!isActive())
        return;

    applyFlashSettings();
}

bool AVFCamera::isFlashModeSupported(QCamera::FlashMode mode) const
{
    if (mode == QCamera::FlashOff)
        return true;
    else if (mode == QCamera::FlashOn)
        return isFlashSupported;
    else //if (mode == QCamera::FlashAuto)
        return isFlashAutoSupported;
}

bool AVFCamera::isFlashReady() const
{
    if (!isActive())
        return false;

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return false;

    if (!captureDevice.hasFlash)
        return false;

    if (!isFlashModeSupported(flashMode()))
        return false;

    return isFlashAvailable(captureDevice);
}

void AVFCamera::setTorchMode(QCamera::TorchMode mode)
{
    if (torchMode() == mode)
        return;

    if (isActive() && !isTorchModeSupported(mode)) {
        qDebugCamera() << Q_FUNC_INFO << "unsupported torch mode" << mode;
        return;
    }

    torchModeChanged(mode);

    if (!isActive())
        return;

    applyFlashSettings();
}

bool AVFCamera::isTorchModeSupported(QCamera::TorchMode mode) const
{
    if (mode == QCamera::TorchOff)
        return true;
    else if (mode == QCamera::TorchOn)
        return isTorchSupported;
    else //if (mode == QCamera::TorchAuto)
        return isTorchAutoSupported;
}

void AVFCamera::setExposureMode(QCamera::ExposureMode qtMode)
{
#ifdef Q_OS_IOS
    if (qtMode != QCamera::ExposureAuto && qtMode != QCamera::ExposureManual) {
        qDebugCamera() << Q_FUNC_INFO << "exposure mode not supported";
        return;
    }

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        exposureModeChanged(qtMode);
        return;
    }

    AVCaptureExposureMode avMode = AVCaptureExposureModeContinuousAutoExposure;
    if (!qt_convert_exposure_mode(captureDevice, qtMode, avMode)) {
        qDebugCamera() << Q_FUNC_INFO << "exposure mode not supported";
        return;
    }

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qDebugCamera() << Q_FUNC_INFO << "failed to lock a capture device"
                       << "for configuration";
        return;
    }

    [captureDevice setExposureMode:avMode];
    exposureModeChanged(qtMode);
#else
    Q_UNUSED(qtMode);
#endif
}

bool AVFCamera::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    if (mode == QCamera::ExposureAuto)
        return true;
    if (mode != QCamera::ExposureManual)
        return false;

    if (@available(macOS 10.15, *)) {
        AVCaptureDevice *captureDevice = device();
        return captureDevice && [captureDevice isExposureModeSupported:AVCaptureExposureModeCustom];
    }

    return false;
}

void AVFCamera::applyFlashSettings()
{
    Q_ASSERT(isActive());

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        qDebugCamera() << Q_FUNC_INFO << "no capture device found";
        return;
    }


    const AVFConfigurationLock lock(captureDevice);

    if (captureDevice.hasFlash) {
        auto mode = flashMode();

        auto setAvFlashModeSafe = [&captureDevice](AVCaptureFlashMode avFlashMode) {
            // Note, in some cases captureDevice.hasFlash == false even though
            // no there're no supported flash modes.
            if ([captureDevice isFlashModeSupported:avFlashMode])
                captureDevice.flashMode = avFlashMode;
            else
                qDebugCamera() << Q_FUNC_INFO << "Attempt to setup unsupported flash mode " << avFlashMode;
        };

        if (mode == QCamera::FlashOff) {
            setAvFlashModeSafe(AVCaptureFlashModeOff);
        } else {
            if (isFlashAvailable(captureDevice)) {
                if (mode == QCamera::FlashOn)
                    setAvFlashModeSafe(AVCaptureFlashModeOn);
                else if (mode == QCamera::FlashAuto)
                    setAvFlashModeSafe(AVCaptureFlashModeAuto);
            } else {
                qDebugCamera() << Q_FUNC_INFO << "flash is not available at the moment";
            }
        }
    }

    if (captureDevice.hasTorch) {
        auto mode = torchMode();

        auto setAvTorchModeSafe = [&captureDevice](AVCaptureTorchMode avTorchMode) {
            if ([captureDevice isTorchModeSupported:avTorchMode])
                captureDevice.torchMode = avTorchMode;
            else
                qDebugCamera() << Q_FUNC_INFO << "Attempt to setup unsupported torch mode " << avTorchMode;
        };

        if (mode == QCamera::TorchOff) {
            setAvTorchModeSafe(AVCaptureTorchModeOff);
        } else {
            if (isTorchAvailable(captureDevice)) {
                if (mode == QCamera::TorchOn)
                    setAvTorchModeSafe(AVCaptureTorchModeOn);
                else if (mode == QCamera::TorchAuto)
                    setAvTorchModeSafe(AVCaptureTorchModeAuto);
            } else {
                qDebugCamera() << Q_FUNC_INFO << "torch is not available at the moment";
            }
        }
    }
}


void AVFCamera::setExposureCompensation(float bias)
{
#ifdef Q_OS_IOS
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        exposureCompensationChanged(bias);
        return;
    }

    bias = qBound(captureDevice.minExposureTargetBias, bias, captureDevice.maxExposureTargetBias);

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qDebugCamera() << Q_FUNC_INFO << "failed to lock for configuration";
        return;
    }

    [captureDevice setExposureTargetBias:bias completionHandler:nil];
    exposureCompensationChanged(bias);
#else
    Q_UNUSED(bias);
#endif
}

void AVFCamera::setManualExposureTime(float value)
{
#ifdef Q_OS_IOS
    if (value < 0) {
        setExposureMode(QCamera::ExposureAuto);
        return;
    }

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        exposureTimeChanged(value);
        return;
    }

    const CMTime newDuration = CMTimeMakeWithSeconds(value, captureDevice.exposureDuration.timescale);
    if (!qt_check_exposure_duration(captureDevice, newDuration)) {
        qDebugCamera() << Q_FUNC_INFO << "shutter speed value is out of range";
        return;
    }

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qDebugCamera() << Q_FUNC_INFO << "failed to lock for configuration";
        return;
    }

    // Setting the shutter speed (exposure duration in Apple's terms,
    // since there is no shutter actually) will also reset
    // exposure mode into custom mode.
    [captureDevice setExposureModeCustomWithDuration:newDuration
                                                 ISO:AVCaptureISOCurrent
                                   completionHandler:nil];

    exposureTimeChanged(value);

#else
    Q_UNUSED(value);
#endif
}

float AVFCamera::exposureTime() const
{
#ifdef Q_OS_IOS
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return -1.;
    auto duration = captureDevice.exposureDuration;
    return CMTimeGetSeconds(duration);
#else
    return -1;
#endif
}

#ifdef Q_OS_IOS
namespace {

void avf_convert_white_balance_mode(QCamera::WhiteBalanceMode qtMode,
        AVCaptureWhiteBalanceMode &avMode)
{
    if (qtMode == QCamera::WhiteBalanceAuto)
        avMode = AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance;
    else
        avMode = AVCaptureWhiteBalanceModeLocked;
}

bool avf_set_white_balance_mode(AVCaptureDevice *captureDevice,
        AVCaptureWhiteBalanceMode avMode)
{
    Q_ASSERT(captureDevice);

    const bool lock = [captureDevice lockForConfiguration:nil];
    if (!lock) {
        qDebug() << "Failed to lock a capture device for configuration\n";
        return false;
    }

    captureDevice.whiteBalanceMode = avMode;
    [captureDevice unlockForConfiguration];
    return true;
}

bool avf_convert_temp_and_tint_to_wb_gains(AVCaptureDevice *captureDevice,
        float temp, float tint, AVCaptureWhiteBalanceGains &wbGains)
{
    Q_ASSERT(captureDevice);

    AVCaptureWhiteBalanceTemperatureAndTintValues wbTTValues = {
        .temperature = temp,
        .tint = tint
    };
    wbGains = [captureDevice deviceWhiteBalanceGainsForTemperatureAndTintValues:wbTTValues];

    if (wbGains.redGain >= 1.0 && wbGains.redGain <= captureDevice.maxWhiteBalanceGain
        && wbGains.greenGain >= 1.0 && wbGains.greenGain <= captureDevice.maxWhiteBalanceGain
        && wbGains.blueGain >= 1.0 && wbGains.blueGain <= captureDevice.maxWhiteBalanceGain)
        return true;

    return false;
}

bool avf_set_white_balance_gains(AVCaptureDevice *captureDevice,
        AVCaptureWhiteBalanceGains wbGains)
{
    const bool lock = [captureDevice lockForConfiguration:nil];
    if (!lock) {
        qDebug() << "Failed to lock a capture device for configuration\n";
        return false;
    }

    [captureDevice setWhiteBalanceModeLockedWithDeviceWhiteBalanceGains:wbGains
        completionHandler:nil];
    [captureDevice unlockForConfiguration];
    return true;
}

}

bool AVFCamera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    if (mode == QCamera::WhiteBalanceAuto)
        return true;
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return false;
    return [captureDevice isWhiteBalanceModeSupported:AVCaptureWhiteBalanceModeLocked];
}

void AVFCamera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    if (!isWhiteBalanceModeSupported(mode))
        return;

    AVCaptureDevice *captureDevice = device();
    Q_ASSERT(captureDevice);

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qDebugCamera() << Q_FUNC_INFO << "failed to lock a capture device"
                       << "for configuration";
        return;
    }

    AVCaptureWhiteBalanceMode avMode;
    avf_convert_white_balance_mode(mode, avMode);
    avf_set_white_balance_mode(captureDevice, avMode);

    if (mode == QCamera::WhiteBalanceAuto || mode == QCamera::WhiteBalanceManual) {
        whiteBalanceModeChanged(mode);
        return;
    }

    const int colorTemp = colorTemperatureForWhiteBalance(mode);
    AVCaptureWhiteBalanceGains wbGains;
    if (avf_convert_temp_and_tint_to_wb_gains(captureDevice, colorTemp, 0., wbGains)
        && avf_set_white_balance_gains(captureDevice, wbGains))
        whiteBalanceModeChanged(mode);
}

void AVFCamera::setColorTemperature(int colorTemp)
{
    if (colorTemp == 0) {
        colorTemperatureChanged(colorTemp);
        return;
    }

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice || ![captureDevice isWhiteBalanceModeSupported:AVCaptureWhiteBalanceModeLocked])
        return;

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qDebugCamera() << Q_FUNC_INFO << "failed to lock a capture device"
                       << "for configuration";
        return;
    }

    AVCaptureWhiteBalanceGains wbGains;
    if (avf_convert_temp_and_tint_to_wb_gains(captureDevice, colorTemp, 0., wbGains)
        && avf_set_white_balance_gains(captureDevice, wbGains))
        colorTemperatureChanged(colorTemp);
}
#endif

void AVFCamera::setManualIsoSensitivity(int value)
{
#ifdef Q_OS_IOS
    if (value < 0) {
        setExposureMode(QCamera::ExposureAuto);
        return;
    }

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        isoSensitivityChanged(value);
        return;
    }

    if (!qt_check_ISO_value(captureDevice, value)) {
        qDebugCamera() << Q_FUNC_INFO << "ISO value is out of range";
        return;
    }

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qDebugCamera() << Q_FUNC_INFO << "failed to lock a capture device"
                       << "for configuration";
        return;
    }

    // Setting the ISO will also reset
    // exposure mode to the custom mode.
    [captureDevice setExposureModeCustomWithDuration:AVCaptureExposureDurationCurrent
                                                 ISO:value
                                   completionHandler:nil];

    isoSensitivityChanged(value);
#else
    Q_UNUSED(value);
#endif
}

int AVFCamera::isoSensitivity() const
{
    return manualIsoSensitivity();
}


#include "moc_avfcamera_p.cpp"
