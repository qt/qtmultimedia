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

#include "qgstreamercameraimageprocessing_p.h"
#include "qgstreamercamera_p.h"

#include <gst/video/colorbalance.h>

QT_BEGIN_NAMESPACE

#if 0 && QT_CONFIG(linux_v4l)
class QGstreamerImageProcessingV4L2
{
public:
    struct SourceParameterValueInfo {
        SourceParameterValueInfo()
            : cid(0)
        {
        }

        qint32 defaultValue;
        qint32 minimumValue;
        qint32 maximumValue;
        quint32 cid; // V4L control id
    };

    static qreal scaledImageProcessingParameterValue(
        qint32 sourceValue, const SourceParameterValueInfo &sourceValueInfo);
    static qint32 sourceImageProcessingParameterValue(
        qreal scaledValue, const SourceParameterValueInfo &valueRange);

    QMap<QGstreamerImageProcessing::ProcessingParameter, SourceParameterValueInfo> m_parametersInfo;
};
#endif

QGstreamerImageProcessing::QGstreamerImageProcessing(QGstreamerCamera *camera)
    : QPlatformCameraImageProcessing(camera)
    , m_camera(camera)
#if 0 && QT_CONFIG(linux_v4l)
    , m_v4lImageControl(nullptr)
#endif
{
#if QT_CONFIG(gstreamer_photography)
    if (auto *photography = m_camera->photography())
        gst_photography_set_white_balance_mode(photography, GST_PHOTOGRAPHY_WB_MODE_AUTO);
#endif

#if 0 && QT_CONFIG(linux_v4l)
    if (m_camera->isV4L2Camera())
        m_v4lImageControl = new QGstreamerImageProcessingV4L2;
#endif

    updateColorBalanceValues();
}

QGstreamerImageProcessing::~QGstreamerImageProcessing()
{
}

void QGstreamerImageProcessing::updateColorBalanceValues()
{
    GstColorBalance *balance = m_camera->colorBalance();
    if (!balance)
        return;

    const GList *controls = gst_color_balance_list_channels(balance);

    const GList *item;
    GstColorBalanceChannel *channel;
    gint cur_value;
    qreal scaledValue = 0;

    for (item = controls; item; item = g_list_next (item)) {
        channel = (GstColorBalanceChannel *)item->data;
        cur_value = gst_color_balance_get_value (balance, channel);

        //map the [min_value..max_value] range to [-1.0 .. 1.0]
        if (channel->min_value != channel->max_value) {
            scaledValue = qreal(cur_value - channel->min_value) /
                    (channel->max_value - channel->min_value) * 2 - 1;
        }

        if (!g_ascii_strcasecmp (channel->label, "brightness")) {
            m_values[QPlatformCameraImageProcessing::BrightnessAdjustment] = scaledValue;
        } else if (!g_ascii_strcasecmp (channel->label, "contrast")) {
            m_values[QPlatformCameraImageProcessing::ContrastAdjustment] = scaledValue;
        } else if (!g_ascii_strcasecmp (channel->label, "saturation")) {
            m_values[QPlatformCameraImageProcessing::SaturationAdjustment] = scaledValue;
        }
    }
}

bool QGstreamerImageProcessing::setColorBalanceValue(const char *channel, qreal value)
{
    GstColorBalance *balance = m_camera->colorBalance();
    if (!balance)
        return false;

    const GList *controls = gst_color_balance_list_channels(balance);

    const GList *item;
    GstColorBalanceChannel *colorBalanceChannel;

    for (item = controls; item; item = g_list_next (item)) {
        colorBalanceChannel = (GstColorBalanceChannel *)item->data;

        if (!g_ascii_strcasecmp (colorBalanceChannel->label, channel)) {
            //map the [-1.0 .. 1.0] range to [min_value..max_value]
            gint scaledValue = colorBalanceChannel->min_value + qRound(
                        (value+1.0)/2.0 * (colorBalanceChannel->max_value - colorBalanceChannel->min_value));

            gst_color_balance_set_value (balance, colorBalanceChannel, scaledValue);
            return true;
        }
    }

    return false;
}

QCameraImageProcessing::WhiteBalanceMode QGstreamerImageProcessing::whiteBalanceMode() const
{
    return m_whiteBalanceMode;
}

