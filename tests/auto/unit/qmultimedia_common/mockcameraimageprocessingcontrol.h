/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKCAMERAIMAGEPROCESSINGCONTROL_H
#define MOCKCAMERAIMAGEPROCESSINGCONTROL_H

#include "qcameraimageprocessingcontrol.h"

class MockImageProcessingControl : public QCameraImageProcessingControl
{
    Q_OBJECT
public:
    MockImageProcessingControl(QObject *parent = 0)
        : QCameraImageProcessingControl(parent)
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

    bool isProcessingParameterSupported(ProcessingParameter parameter) const
    {
        //return parameter == Contrast ||  parameter == Sharpening || parameter == ColorTemperature;
        switch (parameter)
        {
        case Contrast:
        case Brightness:
        case Sharpening:
        case Saturation:
        case Denoising:
        case ColorTemperature:
        case ExtendedParameter:
            return true;
        default :
            return false;
        }
    }
    QVariant processingParameter(ProcessingParameter parameter) const
    {
        switch (parameter) {
        case Contrast:
            return m_contrast;
        case Saturation:
            return m_saturation;
        case Brightness:
            return m_brightness;
        case Sharpening:
            return m_sharpeningLevel;
        case Denoising:
            return m_denoising;
        case ColorTemperature:
            return m_manualWhiteBalance;
        case ExtendedParameter:
            return m_extendedParameter;
        default:
            return QVariant();
        }
    }
    void setProcessingParameter(ProcessingParameter parameter, QVariant value)
    {
        switch (parameter) {
        case Contrast:
            m_contrast = value;
            break;
        case Saturation:
            m_saturation = value;
            break;
        case Brightness:
            m_brightness = value;
            break;
        case Sharpening:
            m_sharpeningLevel = value;
            break;
        case Denoising:
            m_denoising = value;
            break;
        case ColorTemperature:
            m_manualWhiteBalance = value;
            break;
        case ExtendedParameter:
            m_extendedParameter = value;
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
    QVariant m_sharpeningLevel;
    QVariant m_saturation;
    QVariant m_brightness;
    QVariant m_denoising;
    QVariant m_extendedParameter;
};

#endif // MOCKCAMERAIMAGEPROCESSINGCONTROL_H
