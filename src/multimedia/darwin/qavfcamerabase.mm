// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qavfcamerabase_p.h"
#include <QtMultimedia/private/qavfcameradebug_p.h>
#include <QtMultimedia/private/qavfcamerautility_p.h>
#include <QtMultimedia/private/qavfhelpers_p.h>
#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qpermissions.h>
#include <QtCore/qset.h>
#include <QtCore/qsystemdetection.h>

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

QAVFCameraBase::QAVFCameraBase(QCamera *camera)
   : QPlatformCamera(camera)
{
    Q_ASSERT(camera);
}

QAVFCameraBase::~QAVFCameraBase()
{
}

bool QAVFCameraBase::isActive() const
{
    return m_active;
}

void QAVFCameraBase::setActive(bool active)
{
    if (m_active == active)
        return;
    if (m_cameraDevice.isNull() && active)
        return;
    if (!checkCameraPermission())
        return;

    m_active = active;

    onActiveChanged(active);

    emit activeChanged(m_active);
}

void QAVFCameraBase::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;
    m_cameraDevice = camera;

    onCameraDeviceChanged(camera);

    // Setting camera format and properties must happen after the
    // backend applies backend specific device changes.
    setCameraFormat({});
    updateSupportedFeatures();
    updateCameraConfiguration();
}

bool QAVFCameraBase::setCameraFormat(const QCameraFormat &newFormat)
{
    if (!newFormat.isNull() && !m_cameraDevice.videoFormats().contains(newFormat))
        return false;

    const QCameraFormat resolvedFormat = newFormat.isNull() ? findBestCameraFormat(m_cameraDevice) : newFormat;
    // If we still couldn't find a suitable default format (such as when none are available)
    // we don't accept the value.
    if (resolvedFormat.isNull())
        return false;

    const bool applySuccess = tryApplyCameraFormat(resolvedFormat);
    if (!applySuccess)
        return false;

    m_cameraFormat = resolvedFormat;

    return true;
}

AVCaptureDevice *QAVFCameraBase::device() const
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

namespace
{

// Only used for setting focus-mode when we have no device attached
// to this QCamera.
bool qt_focus_mode_supported(QCamera::FocusMode mode)
{
    return mode == QCamera::FocusModeAuto;
}

}

void QAVFCameraBase::setFocusMode(QCamera::FocusMode mode)
{
    if (mode == focusMode())
        return;
    forceSetFocusMode(mode);
}

// Does not check if new QCamera::FocusMode is same as old one.
void QAVFCameraBase::forceSetFocusMode(QCamera::FocusMode mode)
{
    if (!isFocusModeSupported(mode)) {
        qCDebug(qLcCamera)
                << Q_FUNC_INFO
                << QString(u"attempted to set focus-mode '%1' on camera where it is unsupported.")
                           .arg(QMetaEnum::fromType<QCamera::FocusMode>().valueToKey(mode));
        return;
    }

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        if (qt_focus_mode_supported(mode)) {
            focusModeChanged(mode);
        } else {
            qCDebug(qLcCamera) << Q_FUNC_INFO
                           << "focus mode not supported";
        }
        return;
    }

    if (mode == QCamera::FocusModeManual) {
        // If we the new focus-mode is 'Manual', then we need to lock
        // lens focus position according to focusDistance().
        //
        // At this point, all relevant settings should have valid values
        // by handling input in setFocusDistance.
        applyFocusDistanceToAVCaptureDevice(focusDistance());
    } else {
        // Apply the focus mode to the AVCaptureDevice.
        const AVFConfigurationLock lock(captureDevice);
        if (!lock) {
            qCDebug(qLcCamera) <<
                Q_FUNC_INFO <<
                "failed to lock for configuration";
            return;
        }
        // Note: We have to set the focus-mode even if capture-device reports
        // the existing value as being the same. For example, this is the case
        // when using the 'setFocusModeLockedWithLensPosition' functionality.
        captureDevice.focusMode = AVCaptureFocusModeContinuousAutoFocus;
    }

    focusModeChanged(mode);
}