bool QGstreamerImageProcessing::setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode)
{
    if (!isWhiteBalanceModeSupported(mode))
        return false;
#if QT_CONFIG(gstreamer_photography)
    if (auto *photography = m_camera->photography()) {
        GstPhotographyWhiteBalanceMode gstMode;
        switch (mode) {
        case QCameraImageProcessing::WhiteBalanceAuto:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_AUTO;
            break;
        case QCameraImageProcessing::WhiteBalanceSunlight:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_DAYLIGHT;
            break;
        case QCameraImageProcessing::WhiteBalanceCloudy:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_CLOUDY;
            break;
        case QCameraImageProcessing::WhiteBalanceShade:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_SHADE;
            break;
        case QCameraImageProcessing::WhiteBalanceSunset:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_SUNSET;
            break;
        case QCameraImageProcessing::WhiteBalanceTungsten:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_TUNGSTEN;
            break;
        case QCameraImageProcessing::WhiteBalanceFluorescent:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_FLUORESCENT;
            break;
        default:
            Q_ASSERT(false);
            break;
        }
        if (gst_photography_set_white_balance_mode(m_camera->photography(), gstMode)) {
            m_whiteBalanceMode = mode;
            return true;
        }
    }
#endif

    return false;
}

bool QGstreamerImageProcessing::isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const
{
#if QT_CONFIG(gstreamer_photography)
    if (m_camera->photography()) {
        switch (mode) {
        case QCameraImageProcessing::WhiteBalanceAuto:
        case QCameraImageProcessing::WhiteBalanceSunlight:
        case QCameraImageProcessing::WhiteBalanceCloudy:
        case QCameraImageProcessing::WhiteBalanceShade:
        case QCameraImageProcessing::WhiteBalanceSunset:
        case QCameraImageProcessing::WhiteBalanceTungsten:
        case QCameraImageProcessing::WhiteBalanceFluorescent:
            return true;
        default:
            break;
        }
    }
#endif
#if 0 && QT_CONFIG(linux_v4l)
        if (!isPhotographyWhiteBalanceSupported)
            return m_v4lImageControl->isParameterValueSupported(parameter, value);
#endif

    return mode == QCameraImageProcessing::WhiteBalanceAuto;
}

QCameraImageProcessing::ColorFilter QGstreamerImageProcessing::colorFilter() const
{
    return m_colorFilter;
}

bool QGstreamerImageProcessing::setColorFilter(QCameraImageProcessing::ColorFilter filter)
{
#if QT_CONFIG(gstreamer_photography)
    if (GstPhotography *photography = m_camera->photography()) {
        GstPhotographyColorToneMode mode;
        switch (filter) {
        case QCameraImageProcessing::ColorFilterNone:
            mode = GST_PHOTOGRAPHY_COLOR_TONE_MODE_NORMAL;
            break;
        case QCameraImageProcessing::ColorFilterSepia:
            mode = GST_PHOTOGRAPHY_COLOR_TONE_MODE_SEPIA;
            break;
        case QCameraImageProcessing::ColorFilterGrayscale:
            mode = GST_PHOTOGRAPHY_COLOR_TONE_MODE_GRAYSCALE;
            break;
        case QCameraImageProcessing::ColorFilterNegative:
            mode = GST_PHOTOGRAPHY_COLOR_TONE_MODE_NEGATIVE;
            break;
        case QCameraImageProcessing::ColorFilterSolarize:
            mode = GST_PHOTOGRAPHY_COLOR_TONE_MODE_SOLARIZE;
            break;
        case QCameraImageProcessing::ColorFilterPosterize:
            mode = GST_PHOTOGRAPHY_COLOR_TONE_MODE_POSTERIZE;
            break;
        case QCameraImageProcessing::ColorFilterWhiteboard:
            mode = GST_PHOTOGRAPHY_COLOR_TONE_MODE_WHITEBOARD;
            break;
        case QCameraImageProcessing::ColorFilterBlackboard:
            mode = GST_PHOTOGRAPHY_COLOR_TONE_MODE_BLACKBOARD;
            break;
        case QCameraImageProcessing::ColorFilterAqua:
            mode = GST_PHOTOGRAPHY_COLOR_TONE_MODE_AQUA;
            break;
        }
        if (gst_photography_set_color_tone_mode(photography, mode)) {
            m_colorFilter = filter;
            return true;
        }
    }
#endif
    return false;
}

bool QGstreamerImageProcessing::isColorFilterSupported(QCameraImageProcessing::ColorFilter filter) const
{
#if QT_CONFIG(gstreamer_photography)
    if (m_camera->photography())
        return true;
#endif
    return filter == QCameraImageProcessing::ColorFilterNone;
}

bool QGstreamerImageProcessing::isParameterSupported(QPlatformCameraImageProcessing::ProcessingParameter parameter) const
{
#if QT_CONFIG(gstreamer_photography)
    if (m_camera->photography()) {
        if (parameter == QPlatformCameraImageProcessing::WhiteBalancePreset || parameter == QPlatformCameraImageProcessing::ColorFilter)
            return true;
    }
#endif

    if (parameter == QPlatformCameraImageProcessing::Contrast
            || parameter == QPlatformCameraImageProcessing::Brightness
            || parameter == QPlatformCameraImageProcessing::Saturation) {
        if (m_camera->colorBalance())
            return true;
    }

#if 0 && QT_CONFIG(linux_v4l)
    if (m_v4lImageControl->isParameterSupported(parameter))
        return true;
#endif

    return false;
}

