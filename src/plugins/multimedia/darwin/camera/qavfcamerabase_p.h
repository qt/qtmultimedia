// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFCAMERABASE_H
#define QAVFCAMERABASE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>

#include <private/qplatformcamera_p.h>
#include <private/qplatformvideodevices_p.h>

#include <functional>

Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDeviceFormat);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureConnection);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDevice);

QT_BEGIN_NAMESPACE
class QPlatformMediaIntegration;

class QAVFVideoDevices : public QPlatformVideoDevices
{
public:
    // Takes a delegate to check whether a given CvPixelFormat is supported for a capture session.
    // If given a nullptr, it assumes all formats are supported.
    QAVFVideoDevices(
        QPlatformMediaIntegration *integration,
        std::function<bool(uint32_t)> &&isCvPixelFormatSupportedDelegate = nullptr);
    ~QAVFVideoDevices();

    // Returns true if the given CvPixelFormat is supported for camera capture session.
    [[nodiscard]] bool isCvPixelFormatSupported(uint32_t cvPixelFormat) const;

protected:
    QList<QCameraDevice> findVideoInputs() const override;

private:
    void updateCameraDevices();

    NSObject *m_deviceConnectedObserver;
    NSObject *m_deviceDisconnectedObserver;
    std::function<bool(uint32_t)> m_isCvPixelFormatSupportedDelegate;

    QList<QCameraDevice> m_cameraDevices;
};

// The purpose of this class is to provide camera controls on
// both the old native Darwin backend and the FFmpeg backend.
class QAVFCameraBase : public QPlatformCamera
{
Q_OBJECT
public:
    QAVFCameraBase(QCamera *camera);
    ~QAVFCameraBase();

    bool isActive() const override;
    void setActive(bool active) override final;

    void setCamera(const QCameraDevice &camera) override final;
    bool setCameraFormat(const QCameraFormat &format) override final;

    void setFocusMode(QCamera::FocusMode mode) override;

    // FocusModeAuto maps to AVCaptureFocusModeContinuousFocusMode.
    //
    // FocusModeManual does not map to any specific AVCaptureFocusMode
    // value, but rather to the setting setFocusModeLockedWithLensPosition.
    // This setting doesn't actually change the AVCaptureFocusMode but puts it
    // into a state where the AVCaptureFocusMode no longer applies.
    // You can go back into autofocus mode by setting the AVCaptureDevice
    // focus mode.
    bool isFocusModeSupported(QCamera::FocusMode mode) const override;

    void setCustomFocusPoint(const QPointF &point) override;

    void setFocusDistance(float distance) override;
    void zoomTo(float factor, float rate) override;

    void setFlashMode(QCamera::FlashMode mode) override;
    bool isFlashModeSupported(QCamera::FlashMode mode) const override;
    bool isFlashReady() const override;

    void setTorchMode(QCamera::TorchMode mode) override;
    bool isTorchModeSupported(QCamera::TorchMode mode) const override;

    void setExposureMode(QCamera::ExposureMode) override;
    bool isExposureModeSupported(QCamera::ExposureMode mode) const override;

    void setExposureCompensation(float bias) override;
    void setManualIsoSensitivity(int value) override;
    virtual int isoSensitivity() const override;
    void setManualExposureTime(float value) override;
    virtual float exposureTime() const override;

#ifdef Q_OS_IOS
    // not supported on macOS
    bool isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const override;
    void setWhiteBalanceMode(QCamera::WhiteBalanceMode /*mode*/) override;
    void setColorTemperature(int /*temperature*/) override;
#endif

    AVCaptureDevice *device() const;

protected:
    // Called by setActive() when the active status is successfully changed.
    virtual void onActiveChanged(bool active) = 0;
    // Called by setCamera() when the camera is successfully changed.
    virtual void onCameraDeviceChanged(const QCameraDevice &device) = 0;
    // Should be implemented by the backend to apply the camera-format
    // to the physical camera if possible.
    // Returns true if the format was successfully applied.
    [[nodiscard]] virtual bool tryApplyCameraFormat(const QCameraFormat&) = 0;

    bool checkCameraPermission();

    void updateCameraConfiguration();
    void updateSupportedFeatures();
    void applyFlashSettings();

    // Applies the focusDistance to the AVCaptureDevice.
    // Does NOT trigger focusDistanceChanged
    void applyFocusDistanceToAVCaptureDevice(float distance);

    QCameraDevice m_cameraDevice;
    bool m_active = false;
private:
    bool isFlashSupported = false;
    bool isFlashAutoSupported = false;
    bool isTorchSupported = false;
    bool isTorchAutoSupported = false;

    void forceSetFocusMode(QCamera::FocusMode mode);
    void forceZoomTo(float factor, float rate);
};

QT_END_NAMESPACE

#endif
