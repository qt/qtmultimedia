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

#ifndef MOCKCAMERAIMAGEPROCESSINGCONTROL_H
#define MOCKCAMERAIMAGEPROCESSINGCONTROL_H

#include "private/qplatformcameraimageprocessing_p.h"

class QMockCameraImageProcessing : public QPlatformCameraImageProcessing
{
    Q_OBJECT
public:
    QMockCameraImageProcessing(QObject *parent = 0)
        : QPlatformCameraImageProcessing(parent)
    {
        m_supportedWhiteBalance.insert(QCameraImageProcessing::WhiteBalanceAuto);
    }

    QCameraImageProcessing::WhiteBalanceMode whiteBalanceMode() const
    {
        return m_whiteBalanceMode;
    }
    void setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode)
    {
        m_whiteBalanceMode = mode;
    }

    bool isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const
    {
        return m_supportedWhiteBalance.contains(mode);
    }

    void setSupportedWhiteBalanceModes(QSet<QCameraImageProcessing::WhiteBalanceMode> modes)
    {
        m_supportedWhiteBalance = modes;
    }

    bool isParameterSupported(ProcessingParameter parameter) const
    {
        switch (parameter)
        {
        case ContrastAdjustment:
        case BrightnessAdjustment:
        case SaturationAdjustment:
        case ColorTemperature:
        case WhiteBalancePreset:
            return true;
        default :
            return false;
        }
    }

    bool isParameterValueSupported(ProcessingParameter parameter, const QVariant &value) const
    {
        if (parameter != WhiteBalancePreset)
            return false;

        return m_supportedWhiteBalance.contains(value.value<QCameraImageProcessing::WhiteBalanceMode>());
    }

    void setParameter(ProcessingParameter parameter, const QVariant &value)
    {
        switch (parameter) {
        case ContrastAdjustment:
            m_contrast = value;
            break;
        case SaturationAdjustment:
            m_saturation = value;
            break;
        case BrightnessAdjustment:
            m_brightness = value;
            break;
        case ColorTemperature:
            m_manualWhiteBalance = value;
            break;
        case WhiteBalancePreset:
            m_whiteBalanceMode = value.value<QCameraImageProcessing::WhiteBalanceMode>();
            break;
        default:
            break;
        }
    }


private:
    QCameraImageProcessing::WhiteBalanceMode m_whiteBalanceMode;
    QSet<QCameraImageProcessing::WhiteBalanceMode> m_supportedWhiteBalance;
    QVariant m_manualWhiteBalance;
    QVariant m_contrast;
    QVariant m_saturation;
    QVariant m_brightness;
};

#endif // MOCKCAMERAIMAGEPROCESSINGCONTROL_H
