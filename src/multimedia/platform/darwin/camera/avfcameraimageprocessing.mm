/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/qdebug.h>

#include "avfcameraimageprocessing_p.h"
#include "avfcamera_p.h"

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

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

#ifdef Q_OS_IOS
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
#endif

}

AVFCameraImageProcessing::AVFCameraImageProcessing(AVFCamera *camera)
    : QPlatformCameraImageProcessing(camera),
      m_camera(camera),
      m_whiteBalanceMode(QCamera::WhiteBalanceAuto)
{
    Q_ASSERT(m_camera);

    // AVFoundation's API allows adjusting white balance gains values(or temperature and tint)
    // only for iOS
#ifdef Q_OS_IOS
    m_mappedWhiteBalancePresets[QCamera::WhiteBalanceSunlight]
        = qMakePair(5600.0, .0);
    m_mappedWhiteBalancePresets[QCamera::WhiteBalanceCloudy]
        = qMakePair(6000.0, .0);
    m_mappedWhiteBalancePresets[QCamera::WhiteBalanceShade]
        = qMakePair(7000.0, .0);
    m_mappedWhiteBalancePresets[QCamera::WhiteBalanceTungsten]
        = qMakePair(3200.0, .0);
    m_mappedWhiteBalancePresets[QCamera::WhiteBalanceFluorescent]
        = qMakePair(4000.0, .0);
    m_mappedWhiteBalancePresets[QCamera::WhiteBalanceFlash]
        = qMakePair(5500.0, .0);
    m_mappedWhiteBalancePresets[QCamera::WhiteBalanceSunset]
        = qMakePair(3000.0, .0);
#endif

    // The default white balance mode of AVFoundation is WhiteBalanceModeLocked
    // so set it to correspond to Qt's WhiteBalanceModeAuto as soon as the device
    // is available
    connect(m_camera, SIGNAL(activeChanged(bool)), SLOT(cameraActiveChanged(bool)));
}

AVFCameraImageProcessing::~AVFCameraImageProcessing()
{
}

bool AVFCameraImageProcessing::isParameterSupported(
        QPlatformCameraImageProcessing::ProcessingParameter parameter) const
{
#ifdef Q_OS_IOS
    return (parameter == QPlatformCameraImageProcessing::WhiteBalancePreset
            || parameter == QPlatformCameraImageProcessing::ColorTemperature)
            && m_camera->device();
#else
    return parameter == QPlatformCameraImageProcessing::WhiteBalancePreset
            && m_camera->device();
#endif
}

bool AVFCameraImageProcessing::isParameterValueSupported(
        QPlatformCameraImageProcessing::ProcessingParameter parameter,
        const QVariant &value) const
{
    AVCaptureDevice *captureDevice = m_camera->device();
    Q_ASSERT(captureDevice);

    if (parameter == QPlatformCameraImageProcessing::WhiteBalancePreset)
        return isWhiteBalanceModeSupported(
            value.value<QCamera::WhiteBalanceMode>());

#ifdef Q_OS_IOS
    if (parameter == QPlatformCameraImageProcessing::ColorTemperature) {
        AVCaptureWhiteBalanceGains gains;
        return avf_convert_temp_and_tint_to_wb_gains(
            captureDevice, value.value<float>(), .0, gains);
    }
#endif

    return false;
}

void AVFCameraImageProcessing::setParameter(
        QPlatformCameraImageProcessing::ProcessingParameter parameter,
        const QVariant &value)
{
    bool result = false;
    if (parameter == QPlatformCameraImageProcessing::WhiteBalancePreset)
        result = setWhiteBalanceMode(value.value<QCamera::WhiteBalanceMode>());

#ifdef Q_OS_IOS
    else if (parameter == QPlatformCameraImageProcessing::ColorTemperature)
        result = setColorTemperature(value.value<float>());
#endif

    else
        qDebug() << "Setting parameter is not supported\n";

    if (!result)
        qDebug() << "Could not set parameter\n";
}