bool QGstreamerImageProcessing::isParameterValueSupported(QPlatformCameraImageProcessing::ProcessingParameter parameter, const QVariant &value) const
{
    switch (parameter) {
    case ContrastAdjustment:
    case BrightnessAdjustment:
    case SaturationAdjustment: {
        GstColorBalance *balance = m_camera->colorBalance();
        const bool isGstColorBalanceValueSupported = balance && qAbs(value.toReal()) <= 1.0;
#if 0 && QT_CONFIG(linux_v4l)
        if (!isGstColorBalanceValueSupported)
            return m_v4lImageControl->isParameterValueSupported(parameter, value);
#endif
        return isGstColorBalanceValueSupported;
    }
    case ColorTemperature: {
#if 0 && QT_CONFIG(linux_v4l)
        return m_v4lImageControl->isParameterValueSupported(parameter, value);
#else
        return false;
#endif
    }
    case WhiteBalancePreset:
        return isWhiteBalanceModeSupported(value.value<QCameraImageProcessing::WhiteBalanceMode>());
    case ColorFilter:
        return isColorFilterSupported(value.value<QCameraImageProcessing::ColorFilter>());
    default:
        break;
    }

    return false;
}

QVariant QGstreamerImageProcessing::parameter(QPlatformCameraImageProcessing::ProcessingParameter parameter) const
{
    switch (parameter) {
    case QPlatformCameraImageProcessing::WhiteBalancePreset: {
        const QCameraImageProcessing::WhiteBalanceMode mode = whiteBalanceMode();
#if 0 && QT_CONFIG(linux_v4l)
        if (mode == QCameraImageProcessing::WhiteBalanceAuto
                || mode == QCameraImageProcessing::WhiteBalanceManual) {
            return m_v4lImageControl->parameter(parameter);
        }
#endif
        return QVariant::fromValue<QCameraImageProcessing::WhiteBalanceMode>(mode);
    }
    case QPlatformCameraImageProcessing::ColorTemperature: {
#if 0 && QT_CONFIG(linux_v4l)
        return m_v4lImageControl->parameter(parameter);
#else
        return QVariant();
#endif
    }
    case QPlatformCameraImageProcessing::ColorFilter:
        return QVariant::fromValue(colorFilter());
    default: {
        const bool isGstParameterSupported = m_values.contains(parameter);
#if 0 && QT_CONFIG(linux_v4l)
        if (!isGstParameterSupported) {
            if (parameter == QPlatformCameraImageProcessing::BrightnessAdjustment
                    || parameter == QPlatformCameraImageProcessing::ContrastAdjustment
                    || parameter == QPlatformCameraImageProcessing::SaturationAdjustment) {
                return m_v4lImageControl->parameter(parameter);
            }
        }
#endif
        return isGstParameterSupported
                ? QVariant(m_values.value(parameter))
                : QVariant();
    }
    }
}

void QGstreamerImageProcessing::setParameter(QPlatformCameraImageProcessing::ProcessingParameter parameter,
        const QVariant &value)
{
    switch (parameter) {
    case ContrastAdjustment: {
        if (!setColorBalanceValue("contrast", value.toReal())) {
#if 0 && QT_CONFIG(linux_v4l)
            m_v4lImageControl->setParameter(parameter, value);
#endif
        }
    }
        break;
    case BrightnessAdjustment: {
        if (!setColorBalanceValue("brightness", value.toReal())) {
#if 0 && QT_CONFIG(linux_v4l)
            m_v4lImageControl->setParameter(parameter, value);
#endif
        }
    }
        break;
    case SaturationAdjustment: {
        if (!setColorBalanceValue("saturation", value.toReal())) {
#if 0 && QT_CONFIG(linux_v4l)
            m_v4lImageControl->setParameter(parameter, value);
#endif
        }
    }
        break;
    case WhiteBalancePreset: {
        if (!setWhiteBalanceMode(value.value<QCameraImageProcessing::WhiteBalanceMode>())) {
#if 0 && QT_CONFIG(linux_v4l)
            const QCameraImageProcessing::WhiteBalanceMode mode =
                    value.value<QCameraImageProcessing::WhiteBalanceMode>();
            if (mode == QCameraImageProcessing::WhiteBalanceAuto
                    || mode == QCameraImageProcessing::WhiteBalanceManual) {
                m_v4lImageControl->setParameter(parameter, value);
                return;
            }
#endif
        }
    }
        break;
    case QPlatformCameraImageProcessing::ColorTemperature: {
#if 0 && QT_CONFIG(linux_v4l)
        m_v4lImageControl->setParameter(parameter, value);
#endif
        break;
    }
    case QPlatformCameraImageProcessing::ColorFilter:
        setColorFilter(value.value<QCameraImageProcessing::ColorFilter>());
        break;
    default:
        break;
    }

    updateColorBalanceValues();
}

QT_END_NAMESPACE
