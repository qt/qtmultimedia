// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMCAMERA_H
#define QPLATFORMCAMERA_H

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

#include "qplatformvideosource_p.h"

#include <QtMultimedia/qcamera.h>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QPlatformCamera : public QPlatformVideoSource
{
    Q_OBJECT

public:
    virtual void setCamera(const QCameraDevice &camera) = 0;
    virtual bool setCameraFormat(const QCameraFormat &/*format*/) { return false; }
    QCameraFormat cameraFormat() const { return m_cameraFormat; }

    virtual bool isFocusModeSupported(QCamera::FocusMode mode) const { return mode == QCamera::FocusModeAuto; }
    virtual void setFocusMode(QCamera::FocusMode /*mode*/) {}

    virtual void setCustomFocusPoint(const QPointF &/*point*/) {}

    virtual void setFocusDistance(float) {}

    // smaller 0: zoom instantly, rate in power-of-two/sec
    virtual void zoomTo(float /*newZoomFactor*/, float /*rate*/ = -1.) {}

    virtual void setFlashMode(QCamera::FlashMode /*mode*/) {}
    virtual bool isFlashModeSupported(QCamera::FlashMode mode) const { return mode == QCamera::FlashOff; }
    virtual bool isFlashReady() const { return false; }

    virtual void setTorchMode(QCamera::TorchMode /*mode*/) {}
    virtual bool isTorchModeSupported(QCamera::TorchMode mode) const { return mode == QCamera::TorchOff; }

    virtual void setExposureMode(QCamera::ExposureMode) {}
    virtual bool isExposureModeSupported(QCamera::ExposureMode mode) const { return mode == QCamera::ExposureAuto; }
    virtual void setExposureCompensation(float) {}
    virtual int isoSensitivity() const { return 100; }
    virtual void setManualIsoSensitivity(int) {}
    virtual void setManualExposureTime(float) {}
    virtual float exposureTime() const { return -1.; }

    virtual bool isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const { return mode == QCamera::WhiteBalanceAuto; }
    virtual void setWhiteBalanceMode(QCamera::WhiteBalanceMode /*mode*/) {}
    virtual void setColorTemperature(int /*temperature*/) {}

    QVideoFrameFormat frameFormat() const override;

    QCamera::Features supportedFeatures() const { return m_supportedFeatures; }

    QCamera::FocusMode focusMode() const { return m_focusMode; }
    QPointF focusPoint() const { return m_customFocusPoint; }

    float minZoomFactor() const { return m_minZoom; }
    float maxZoomFactor() const { return m_maxZoom; }
    float zoomFactor() const { return m_zoomFactor; }
    QPointF customFocusPoint() const { return m_customFocusPoint; }
    float focusDistance() const { return m_focusDistance; }

    QCamera::FlashMode flashMode() const { return m_flashMode; }
    QCamera::TorchMode torchMode() const { return m_torchMode; }

    QCamera::ExposureMode exposureMode() const { return m_exposureMode; }
    float exposureCompensation() const { return m_exposureCompensation; }
    int manualIsoSensitivity() const { return m_iso; }
    int minIso() const { return m_minIso; }
    int maxIso() const { return m_maxIso; }
    float manualExposureTime() const { return m_exposureTime; }
    float minExposureTime() const { return m_minExposureTime; }
    float maxExposureTime() const { return m_maxExposureTime; }
    QCamera::WhiteBalanceMode whiteBalanceMode() const { return m_whiteBalance; }
    int colorTemperature() const { return m_colorTemperature; }

    void supportedFeaturesChanged(QCamera::Features);
    void minimumZoomFactorChanged(float factor);
    void maximumZoomFactorChanged(float);
    void focusModeChanged(QCamera::FocusMode mode);
    void customFocusPointChanged(const QPointF &point);
    void focusDistanceChanged(float d);
    void zoomFactorChanged(float zoom);
    void flashReadyChanged(bool);
    void flashModeChanged(QCamera::FlashMode mode);
    void torchModeChanged(QCamera::TorchMode mode);
    void exposureModeChanged(QCamera::ExposureMode mode);
    void exposureCompensationChanged(float compensation);
    void exposureCompensationRangeChanged(float min, float max);
    void isoSensitivityChanged(int iso);
    void minIsoChanged(int iso) { m_minIso = iso; }
    void maxIsoChanged(int iso) { m_maxIso = iso; }
    void exposureTimeChanged(float speed);
    void minExposureTimeChanged(float secs) { m_minExposureTime = secs; }
    void maxExposureTimeChanged(float secs) { m_maxExposureTime = secs; }
    void whiteBalanceModeChanged(QCamera::WhiteBalanceMode mode);
    void colorTemperatureChanged(int temperature);

    static int colorTemperatureForWhiteBalance(QCamera::WhiteBalanceMode mode);

Q_SIGNALS:
    void error(int error, const QString &errorString);

protected:
    explicit QPlatformCamera(QCamera *parent);

    virtual int cameraPixelFormatScore(QVideoFrameFormat::PixelFormat /*format*/,
                                       QVideoFrameFormat::ColorRange /*colorRange*/) const
    {
        return 0;
    }

    QCameraFormat findBestCameraFormat(const QCameraDevice &camera) const;
    QCameraFormat m_cameraFormat;
    QVideoFrameFormat::PixelFormat m_framePixelFormat = QVideoFrameFormat::Format_Invalid;

private:
    QCamera *m_camera = nullptr;
    QCamera::Features m_supportedFeatures = {};
    QCamera::FocusMode m_focusMode = QCamera::FocusModeAuto;
    float m_minZoom = 1.;
    float m_maxZoom = 1.;
    float m_zoomFactor = 1.;
    float m_focusDistance = 1.;
    QPointF m_customFocusPoint{-1, -1};
    bool m_flashReady = false;
    QCamera::FlashMode m_flashMode = QCamera::FlashOff;
    QCamera::TorchMode m_torchMode = QCamera::TorchOff;
    QCamera::ExposureMode m_exposureMode = QCamera::ExposureAuto;
    float m_exposureCompensation = 0.;
    float m_minExposureCompensation = 0.;
    float m_maxExposureCompensation = 0.;
    int m_iso = -1;
    int m_minIso = -1;
    int m_maxIso = -1;
    float m_exposureTime = -1.;
    float m_minExposureTime = -1.;
    float m_maxExposureTime = -1.;
    QCamera::WhiteBalanceMode m_whiteBalance = QCamera::WhiteBalanceAuto;
    int m_colorTemperature = 0;
};

QT_END_NAMESPACE


#endif  // QPLATFORMCAMERA_H

