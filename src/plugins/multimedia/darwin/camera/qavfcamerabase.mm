// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfcameradebug_p.h"
#include "qavfcamerabase_p.h"
#include "avfcamerautility_p.h"
#include <private/qcameradevice_p.h>
#include "qavfhelpers_p.h"
#include <private/qplatformmediaintegration_p.h>
#include <QtCore/qset.h>

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


QAVFVideoDevices::QAVFVideoDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration)
{
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    m_deviceConnectedObserver = [notificationCenter addObserverForName:AVCaptureDeviceWasConnectedNotification
                                                                object:nil
                                                                queue:[NSOperationQueue mainQueue]
                                                                usingBlock:^(NSNotification *) {
                                                                        this->updateCameraDevices();
                                                                }];

    m_deviceDisconnectedObserver = [notificationCenter addObserverForName:AVCaptureDeviceWasDisconnectedNotification
                                                                object:nil
                                                                queue:[NSOperationQueue mainQueue]
                                                                usingBlock:^(NSNotification *) {
                                                                        this->updateCameraDevices();
                                                                }];
    updateCameraDevices();
}

QAVFVideoDevices::~QAVFVideoDevices()
{
    NSNotificationCenter* notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter removeObserver:(id)m_deviceConnectedObserver];
    [notificationCenter removeObserver:(id)m_deviceDisconnectedObserver];
}

QList<QCameraDevice> QAVFVideoDevices::videoDevices() const
{
    return m_cameraDevices;
}

void QAVFVideoDevices::updateCameraDevices()
{
#ifdef Q_OS_IOS
    // Cameras can't change dynamically on iOS. Update only once.
    if (!m_cameraDevices.isEmpty())
        return;
#endif

    QList<QCameraDevice> cameras;

    AVCaptureDevice *defaultDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    NSArray *videoDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];

    for (AVCaptureDevice *device in videoDevices) {
        auto info = std::make_unique<QCameraDevicePrivate>();
        if (defaultDevice && [defaultDevice.uniqueID isEqualToString:device.uniqueID])
            info->isDefault = true;
        info->id = QByteArray([[device uniqueID] UTF8String]);
        info->description = QString::fromNSString([device localizedName]);

        qCDebug(qLcCamera) << "Handling camera info" << info->description
                           << (info->isDefault ? "(default)" : "");

        QSet<QSize> photoResolutions;
        QList<QCameraFormat> videoFormats;

        for (AVCaptureDeviceFormat *format in device.formats) {
            if (![format.mediaType isEqualToString:AVMediaTypeVideo])
                continue;

            auto dimensions = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
            QSize resolution(dimensions.width, dimensions.height);
            photoResolutions.insert(resolution);

            float maxFrameRate = 0;
            float minFrameRate = 1.e6;

            auto encoding = CMVideoFormatDescriptionGetCodecType(format.formatDescription);
            auto pixelFormat = QAVFHelpers::fromCVPixelFormat(encoding);
            auto colorRange = QAVFHelpers::colorRangeForCVPixelFormat(encoding);
            // Ignore pixel formats we can't handle
            if (pixelFormat == QVideoFrameFormat::Format_Invalid) {
                qCDebug(qLcCamera) << "ignore camera CV format" << encoding
                                   << "as no matching video format found";
                continue;
            }

            for (AVFrameRateRange *frameRateRange in format.videoSupportedFrameRateRanges) {
                if (frameRateRange.minFrameRate < minFrameRate)
                    minFrameRate = frameRateRange.minFrameRate;
                if (frameRateRange.maxFrameRate > maxFrameRate)
                    maxFrameRate = frameRateRange.maxFrameRate;
            }

#ifdef Q_OS_IOS
            // From Apple's docs (iOS):
            // By default, AVCaptureStillImageOutput emits images with the same dimensions as
            // its source AVCaptureDevice instance’s activeFormat.formatDescription. However,
            // if you set this property to YES, the receiver emits still images at the capture
            // device’s highResolutionStillImageDimensions value.
            const QSize hrRes(qt_device_format_high_resolution(format));
            if (!hrRes.isNull() && hrRes.isValid())
                photoResolutions.insert(hrRes);
#endif

            qCDebug(qLcCamera) << "Add camera format. pixelFormat:" << pixelFormat
                               << "colorRange:" << colorRange << "cvPixelFormat" << encoding
                               << "resolution:" << resolution << "frameRate: [" << minFrameRate
                               << maxFrameRate << "]";

            auto *f = new QCameraFormatPrivate{ QSharedData(), pixelFormat,  resolution,
                                                minFrameRate,  maxFrameRate, colorRange };
            videoFormats << f->create();
        }
        if (videoFormats.isEmpty()) {
            // skip broken cameras without valid formats
            qCWarning(qLcCamera())
                    << "Skip camera" << info->description << "without supported formats";
            continue;
        }
        info->videoFormats = videoFormats;
        info->photoResolutions = photoResolutions.values();

        cameras.append(info.release()->create());
    }

    if (cameras != m_cameraDevices) {
        m_cameraDevices = cameras;
        emit videoInputsChanged();
    }
}


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

    m_active = active;

    if (active)
        updateCameraConfiguration();
    Q_EMIT activeChanged(m_active);
}