bool QAVFCameraBase::isFocusModeSupported(QCamera::FocusMode mode) const
{
    AVCaptureDevice *captureDevice = device();
    if (captureDevice) {
        switch (mode) {
        case QCamera::FocusModeAuto:
            return [captureDevice isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus];
#ifdef Q_OS_IOS
        case QCamera::FocusModeManual:
            return captureDevice.lockingFocusWithCustomLensPositionSupported;
#endif // Q_OS_IOS
        case QCamera::FocusModeAutoNear:
            Q_FALLTHROUGH();
        case QCamera::FocusModeAutoFar:
#ifdef Q_OS_IOS
            return captureDevice.autoFocusRangeRestrictionSupported
                && [captureDevice isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus];
#else
            break;
#endif // Q_OS_IOS
        default:
            break;
        }
    }
    return mode == QCamera::FocusModeAuto;
}

void QAVFCameraBase::setCustomFocusPoint(const QPointF &point)
{
    if (customFocusPoint() == point)
        return;

    if (!QRectF(0.f, 0.f, 1.f, 1.f).contains(point)) {
        // ### release custom focus point, tell the camera to focus where it wants...
        qCDebug(qLcCamera) << Q_FUNC_INFO << "invalid focus point (out of range)";
        return;
    }

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return;

    if ([captureDevice isFocusPointOfInterestSupported]) {
        const AVFConfigurationLock lock(captureDevice);
        if (!lock) {
            qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock for configuration";
            return;
        }

        const CGPoint focusPOI = CGPointMake(point.x(), point.y());
        [captureDevice setFocusPointOfInterest:focusPOI];
        if (focusMode() != QCamera::FocusModeAuto)
            [captureDevice setFocusMode:AVCaptureFocusModeAutoFocus];

        customFocusPointChanged(point);
    }
}

void QAVFCameraBase::applyFocusDistanceToAVCaptureDevice(float distance)
{
#ifdef Q_OS_IOS
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return;

    // This should generally always return true assuming the check
    // for supportedFeatures succeeds, but the consequence of it not being
    // the case is a thrown exception that we don't handle.
    // So it can be useful to keep it here just in case.
    if (!captureDevice.lockingFocusWithCustomLensPositionSupported) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to apply focusDistance to AVCaptureDevice.";
        return;
    }

    {
        AVFConfigurationLock lock(captureDevice);
        if (!lock) {
            qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock camera for configuration";
            return;
        }
        [captureDevice setFocusModeLockedWithLensPosition:distance completionHandler:nil];
    }
#else
    Q_UNUSED(distance);
#endif
}

void QAVFCameraBase::setFocusDistance(float distance)
{
    if (qFuzzyCompare(focusDistance(), distance))
        return;

    if (!(supportedFeatures() & QCamera::Feature::FocusDistance)) {
        qCWarning(qLcCamera) <<
            Q_FUNC_INFO <<
            "attmpted to set focus-distance on camera without support for FocusDistance feature";
        return;
    }

    if (distance < 0 || distance > 1) {
        qCWarning(qLcCamera) <<
            Q_FUNC_INFO <<
            "attempted to set camera focus-distance with out-of-bounds value";
        return;
    }

    // If we are not currently in FocusModeManual, just accept the
    // value and apply it sometime later during setFocusMode(Manual).
    if (focusMode() == QCamera::FocusModeManual) {
        applyFocusDistanceToAVCaptureDevice(distance);
    }

    focusDistanceChanged(distance);
}

void QAVFCameraBase::updateCameraConfiguration()
{
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "capture device is nil when trying to update QAVFCamera";
        return;
    }

    // We require an active format to update several of the valid ranges. I.e max zoom factor.
    AVCaptureDeviceFormat *activeFormat = captureDevice.activeFormat;
    if (!activeFormat) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "capture device has no active format when trying to update QAVFCamera";
        return;
    }

    // First we gather new capabilities

    // Handle flash/torch capabilities.
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

    flashReadyChanged(isFlashSupported);
#ifdef Q_OS_IOS
    minimumZoomFactorChanged(captureDevice.minAvailableVideoZoomFactor);
    maximumZoomFactorChanged(activeFormat.videoMaxZoomFactor);
