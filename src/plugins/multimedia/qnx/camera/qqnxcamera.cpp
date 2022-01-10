/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#include "qqnxcamera_p.h"

#include "qqnxmediacapture_p.h"
#include <qcameradevice.h>
#include <qmediadevices.h>

#include <camera/camera_api.h>
#include <camera/camera_3a.h>

QT_BEGIN_NAMESPACE

QQnxCamera::QQnxCamera(QCamera *parent)
    : QPlatformCamera(parent)
{
    m_camera = QMediaDevices::defaultVideoInput();
}

bool QQnxCamera::isActive() const
{
    return m_handle != CAMERA_HANDLE_INVALID;
}

void QQnxCamera::setActive(bool active)
{
    if (active) {
        if (m_handle)
            return;
        auto error = camera_open(m_cameraUnit, CAMERA_MODE_RO|CAMERA_MODE_PWRITE, &m_handle);
        if (error != CAMERA_EOK) {
            qWarning() << "Failed to open camera" << error;
            return;
        }
    } else {
        if (!m_handle)
            return;
        auto error = camera_close(m_handle);
        m_handle = CAMERA_HANDLE_INVALID;
        if (error != CAMERA_EOK) {
            qWarning() << "Failed to close camera" << error;
            return;
        }
    }

    updateCameraFeatures();
}

void QQnxCamera::setCamera(const QCameraDevice &camera)
{
    if (m_camera == camera)
        return;
    m_camera = camera;
    m_cameraUnit = camera_unit_t(camera.id().toUInt());
}

void QQnxCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    if (m_session == session)
        return;
    m_session = static_cast<QQnxMediaCaptureSession *>(session);
}

camera_focusmode_t qnxFocusMode(QCamera::FocusMode mode)
{
    switch (mode) {
    default:
    case QCamera::FocusModeAuto:
    case QCamera::FocusModeAutoFar:
    case QCamera::FocusModeInfinity:
        return CAMERA_FOCUSMODE_CONTINUOUS_AUTO;
    case QCamera::FocusModeAutoNear:
        return CAMERA_FOCUSMODE_CONTINUOUS_MACRO;
    case QCamera::FocusModeHyperfocal:
        return CAMERA_FOCUSMODE_EDOF;
    case QCamera::FocusModeManual:
        return CAMERA_FOCUSMODE_MANUAL;
    }
}

bool QQnxCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    if (!m_handle)
        return false;

    camera_focusmode_t focusModes[CAMERA_FOCUSMODE_NUMFOCUSMODES];
    int nFocusModes = 0;
    auto error = camera_get_focus_modes(m_handle, CAMERA_FOCUSMODE_NUMFOCUSMODES, &nFocusModes, focusModes);
    if (error != CAMERA_EOK || nFocusModes == 0)
        return false;

    auto qnxMode = qnxFocusMode(mode);
    for (int i = 0; i < nFocusModes; ++i) {
        if (focusModes[i] == qnxMode)
            return true;
    }
    return false;
}

void QQnxCamera::setFocusMode(QCamera::FocusMode mode)
{
    if (!m_handle)
        return;

    auto qnxMode = qnxFocusMode(mode);
    const camera_error_t result = camera_set_focus_mode(m_handle, qnxMode);

    if (result != CAMERA_EOK) {
        qWarning() << "Unable to set focus mode:" << result;
        return;
    }

    focusModeChanged(mode);
}

void QQnxCamera::setCustomFocusPoint(const QPointF &point)
{
    // get the size of the viewfinder
    int width = 0;
    int height = 0;
    auto result = camera_get_vf_property(m_handle,
                                        CAMERA_IMGPROP_WIDTH, width,
                                        CAMERA_IMGPROP_HEIGHT, height);
    if (result != CAMERA_EOK)
        return;

    // define a 40x40 pixel focus region around the custom focus point
    camera_region_t focusRegion;
    focusRegion.left = qMax(0, static_cast<int>(point.x() * width) - 20);
    focusRegion.top = qMax(0, static_cast<int>(point.y() * height) - 20);
    focusRegion.width = 40;
    focusRegion.height = 40;

    result = camera_set_focus_regions(m_handle, 1, &focusRegion);
    if (result != CAMERA_EOK) {
        qWarning() << "Unable to set focus region:" << result;
        return;
    }
    auto qnxMode = qnxFocusMode(focusMode());
    result = camera_set_focus_mode(m_handle, qnxMode);
    if (result != CAMERA_EOK) {
        qWarning() << "Unable to set focus region:" << result;
        return;
    }
    customFocusPointChanged(point);
}

