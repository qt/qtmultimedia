// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmockcamera.h"

QT_BEGIN_NAMESPACE

QMockCamera::QMockCamera(QCamera *parent)
    : QPlatformCamera(parent), m_propertyChangesSupported(false)
{
    if (!simpleCamera) {
        minIsoChanged(100);
        maxIsoChanged(800);
        minExposureTimeChanged(.001f);
        maxExposureTimeChanged(1.f);
        exposureCompensationRangeChanged(-2, 2);
        maximumZoomFactorChanged(4.);
        setFlashMode(QCamera::FlashAuto);
    }
}

QMockCamera::~QMockCamera() { }

bool QMockCamera::isActive() const
{
    return m_active;
}

void QMockCamera::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    emit activeChanged(active);
}

void QMockCamera::setCamera(const QCameraDevice &camera)
{
    m_camera = camera;
}

bool QMockCamera::setCameraFormat(const QCameraFormat &format)
{
    if (!format.isNull() && !m_camera.videoFormats().contains(format))
        return false;
    return true;
}

void QMockCamera::setFocusMode(QCamera::FocusMode mode)
{
    if (isFocusModeSupported(mode))
        focusModeChanged(mode);
}

bool QMockCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    return simpleCamera ? mode == QCamera::FocusModeAuto : mode != QCamera::FocusModeInfinity;
}

void QMockCamera::setCustomFocusPoint(const QPointF &point)
{
    if (!simpleCamera)
        customFocusPointChanged(point);
}

void QMockCamera::setFocusDistance(float d)
{
    if (!simpleCamera)
        focusDistanceChanged(d);
}

void QMockCamera::zoomTo(float newZoomFactor, float /*rate*/)
{
    zoomFactorChanged(newZoomFactor);
}

void QMockCamera::setFlashMode(QCamera::FlashMode mode)
{
    if (!simpleCamera)
        flashModeChanged(mode);
    flashReadyChanged(mode != QCamera::FlashOff);
}
bool QMockCamera::isFlashModeSupported(QCamera::FlashMode mode) const
{
    return simpleCamera ? mode == QCamera::FlashOff : true;
}

bool QMockCamera::isFlashReady() const
{
    return flashMode() != QCamera::FlashOff;
}

void QMockCamera::setExposureMode(QCamera::ExposureMode mode)
{
    if (!simpleCamera && isExposureModeSupported(mode))
        exposureModeChanged(mode);
}

bool QMockCamera::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    return simpleCamera ? mode == QCamera::ExposureAuto : mode <= QCamera::ExposureBeach;
}

void QMockCamera::setExposureCompensation(float c)
{
    if (!simpleCamera)
        exposureCompensationChanged(qBound(-2., c, 2.));
}

int QMockCamera::isoSensitivity() const
{
    if (simpleCamera)
        return -1;
    return manualIsoSensitivity() > 0 ? manualIsoSensitivity() : 100;
}

void QMockCamera::setManualIsoSensitivity(int iso)
{
    if (!simpleCamera)
        isoSensitivityChanged(qBound(100, iso, 800));
}

void QMockCamera::setManualExposureTime(float secs)
{
    if (!simpleCamera)
        exposureTimeChanged(qBound(0.001, secs, 1.));
}

float QMockCamera::exposureTime() const
{
    if (simpleCamera)
        return -1.;
    return manualExposureTime() > 0 ? manualExposureTime() : .05;
}

bool QMockCamera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    if (simpleCamera)
        return mode == QCamera::WhiteBalanceAuto;
    return mode == QCamera::WhiteBalanceAuto || mode == QCamera::WhiteBalanceManual
            || mode == QCamera::WhiteBalanceSunlight;
}

void QMockCamera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    if (isWhiteBalanceModeSupported(mode))
        whiteBalanceModeChanged(mode);
}

void QMockCamera::setColorTemperature(int temperature)
{
    if (!simpleCamera)
        colorTemperatureChanged(temperature);
}

QT_END_NAMESPACE

#include "moc_qmockcamera.cpp"