void QAVFCameraBase::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;
    m_cameraDevice = camera;
    setCameraFormat({});
}

bool QAVFCameraBase::setCameraFormat(const QCameraFormat &format)
{
    if (!format.isNull() && !m_cameraDevice.videoFormats().contains(format))
        return false;

    m_cameraFormat = format.isNull() ? findBestCameraFormat(m_cameraDevice) : format;

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

void QAVFCameraBase::setFocusMode(QCamera::FocusMode mode)
{
#ifdef Q_OS_IOS
    if (focusMode() == mode)
        return;

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

    if (isFocusModeSupported(mode)) {
        const AVFConfigurationLock lock(captureDevice);
        if (!lock) {
            qCDebug(qLcCamera) << Q_FUNC_INFO
                           << "failed to lock for configuration";
            return;
        }

        captureDevice.focusMode = avf_focus_mode(mode);
    } else {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "focus mode not supported";
        return;
    }

    Q_EMIT focusModeChanged(mode);
#else
    Q_UNUSED(mode);
#endif
}

bool QAVFCameraBase::isFocusModeSupported(QCamera::FocusMode mode) const
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

void QAVFCameraBase::setFocusDistance(float d)
{
#ifdef Q_OS_IOS
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return;

    if (captureDevice.lockingFocusWithCustomLensPositionSupported) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "Setting custom focus distance not supported\n";
        return;
    }

    {
        AVFConfigurationLock lock(captureDevice);
        if (!lock) {
            qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock for configuration";
            return;
        }
        [captureDevice setFocusModeLockedWithLensPosition:d completionHandler:nil];
    }
    focusDistanceChanged(d);
#else
    Q_UNUSED(d);
#endif
}

void QAVFCameraBase::updateCameraConfiguration()
{
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "capture device is nil in 'active' state";
        return;
    }

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
    if (focusMode() != QCamera::FocusModeAuto) {
        const AVCaptureFocusMode avMode = avf_focus_mode(focusMode());
        if (captureDevice.focusMode != avMode) {
            if ([captureDevice isFocusModeSupported:avMode]) {
                [captureDevice setFocusMode:avMode];
            } else {
                qCDebug(qLcCamera) << Q_FUNC_INFO << "focus mode not supported";
            }
        }
    }

    if (!captureDevice.activeFormat) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "camera state is active, but active format is nil";
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

void QAVFCameraBase::updateCameraProperties()
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

void QAVFCameraBase::zoomTo(float factor, float rate)
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
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock for configuration";
        return;
    }

   if (rate <= 0)
        captureDevice.videoZoomFactor = factor;
   else
       [captureDevice rampToVideoZoomFactor:factor withRate:rate];
#endif
}

void QAVFCameraBase::setFlashMode(QCamera::FlashMode mode)
{
    if (flashMode() == mode)
        return;

    if (isActive() && !isFlashModeSupported(mode)) {
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

    if (@available(macOS 10.15, *)) {
        AVCaptureDevice *captureDevice = device();
        return captureDevice && [captureDevice isExposureModeSupported:AVCaptureExposureModeCustom];
    }

    return false;
}

void QAVFCameraBase::applyFlashSettings()
{
    Q_ASSERT(isActive());

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


#include "moc_qavfcamerabase_p.cpp"
