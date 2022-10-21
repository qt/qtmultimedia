/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qplatformcamera_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformCamera
    \obsolete



    \brief The QPlatformCamera class is an abstract base class for
    classes that control still cameras or video cameras.

    \inmodule QtMultimedia

    \ingroup multimedia_control
*/

/*!
    Constructs a camera control object with \a parent.
*/

QPlatformCamera::QPlatformCamera(QCamera *parent)
  : QObject(parent),
    m_camera(parent)
{
}

QCameraFormat QPlatformCamera::findBestCameraFormat(const QCameraDevice &camera)
{
    QCameraFormat f;
    const auto formats = camera.videoFormats();
    for (const auto &fmt : formats) {
        // check if fmt is better. We try to find the highest resolution that offers
        // at least 30 FPS
        // we use 29 FPS to compare against as some cameras report 29.97 FPS...
        if (f.maxFrameRate() < 29 && fmt.maxFrameRate() > f.maxFrameRate())
            f = fmt;
        else if (f.maxFrameRate() == fmt.maxFrameRate() &&
                 f.resolution().width()*f.resolution().height() < fmt.resolution().width()*fmt.resolution().height())
            f = fmt;
    }
    return f;
}

/*!
    \fn void QPlatformCamera::error(int error, const QString &errorString)

    Signal emitted when an error occurs with error code \a error and
    a description of the error \a errorString.
*/

void QPlatformCamera::supportedFeaturesChanged(QCamera::Features f)
{
    if (m_supportedFeatures == f)
        return;
    m_supportedFeatures = f;
    emit m_camera->supportedFeaturesChanged();
}

void QPlatformCamera::minimumZoomFactorChanged(float factor)
{
    if (m_minZoom == factor)
        return;
    m_minZoom = factor;
    emit m_camera->minimumZoomFactorChanged(factor);
}

void QPlatformCamera::maximumZoomFactorChanged(float factor)
{
    if (m_maxZoom == factor)
        return;
    m_maxZoom = factor;
    emit m_camera->maximumZoomFactorChanged(factor);
}

void QPlatformCamera::focusModeChanged(QCamera::FocusMode mode)
{
    if (m_focusMode == mode)
        return;
    m_focusMode = mode;
    emit m_camera->focusModeChanged();
}

void QPlatformCamera::customFocusPointChanged(const QPointF &point)
{
    if (m_customFocusPoint == point)
        return;
    m_customFocusPoint = point;
    emit m_camera->customFocusPointChanged();
}


void QPlatformCamera::zoomFactorChanged(float zoom)
{
    if (m_zoomFactor == zoom)
        return;
    m_zoomFactor = zoom;
    emit m_camera->zoomFactorChanged(zoom);
}


void QPlatformCamera::focusDistanceChanged(float d)
{
    if (m_focusDistance == d)
        return;
    m_focusDistance = d;
    emit m_camera->focusDistanceChanged(m_focusDistance);
}


void QPlatformCamera::flashReadyChanged(bool ready)
{
    if (m_flashReady == ready)
        return;
    m_flashReady = ready;
    emit m_camera->flashReady(m_flashReady);
}

void QPlatformCamera::flashModeChanged(QCamera::FlashMode mode)
{
    if (m_flashMode == mode)
        return;
    m_flashMode = mode;
    emit m_camera->flashModeChanged();
}

void QPlatformCamera::torchModeChanged(QCamera::TorchMode mode)
{
    if (m_torchMode == mode)
        return;
    m_torchMode = mode;
    emit m_camera->torchModeChanged();
}

void QPlatformCamera::exposureModeChanged(QCamera::ExposureMode mode)
{
    if (m_exposureMode == mode)
        return;
    m_exposureMode = mode;
    emit m_camera->exposureModeChanged();
}

void QPlatformCamera::exposureCompensationChanged(float compensation)
{
    if (m_exposureCompensation == compensation)
        return;
    m_exposureCompensation = compensation;
    emit m_camera->exposureCompensationChanged(compensation);
}

void QPlatformCamera::exposureCompensationRangeChanged(float min, float max)
{
    if (m_minExposureCompensation == min && m_maxExposureCompensation == max)
        return;
    m_minExposureCompensation = min;
    m_maxExposureCompensation = max;
    // tell frontend
}

void QPlatformCamera::isoSensitivityChanged(int iso)
{
    if (m_iso == iso)
        return;
    m_iso = iso;
    emit m_camera->isoSensitivityChanged(iso);
}

void QPlatformCamera::exposureTimeChanged(float speed)
{
    if (m_exposureTime == speed)
        return;
    m_exposureTime = speed;
    emit m_camera->exposureTimeChanged(speed);
}

void QPlatformCamera::whiteBalanceModeChanged(QCamera::WhiteBalanceMode mode)
{
    if (m_whiteBalance == mode)
        return;
    m_whiteBalance = mode;
    emit m_camera->whiteBalanceModeChanged();
}

void QPlatformCamera::colorTemperatureChanged(int temperature)
{
    Q_ASSERT(temperature >= 0);
    Q_ASSERT((temperature > 0 && whiteBalanceMode() == QCamera::WhiteBalanceManual) ||
             (temperature == 0 && whiteBalanceMode() == QCamera::WhiteBalanceAuto));
    if (m_colorTemperature == temperature)
        return;
    m_colorTemperature = temperature;
    emit m_camera->colorTemperatureChanged();
}

int QPlatformCamera::colorTemperatureForWhiteBalance(QCamera::WhiteBalanceMode mode)
{
    switch (mode) {
    case QCamera::WhiteBalanceAuto:
        break;
    case QCamera::WhiteBalanceManual:
    case QCamera::WhiteBalanceSunlight:
        return 5600;
    case QCamera::WhiteBalanceCloudy:
        return 6000;
    case QCamera::WhiteBalanceShade:
        return 7000;
    case QCamera::WhiteBalanceTungsten:
        return 3200;
    case QCamera::WhiteBalanceFluorescent:
        return 4000;
    case QCamera::WhiteBalanceFlash:
        return 5500;
    case QCamera::WhiteBalanceSunset:
        return 3000;
    }
    return 0;
}



QT_END_NAMESPACE

#include "moc_qplatformcamera_p.cpp"