bool AVFCameraImageProcessing::setWhiteBalanceMode(
        QCamera::WhiteBalanceMode mode)
{
    AVCaptureDevice *captureDevice = m_camera->device();
    Q_ASSERT(captureDevice);

    AVCaptureWhiteBalanceMode avMode;
    avf_convert_white_balance_mode(mode, avMode);

    if (!isWhiteBalanceModeSupported(mode))
        return false;

    if (mode == QCamera::WhiteBalanceAuto
        && avf_set_white_balance_mode(captureDevice, avMode)) {
            m_whiteBalanceMode = mode;
            return true;
    }

#ifdef Q_OS_IOS
    if (mode == QCamera::WhiteBalanceManual
        && avf_set_white_balance_mode(captureDevice, avMode)) {
            m_whiteBalanceMode = mode;
            return true;
    }

    const auto mappedValues = m_mappedWhiteBalancePresets[mode];
    AVCaptureWhiteBalanceGains wbGains;
    if (avf_convert_temp_and_tint_to_wb_gains(captureDevice, mappedValues.first, mappedValues.second, wbGains)
        && avf_set_white_balance_gains(captureDevice, wbGains)) {
            m_whiteBalanceMode = mode;
            return true;
    }
#endif

    return false;
}

bool AVFCameraImageProcessing::isWhiteBalanceModeSupported(
        QCamera::WhiteBalanceMode qtMode) const
{
    AVCaptureDevice *captureDevice = m_camera->device();
    Q_ASSERT(captureDevice);

    AVCaptureWhiteBalanceMode avMode;
    avf_convert_white_balance_mode(qtMode, avMode);

    // Since AVFoundation's API does not support setting custom white balance gains
    // on macOS, only WhiteBalanceAuto mode is supported.
    if (qtMode == QCamera::WhiteBalanceAuto)
        return [captureDevice isWhiteBalanceModeSupported:avMode];

#ifdef Q_OS_IOS
    // Qt's WhiteBalanceManual corresponds to AVFoundations's WhiteBalanceModeLocked
    // + setting custom white balance gains (or color temperature in Qt)
    if (qtMode == QCamera::WhiteBalanceManual)
        return [captureDevice isWhiteBalanceModeSupported:avMode]
            && captureDevice.lockingWhiteBalanceWithCustomDeviceGainsSupported;

    // Qt's white balance presets correspond to AVFoundation's WhiteBalanceModeLocked
    // + setting the white balance gains to the mapped value for each preset
    if (m_mappedWhiteBalancePresets.find(qtMode) != m_mappedWhiteBalancePresets.end()) {
        const auto mappedValues = m_mappedWhiteBalancePresets[qtMode];
        AVCaptureWhiteBalanceGains wbGains;
        return [captureDevice isWhiteBalanceModeSupported:avMode]
            && captureDevice.lockingWhiteBalanceWithCustomDeviceGainsSupported
            && avf_convert_temp_and_tint_to_wb_gains(captureDevice, mappedValues.first,
                mappedValues.second, wbGains);
    }
#endif

    qDebug() << "White balance mode not supported";
    return false;
}

#ifdef Q_OS_IOS
float AVFCameraImageProcessing::colorTemperature() const
{
    return m_colorTemperature;
}

bool AVFCameraImageProcessing::setColorTemperature(float temperature)
{
    AVCaptureDevice *captureDevice = m_camera->device();
    Q_ASSERT(captureDevice);

    AVCaptureWhiteBalanceGains wbGains;
    if (avf_convert_temp_and_tint_to_wb_gains(captureDevice, temperature, .0, wbGains)
        && avf_set_white_balance_gains(captureDevice, wbGains)) {
            m_whiteBalanceMode = QCamera::WhiteBalanceManual;
            return true;
    }

    return false;
}
#endif

void AVFCameraImageProcessing::cameraActiveChanged(bool active)
{
    if (!active)
        return;
    setWhiteBalanceMode(QCamera::WhiteBalanceAuto);
}

QT_END_NAMESPACE