#endif // Q_OS_IOS


    // The we apply properties to the camera if they are supported.

    if (minZoomFactor() < maxZoomFactor()) {
        // Zoom is supported, clamp it and apply it.
        //
        // TODO: zoom has no public API to allow instant zooming, only smooth transitions
        // but the Darwin implementation of zoomTo uses rate < 0 to allow this.
        forceZoomTo(qBound(zoomFactor(), minZoomFactor(), maxZoomFactor()), -1);
    }
    applyFlashSettings();

    // Focus mode properties
    // This will also take care of applying FocusDistance if applicable.
    if (isFocusModeSupported(focusMode()))
        forceSetFocusMode(focusMode());


    // Reset properties that are not supported on the new camera device.

    if (minZoomFactor() >= maxZoomFactor())
        zoomFactorChanged(defaultZoomFactor());

    if (!(supportedFeatures() & QCamera::Feature::FocusDistance))
        focusDistanceChanged(defaultFocusDistance());

    if (!isFocusModeSupported(focusMode()))
        focusModeChanged(defaultFocusMode());

    // TODO: Several of the following features have inconsistent behavior
    // and currently do not have any Android implementation. These features
    // should be revised.

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock for configuration";
        return;
    }

    if ([captureDevice isFocusPointOfInterestSupported]) {
        auto point = customFocusPoint();
        const CGPoint focusPOI = CGPointMake(point.x(), point.y());
        [captureDevice setFocusPointOfInterest:focusPOI];
    }

#ifdef Q_OS_IOS
    CMTime newDuration = AVCaptureExposureDurationCurrent;
    bool setCustomMode = false;

    float exposureTime = manualExposureTime();
    if (exposureTime > 0
        && !qt_exposure_duration_equal(captureDevice, exposureTime)) {
        newDuration = CMTimeMakeWithSeconds(exposureTime, captureDevice.exposureDuration.timescale);
        if (!qt_check_exposure_duration(captureDevice, newDuration)) {
            qCDebug(qLcCamera) << Q_FUNC_INFO << "requested exposure duration is out of range";
            return;
        }
        setCustomMode = true;
    }

    float newISO = AVCaptureISOCurrent;
    int iso = manualIsoSensitivity();
    if (iso > 0 && !qt_iso_equal(captureDevice, iso)) {
        newISO = iso;
        if (!qt_check_ISO_value(captureDevice, newISO)) {
            qCDebug(qLcCamera) << Q_FUNC_INFO << "requested ISO value is out of range";
            return;
        }
        setCustomMode = true;
    }

    float bias = exposureCompensation();
    if (bias != 0 && !qt_exposure_bias_equal(captureDevice, bias)) {
        // TODO: mixed fpns.
        if (bias < captureDevice.minExposureTargetBias || bias > captureDevice.maxExposureTargetBias) {
            qCDebug(qLcCamera) << Q_FUNC_INFO << "exposure compensation value is"
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
        qCDebug(qLcCamera) << Q_FUNC_INFO << "requested exposure mode is not supported";
        return;
    }

    captureDevice.exposureMode = avMode;
#endif // Q_OS_IOS
}

// Updates the supportedFeatures() flags based on the current
// AVCaptureDevice.
void QAVFCameraBase::updateSupportedFeatures()
{
    QCamera::Features features;
    AVCaptureDevice *captureDevice = device();

    if (captureDevice) {
        if ([captureDevice isFocusPointOfInterestSupported])
            features |= QCamera::Feature::CustomFocusPoint;

#ifdef Q_OS_IOS
        AVCaptureDeviceFormat *activeFormat = captureDevice.activeFormat;

        // IsoSensitivity
        if ([captureDevice isExposureModeSupported:AVCaptureExposureModeCustom] &&
            activeFormat.minISO < activeFormat.maxISO)
            features |= QCamera::Feature::IsoSensitivity;

        // ColorTemperature
        if (captureDevice.lockingWhiteBalanceWithCustomDeviceGainsSupported)
            features |= QCamera::Feature::ColorTemperature;

        // Exposure compensation
        if ([captureDevice isExposureModeSupported:AVCaptureExposureModeCustom] &&
            captureDevice.minExposureTargetBias < captureDevice.maxExposureTargetBias)
            features |= QCamera::Feature::ExposureCompensation;

        // Manual exposure time
        if ([captureDevice isExposureModeSupported:AVCaptureExposureModeCustom] &&
            CMTimeCompare(activeFormat.minExposureDuration, activeFormat.maxExposureDuration) == -1)
            features |= QCamera::Feature::ManualExposureTime;

        // No point in reporting the feature as supported if we don't also
        // report the corresponding focus-mode as supported.
        if (isFocusModeSupported(QCamera::FocusModeManual) &&
            [captureDevice isLockingFocusWithCustomLensPositionSupported])
            features |= QCamera::Feature::FocusDistance;
#endif
    }

    supportedFeaturesChanged(features);
}