void QQnxCamera::zoomTo(float factor, float)
{
    if (maxZoom <= minZoom)
        return;
    // QNX has an integer based API. Interpolate between the levels according to the factor we get
    float max = maxZoomFactor();
    float min = minZoomFactor();
    if (max <= min)
        return;
    factor = qBound(min, factor, max) - min;
    uint zoom = minZoom + (uint)qRound(factor*(maxZoom - minZoom)/(max - min));

    auto error = camera_set_vf_property(m_handle, CAMERA_IMGPROP_ZOOMFACTOR, zoom);
    if (error == CAMERA_EOK)
        zoomFactorChanged(factor);
}

bool QQnxCamera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    if (!whiteBalanceModesChecked) {
        whiteBalanceModesChecked = true;
        unsigned numWhiteBalanceValues = 0;
        auto error = camera_get_supported_manual_white_balance_values(m_handle, 0, &numWhiteBalanceValues,
                                                                      nullptr, &continuousColorTemperatureSupported);
        if (error == CAMERA_EOK) {
            manualColorTemperatureValues.resize(numWhiteBalanceValues);
            auto error = camera_get_supported_manual_white_balance_values(m_handle, numWhiteBalanceValues, &numWhiteBalanceValues,
                                                                          manualColorTemperatureValues.data(),
                                                                          &continuousColorTemperatureSupported);

            minColorTemperature = 1024*1014; // large enough :)
            for (int temp : qAsConst(manualColorTemperatureValues)) {
                minColorTemperature = qMin(minColorTemperature, temp);
                maxColorTemperature = qMax(maxColorTemperature, temp);
            }
        } else {
            maxColorTemperature = 0;
        }
    }

    if (maxColorTemperature != 0)
        return true;
    return mode == QCamera::WhiteBalanceAuto;
}

void QQnxCamera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    if (mode == QCamera::WhiteBalanceAuto) {
        camera_set_whitebalance_mode(m_handle, CAMERA_WHITEBALANCEMODE_AUTO);
        return;
    }
    camera_set_whitebalance_mode(m_handle, CAMERA_WHITEBALANCEMODE_MANUAL);
    setColorTemperature(colorTemperatureForWhiteBalance(mode));
}

void QQnxCamera::setColorTemperature(int temperature)
{

    if (maxColorTemperature == 0)
        return;

    unsigned bestTemp = 0;
    if (!continuousColorTemperatureSupported) {
        // find the closest match
        int delta = 1024*1024;
        for (unsigned temp : qAsConst(manualColorTemperatureValues)) {
            int d = qAbs(int(temp) - temperature);
            if (d < delta) {
                bestTemp = temp;
                delta = d;
            }
        }
    } else {
        bestTemp = (unsigned)qBound(minColorTemperature, temperature, maxColorTemperature);
    }

    auto error = camera_set_manual_white_balance(m_handle, bestTemp);
}

camera_handle_t QQnxCamera::handle() const
{
    return m_handle;
}

void QQnxCamera::updateCameraFeatures()
{
    whiteBalanceModesChecked = false;

    bool smooth;
    auto error = camera_get_zoom_limits(m_handle, &minZoom, &maxZoom, &smooth);
    if (error == CAMERA_EOK) {
        double level;
        camera_get_zoom_ratio_from_zoom_level(m_handle, minZoom, &level);
        minimumZoomFactorChanged(level);
        camera_get_zoom_ratio_from_zoom_level(m_handle, maxZoom, &level);
        maximumZoomFactorChanged(level);
    } else {
        minZoom = maxZoom = 1;
    }

    QCamera::Features features = {};

    if (camera_has_feature(m_handle, CAMERA_FEATURE_REGIONFOCUS))
        features |= QCamera::Feature::CustomFocusPoint;

    minimumZoomFactorChanged(minZoom);
    maximumZoomFactorChanged(maxZoom);
    supportedFeaturesChanged(features);
}

QT_END_NAMESPACE
