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

Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDeviceFormat);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureConnection);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDevice);

QT_BEGIN_NAMESPACE
class QPlatformMediaIntegration;

class QAVFVideoDevices : public QPlatformVideoDevices
{
public:
    QAVFVideoDevices(QPlatformMediaIntegration *integration);
    ~QAVFVideoDevices();

    QList<QCameraDevice> videoDevices() const override;

private:
    void updateCameraDevices();

    NSObject *m_deviceConnectedObserver;
    NSObject *m_deviceDisconnectedObserver;

    QList<QCameraDevice> m_cameraDevices;
};


class QAVFCameraBase : public QPlatformCamera
{;
Q_OBJECT
public:
    QAVFCameraBase(QCamera *camera);
    ~QAVFCameraBase();

    bool isActive() const override;
    void setActive(bool activce) override;

    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;

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
    void updateCameraConfiguration();

    // Updates the supported features of the camera.
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
};

QT_END_NAMESPACE

#endif