void QAVFCameraBase::zoomTo(float factor, float rate)
{
    if (zoomFactor() == factor)
        return;
    forceZoomTo(factor, rate);
}

// Internal function that gives us the option to run zoomTo
// without skipping early return when factor == QCamera::zoomFactor()
// Useful when switching camera-device.
void QAVFCameraBase::forceZoomTo(float factor, float rate)
{
#ifdef Q_OS_IOS
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice || !captureDevice.activeFormat)
        return;

    factor = qBound(captureDevice.minAvailableVideoZoomFactor, factor,
                    captureDevice.activeFormat.videoMaxZoomFactor);

    {
        const AVFConfigurationLock lock(captureDevice);
        if (!lock) {
            qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock for configuration";
            return;
        }

        if (rate <= 0)
            captureDevice.videoZoomFactor = factor;
        else
            [captureDevice rampToVideoZoomFactor:factor withRate:rate];
    }
    zoomFactorChanged(factor);
#else
    Q_UNUSED(rate);
    Q_UNUSED(factor);
#endif
}

void QAVFCameraBase::setFlashMode(QCamera::FlashMode mode)
{
    if (flashMode() == mode)
        return;

    if (!isFlashModeSupported(mode)) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "unsupported mode" << mode;
        return;
    }

    flashModeChanged(mode);

    if (!isActive())
        return;

    applyFlashSettings();
}

bool QAVFCameraBase::isFlashModeSupported(QCamera::FlashMode mode) const
{
    if (mode == QCamera::FlashOff)
        return true;
    else if (mode == QCamera::FlashOn)
        return isFlashSupported;
    else //if (mode == QCamera::FlashAuto)
        return isFlashAutoSupported;
}

bool QAVFCameraBase::isFlashReady() const
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

    // AVCaptureDevice's docs:
    // "The flash may become unavailable if, for example,
    //  the device overheats and needs to cool off."
    return [captureDevice isFlashAvailable];
}

void QAVFCameraBase::setTorchMode(QCamera::TorchMode mode)
{
    if (torchMode() == mode)
        return;

    if (isActive() && !isTorchModeSupported(mode)) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "unsupported torch mode" << mode;
        return;
    }

    torchModeChanged(mode);

    if (!isActive())
        return;

    applyFlashSettings();
}

bool QAVFCameraBase::isTorchModeSupported(QCamera::TorchMode mode) const
{
    if (mode == QCamera::TorchOff)
        return true;
    else if (mode == QCamera::TorchOn)
        return isTorchSupported;
    else //if (mode == QCamera::TorchAuto)
        return isTorchAutoSupported;
}

void QAVFCameraBase::setExposureMode(QCamera::ExposureMode qtMode)
{
#ifdef Q_OS_IOS
    if (qtMode != QCamera::ExposureAuto && qtMode != QCamera::ExposureManual) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "exposure mode not supported";
        return;
    }

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        exposureModeChanged(qtMode);
        return;
    }

    AVCaptureExposureMode avMode = AVCaptureExposureModeContinuousAutoExposure;
    if (!qt_convert_exposure_mode(captureDevice, qtMode, avMode)) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "exposure mode not supported";
        return;
    }

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock a capture device"
                       << "for configuration";
        return;
    }

    [captureDevice setExposureMode:avMode];
    exposureModeChanged(qtMode);
#else
    Q_UNUSED(qtMode);
#endif
}

bool QAVFCameraBase::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    if (mode == QCamera::ExposureAuto)
        return true;
    if (mode != QCamera::ExposureManual)
        return false;

    AVCaptureDevice *captureDevice = device();
    return captureDevice && [captureDevice isExposureModeSupported:AVCaptureExposureModeCustom];
}

