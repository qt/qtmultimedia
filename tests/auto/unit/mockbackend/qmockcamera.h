// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMOCKCAMERA_H
#define QMOCKCAMERA_H

#include "private/qplatformcamera_p.h"
#include "qcameradevice.h"
#include <qtimer.h>

QT_BEGIN_NAMESPACE

class QMockCamera : public QPlatformCamera
{
    friend class MockCaptureControl;
    Q_OBJECT

    static bool simpleCamera;
public:

    struct Simple {
        Simple() { simpleCamera = true; }
        ~Simple() { simpleCamera = false; }
    };

    QMockCamera(QCamera *parent);

    ~QMockCamera() override;

    bool isActive() const override;

    void setActive(bool active) override;

    void setCamera(const QCameraDevice &camera) override;

    bool setCameraFormat(const QCameraFormat &format) override;

    void setFocusMode(QCamera::FocusMode mode) override;

    bool isFocusModeSupported(QCamera::FocusMode mode) const override;

    void setCustomFocusPoint(const QPointF &point) override;

    void setFocusDistance(float d) override;

    void zoomTo(float newZoomFactor, float /*rate*/) override;

    void setFlashMode(QCamera::FlashMode mode) override;

    bool isFlashModeSupported(QCamera::FlashMode mode) const override;

    bool isFlashReady() const override;

    void setExposureMode(QCamera::ExposureMode mode) override;

    bool isExposureModeSupported(QCamera::ExposureMode mode) const override;

    void setExposureCompensation(float c) override;

    int isoSensitivity() const override;

    void setManualIsoSensitivity(int iso) override;

    void setManualExposureTime(float secs) override;

    float exposureTime() const override;

    bool isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const override;

    void setWhiteBalanceMode(QCamera::WhiteBalanceMode mode) override;

    void setColorTemperature(int temperature) override;

    bool m_active = false;
    QCameraDevice m_camera;
    bool m_propertyChangesSupported;
};

QT_END_NAMESPACE

#endif // QMOCKCAMERA_H
