/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

    QMockCamera(QCamera *parent)
        : QPlatformCamera(parent),
          m_propertyChangesSupported(false)
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

    ~QMockCamera() {}

    bool isActive() const override { return m_active; }
    void setActive(bool active) override {
        if (m_active == active)
            return;
        m_active = active;
        emit activeChanged(active);
    }

    /* helper method to emit the signal error */
    void setError(QCamera::Error err, QString errorString)
    {
        emit error(err, errorString);
    }

    void setCamera(const QCameraDevice &camera) override
    {
        m_camera = camera;
    }

    bool setCameraFormat(const QCameraFormat& format) override
    {
        if (!format.isNull() && !m_camera.videoFormats().contains(format))
            return false;
        return true;
    }

    void setFocusMode(QCamera::FocusMode mode) override
    {
        if (isFocusModeSupported(mode))
            focusModeChanged(mode);
    }
    bool isFocusModeSupported(QCamera::FocusMode mode) const override
    { return simpleCamera ? mode == QCamera::FocusModeAuto : mode != QCamera::FocusModeInfinity; }

    void setCustomFocusPoint(const QPointF &point) override
    {
        if (!simpleCamera)
            customFocusPointChanged(point);
    }

    void setFocusDistance(float d) override
    {
        if (!simpleCamera)
            focusDistanceChanged(d);
    }

    void zoomTo(float newZoomFactor, float /*rate*/) override { zoomFactorChanged(newZoomFactor); }

    void setFlashMode(QCamera::FlashMode mode) override
    {
        if (!simpleCamera)
            flashModeChanged(mode);
        flashReadyChanged(mode != QCamera::FlashOff);
    }
    bool isFlashModeSupported(QCamera::FlashMode mode) const override { return simpleCamera ? mode == QCamera::FlashOff : true; }
    bool isFlashReady() const override { return flashMode() != QCamera::FlashOff; }

    void setExposureMode(QCamera::ExposureMode mode) override
    {
        if (!simpleCamera && isExposureModeSupported(mode))
            exposureModeChanged(mode);
    }
    bool isExposureModeSupported(QCamera::ExposureMode mode) const override
    {
        return simpleCamera ? mode == QCamera::ExposureAuto : mode <= QCamera::ExposureBeach;
    }
    void setExposureCompensation(float c) override
    {
        if (!simpleCamera)
            exposureCompensationChanged(qBound(-2., c, 2.));
    }
    int isoSensitivity() const override
    {
        if (simpleCamera)
            return -1;
        return manualIsoSensitivity() > 0 ? manualIsoSensitivity() : 100;
    }
    void setManualIsoSensitivity(int iso) override
    {
        if (!simpleCamera)
            isoSensitivityChanged(qBound(100, iso, 800));
    }
    void setManualExposureTime(float secs) override
    {
        if (!simpleCamera)
            exposureTimeChanged(qBound(0.001, secs, 1.));
    }
    float exposureTime() const override
    {
        if (simpleCamera)
            return -1.;
        return manualExposureTime() > 0 ? manualExposureTime() : .05;
    }

    bool isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const override
    {
        if (simpleCamera)
            return mode == QCamera::WhiteBalanceAuto;
        return mode == QCamera::WhiteBalanceAuto ||
               mode == QCamera::WhiteBalanceManual ||
               mode == QCamera::WhiteBalanceSunlight;
    }
    void setWhiteBalanceMode(QCamera::WhiteBalanceMode mode) override
    {
        if (isWhiteBalanceModeSupported(mode))
            whiteBalanceModeChanged(mode);
    }
    void setColorTemperature(int temperature) override
    {
        if (!simpleCamera)
            colorTemperatureChanged(temperature);
    }

    bool m_active = false;
    QCameraDevice m_camera;
    bool m_propertyChangesSupported;
};

QT_END_NAMESPACE

#endif // QMOCKCAMERA_H