void QAVFCameraBase::applyFlashSettings()
{
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "no capture device found";
        return;
    }

    const AVFConfigurationLock lock(captureDevice);

    if (captureDevice.hasFlash) {
        const auto mode = flashMode();

        auto setAvFlashModeSafe = [&captureDevice](AVCaptureFlashMode avFlashMode) {
            // Note, in some cases captureDevice.hasFlash == false even though
            // no there're no supported flash modes.
            if ([captureDevice isFlashModeSupported:avFlashMode])
                captureDevice.flashMode = avFlashMode;
            else
                qCDebug(qLcCamera) << "Attempt to setup unsupported flash mode " << avFlashMode;
        };

        if (mode == QCamera::FlashOff) {
            setAvFlashModeSafe(AVCaptureFlashModeOff);
        } else {
            if ([captureDevice isFlashAvailable]) {
                if (mode == QCamera::FlashOn)
                    setAvFlashModeSafe(AVCaptureFlashModeOn);
                else if (mode == QCamera::FlashAuto)
                    setAvFlashModeSafe(AVCaptureFlashModeAuto);
            } else {
                qCDebug(qLcCamera) << Q_FUNC_INFO << "flash is not available at the moment";
            }
        }
    }

    if (captureDevice.hasTorch) {
        const auto mode = torchMode();

        auto setAvTorchModeSafe = [&captureDevice](AVCaptureTorchMode avTorchMode) {
            if ([captureDevice isTorchModeSupported:avTorchMode])
                captureDevice.torchMode = avTorchMode;
            else
                qCDebug(qLcCamera) << "Attempt to setup unsupported torch mode " << avTorchMode;
        };

        if (mode == QCamera::TorchOff) {
            setAvTorchModeSafe(AVCaptureTorchModeOff);
        } else {
            if ([captureDevice isTorchAvailable]) {
                if (mode == QCamera::TorchOn)
                    setAvTorchModeSafe(AVCaptureTorchModeOn);
                else if (mode == QCamera::TorchAuto)
                    setAvTorchModeSafe(AVCaptureTorchModeAuto);
            } else {
                qCDebug(qLcCamera) << Q_FUNC_INFO << "torch is not available at the moment";
            }
        }
    }
}


void QAVFCameraBase::setExposureCompensation(float bias)
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
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock for configuration";
        return;
    }

    [captureDevice setExposureTargetBias:bias completionHandler:nil];
    exposureCompensationChanged(bias);
#else
    Q_UNUSED(bias);
#endif
}

void QAVFCameraBase::setManualExposureTime(float value)
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
        qCDebug(qLcCamera) << Q_FUNC_INFO << "shutter speed value is out of range";
        return;
    }

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock for configuration";
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

float QAVFCameraBase::exposureTime() const
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

bool QAVFCameraBase::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    if (mode == QCamera::WhiteBalanceAuto)
        return true;
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return false;
    return [captureDevice isWhiteBalanceModeSupported:AVCaptureWhiteBalanceModeLocked];
}

void QAVFCameraBase::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    if (!isWhiteBalanceModeSupported(mode))
        return;

    AVCaptureDevice *captureDevice = device();
    Q_ASSERT(captureDevice);

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock a capture device"
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

void QAVFCameraBase::setColorTemperature(int colorTemp)
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
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock a capture device"
                       << "for configuration";
        return;
    }

    AVCaptureWhiteBalanceGains wbGains;
    if (avf_convert_temp_and_tint_to_wb_gains(captureDevice, colorTemp, 0., wbGains)
        && avf_set_white_balance_gains(captureDevice, wbGains))
        colorTemperatureChanged(colorTemp);
}
#endif

void QAVFCameraBase::setManualIsoSensitivity(int value)
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
        qCDebug(qLcCamera) << Q_FUNC_INFO << "ISO value is out of range";
        return;
    }

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock a capture device"
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

int QAVFCameraBase::isoSensitivity() const
{
    return manualIsoSensitivity();
}

// Returns true if the application currently has camera permissions.
bool QAVFCameraBase::checkCameraPermission()
{
    const QCameraPermission permission;
    const bool granted = qApp->checkPermission(permission) == Qt::PermissionStatus::Granted;
    if (!granted)
        qWarning() << "Access to camera not granted";

    return granted;
}


#include "moc_qavfcamerabase_p.cpp"
