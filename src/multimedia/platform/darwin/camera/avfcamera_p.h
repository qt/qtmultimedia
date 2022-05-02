/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef AVFCAMERA_H
#define AVFCAMERA_H

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

Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDeviceFormat);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureConnection);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDevice);

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;
class AVFCameraSession;

class AVFCamera : public QPlatformCamera
{
Q_OBJECT
public:
    AVFCamera(QCamera *camera);
    ~AVFCamera();

    bool isActive() const override;
    void setActive(bool activce) override;

    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;

    void setCaptureSession(QPlatformMediaCaptureSession *) override;

    void setFocusMode(QCamera::FocusMode mode) override;
    bool isFocusModeSupported(QCamera::FocusMode mode) const override;

    void setCustomFocusPoint(const QPointF &point) override;

    void setFocusDistance(float d) override;
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

    AVCaptureConnection *videoConnection() const;
    AVCaptureDevice *device() const;

private:
    void updateCameraConfiguration();
    void updateCameraProperties();
    void applyFlashSettings();

    friend class AVFCameraSession;
    AVFCameraService *m_service = nullptr;
    AVFCameraSession *m_session = nullptr;

    QCameraDevice m_cameraDevice;
    QCameraFormat m_cameraFormat;

    bool m_active = false;

    bool isFlashSupported = false;
    bool isFlashAutoSupported = false;
    bool isTorchSupported = false;
    bool isTorchAutoSupported = false;
};

QT_END_NAMESPACE

#endif
