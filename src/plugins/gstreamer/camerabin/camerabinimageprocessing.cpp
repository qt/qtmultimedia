/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
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

#include "camerabinimageprocessing.h"
#include "camerabinsession.h"

CameraBinImageProcessing::CameraBinImageProcessing(CameraBinSession *session)
    :QCameraImageProcessingControl(session),
     m_session(session)
{
    m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_AUTO] = QCameraImageProcessing::WhiteBalanceAuto;
    m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_DAYLIGHT] = QCameraImageProcessing::WhiteBalanceSunlight;
    m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_CLOUDY] = QCameraImageProcessing::WhiteBalanceCloudy;
    m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_SUNSET] = QCameraImageProcessing::WhiteBalanceSunset;
    m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_TUNGSTEN] = QCameraImageProcessing::WhiteBalanceTungsten;
    m_mappedWbValues[GST_PHOTOGRAPHY_WB_MODE_FLUORESCENT] = QCameraImageProcessing::WhiteBalanceFluorescent;

    updateColorBalanceValues();
}

CameraBinImageProcessing::~CameraBinImageProcessing()
{
}

void CameraBinImageProcessing::updateColorBalanceValues()
{
    if (!GST_IS_COLOR_BALANCE(m_session->cameraBin())) {
        // Camerabin doesn't implement gstcolorbalance interface
        return;
    }

    GstColorBalance *balance = GST_COLOR_BALANCE(m_session->cameraBin());
    const GList *controls = gst_color_balance_list_channels(balance);

    const GList *item;
    GstColorBalanceChannel *channel;
    gint cur_value;

    for (item = controls; item; item = g_list_next (item)) {
        channel = (GstColorBalanceChannel *)item->data;
        cur_value = gst_color_balance_get_value (balance, channel);

        if (!g_ascii_strcasecmp (channel->label, "brightness")) {
            m_values[QCameraImageProcessingControl::Brightness] = cur_value;
        } else if (!g_ascii_strcasecmp (channel->label, "contrast")) {
            m_values[QCameraImageProcessingControl::Contrast] = cur_value;
        } else if (!g_ascii_strcasecmp (channel->label, "saturation")) {
            m_values[QCameraImageProcessingControl::Saturation] = cur_value;
        }
    }
}

bool CameraBinImageProcessing::setColorBalanceValue(const QString& channel, int value)
{

    if (!GST_IS_COLOR_BALANCE(m_session->cameraBin())) {
        // Camerabin doesn't implement gstcolorbalance interface
        return false;
    }

    GstColorBalance *balance = GST_COLOR_BALANCE(m_session->cameraBin());
    const GList *controls = gst_color_balance_list_channels(balance);

    const GList *item;
    GstColorBalanceChannel *colorBalanceChannel;

    for (item = controls; item; item = g_list_next (item)) {
        colorBalanceChannel = (GstColorBalanceChannel *)item->data;

        if (!g_ascii_strcasecmp (colorBalanceChannel->label, channel.toAscii())) {
            gst_color_balance_set_value (balance, colorBalanceChannel, value);
            return true;
        }
    }

    return false;
}

QCameraImageProcessing::WhiteBalanceMode CameraBinImageProcessing::whiteBalanceMode() const
{
    GstWhiteBalanceMode wbMode;
    gst_photography_get_white_balance_mode(m_session->photography(), &wbMode);
    return m_mappedWbValues[wbMode];
}

void CameraBinImageProcessing::setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode)
{
    if (isWhiteBalanceModeSupported(mode))
        gst_photography_set_white_balance_mode(m_session->photography(), m_mappedWbValues.key(mode));
}

bool CameraBinImageProcessing::isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const
{
    return m_mappedWbValues.values().contains(mode);
}

bool CameraBinImageProcessing::isProcessingParameterSupported(QCameraImageProcessingControl::ProcessingParameter parameter) const
{
    return parameter == QCameraImageProcessingControl::Contrast
            || parameter == QCameraImageProcessingControl::Brightness
            || parameter == QCameraImageProcessingControl::Saturation;
}

QVariant CameraBinImageProcessing::processingParameter(
        QCameraImageProcessingControl::ProcessingParameter parameter) const
{
    if (m_values.contains(parameter))
        return m_values.value(parameter);
    else
        return QVariant();
}

void CameraBinImageProcessing::setProcessingParameter(
        QCameraImageProcessingControl::ProcessingParameter parameter,
        QVariant value)
{
    switch (parameter) {
    case Contrast:
        setColorBalanceValue("contrast", value.toInt());
        break;
    case Brightness:
        setColorBalanceValue("brightness", value.toInt());
        break;
    case Saturation:
        setColorBalanceValue("saturation", value.toInt());
        break;
    default:
        break;
    }

    updateColorBalanceValues();
}

