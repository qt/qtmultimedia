/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#if QT_CONFIG(linux_v4l)
#include <linux/videodev2.h>
#include <private/qcore_unix_p.h>
#endif

QT_BEGIN_NAMESPACE

QGstreamerImageProcessing::QGstreamerImageProcessing(QGstreamerCamera *camera)
    : QPlatformCameraImageProcessing(camera)
    , m_camera(camera)
{
#if QT_CONFIG(linux_v4l)
    if (m_camera->isV4L2Camera())
        updateV4L2Controls();
#endif
#if QT_CONFIG(gstreamer_photography)
    if (auto *photography = m_camera->photography())
        gst_photography_set_white_balance_mode(photography, GST_PHOTOGRAPHY_WB_MODE_AUTO);
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
#if QT_CONFIG(linux_v4l)
    if (m_camera->isV4L2Camera()) {
        if (mode == QCameraImageProcessing::WhiteBalanceAuto)
            return true;
        if (v4l2AutoWhiteBalanceSupported && mode == QCameraImageProcessing::WhiteBalanceManual)
            return true;
        // ### Could emulate the others through hardcoded color temperatures
        return false;
    }
#endif
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
#if QT_CONFIG(linux_v4l)
    if (m_camera->isV4L2Camera()) {
        switch (parameter) {
        case WhiteBalancePreset:
            return v4l2AutoWhiteBalanceSupported;
        case ColorTemperature:
            return v4l2ColorTemperatureSupported;
        case ContrastAdjustment:
            return v4l2Contrast.cid != 0;
        case SaturationAdjustment:
            return v4l2Saturation.cid != 0;
        case BrightnessAdjustment:
            return v4l2Brightness.cid != 0;
        case HueAdjustment:
            return v4l2Hue.cid != 0;
        case ColorFilter:
            return false;
        }
    }
#endif

#if QT_CONFIG(gstreamer_photography)
    if (m_camera->photography()) {
        if (parameter == QPlatformCameraImageProcessing::WhiteBalancePreset || parameter == QPlatformCameraImageProcessing::ColorFilter)
            return true;
    }
#endif

    if (parameter == QPlatformCameraImageProcessing::ContrastAdjustment
        || parameter == QPlatformCameraImageProcessing::BrightnessAdjustment
        || parameter == QPlatformCameraImageProcessing::SaturationAdjustment) {
        if (m_camera->colorBalance())
            return true;
    }

    return false;
}

bool QGstreamerImageProcessing::isParameterValueSupported(QPlatformCameraImageProcessing::ProcessingParameter parameter, const QVariant &value) const
{
    switch (parameter) {
    case ContrastAdjustment:
    case BrightnessAdjustment:
    case SaturationAdjustment:
        if (qAbs(value.toReal() > 1))
            return false;
        return isParameterSupported(parameter);
    case ColorTemperature: {
#if QT_CONFIG(linux_v4l)
        if (m_camera->isV4L2Camera()) {
            int temp = value.toInt();
            return v4l2ColorTemperatureSupported &&
                   v4l2ColorTemperature.maximumValue <= temp &&
                   temp <= v4l2ColorTemperature.maximumValue;
        }
#endif
        return false;
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
#if QT_CONFIG(linux_v4l)
    if (m_camera->isV4L2Camera()) {
        auto result = getV4L2Param(parameter);
        if (result)
            return QVariant(*result);
    }
#endif
    switch (parameter) {
    case QPlatformCameraImageProcessing::WhiteBalancePreset:
        return whiteBalanceMode();
    case QPlatformCameraImageProcessing::ColorTemperature:
        return QVariant();
    case QPlatformCameraImageProcessing::ColorFilter:
        return QVariant::fromValue(colorFilter());
    default: {
        const bool isGstParameterSupported = m_values.contains(parameter);
        return isGstParameterSupported
                ? QVariant(m_values.value(parameter))
                : QVariant();
    }
    }
}

void QGstreamerImageProcessing::setParameter(QPlatformCameraImageProcessing::ProcessingParameter parameter, const QVariant &value)
{
#if QT_CONFIG(linux_v4l)
    if (m_camera->isV4L2Camera()) {
        if (setV4L2Param(parameter, value))
            return;
    }
#endif

    switch (parameter) {
    case ContrastAdjustment:
        setColorBalanceValue("contrast", value.toReal());
        break;
    case BrightnessAdjustment:
        setColorBalanceValue("brightness", value.toReal());
        break;
    case SaturationAdjustment:
        setColorBalanceValue("saturation", value.toReal());
        break;
    case WhiteBalancePreset:
        setWhiteBalanceMode(value.value<QCameraImageProcessing::WhiteBalanceMode>());
        break;
    case QPlatformCameraImageProcessing::ColorTemperature:
        break;
    case QPlatformCameraImageProcessing::ColorFilter:
        setColorFilter(value.value<QCameraImageProcessing::ColorFilter>());
        break;
    default:
        break;
    }

    updateColorBalanceValues();
}

void QGstreamerImageProcessing::update()
{
#if QT_CONFIG(linux_v4l)
    updateV4L2Controls();
#endif
}

#if QT_CONFIG(linux_v4l)
void QGstreamerImageProcessing::updateV4L2Controls()
{
    const QString deviceName = m_camera->v4l2Device();
    if (deviceName.isEmpty())
        return;
    isV4L2Device = true;

    const int fd = qt_safe_open(deviceName.toLocal8Bit().constData(), O_RDONLY);
    if (fd == -1) {
        qWarning() << "Unable to open the camera" << deviceName
                   << "for read to query the parameter info:" << qt_error_string(errno);
        return;
    }

    struct v4l2_queryctrl queryControl;
    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_AUTO_WHITE_BALANCE;

    if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0)
        v4l2AutoWhiteBalanceSupported = true;

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
    if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2ColorTemperature = { queryControl.id, queryControl.default_value, queryControl.minimum, queryControl.maximum };
        v4l2ColorTemperatureSupported = true;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_BRIGHTNESS;
    if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2Brightness = { queryControl.id, queryControl.default_value, queryControl.minimum, queryControl.maximum };
        qDebug() << "V4L2: query brightness" << queryControl.minimum << queryControl.default_value << queryControl.maximum;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_CONTRAST;
    if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2Contrast = { queryControl.id, queryControl.default_value, queryControl.minimum, queryControl.maximum };
        qDebug() << "V4L2: query contrast" << queryControl.minimum << queryControl.default_value << queryControl.maximum;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_SATURATION;
    if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2Saturation = { queryControl.id, queryControl.default_value, queryControl.minimum, queryControl.maximum };
        qDebug() << "V4L2: query saturation" << queryControl.minimum << queryControl.default_value << queryControl.maximum;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_HUE;
    if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2Hue = { queryControl.id, queryControl.default_value, queryControl.minimum, queryControl.maximum };
        qDebug() << "V4L2: query hue" << queryControl.minimum << queryControl.default_value << queryControl.maximum;
    }

    qt_safe_close(fd);

}

static qreal scaledImageProcessingParameterValue(qint32 sourceValue, const QGstreamerImageProcessing::SourceParameterValueInfo &sourceValueInfo)
{
    if (sourceValue == sourceValueInfo.defaultValue)
        return 0.0f;

    if (sourceValue < sourceValueInfo.defaultValue)
        return ((sourceValue - sourceValueInfo.minimumValue)
                / qreal(sourceValueInfo.defaultValue - sourceValueInfo.minimumValue))
               + (-1.0f);

    return ((sourceValue - sourceValueInfo.defaultValue)
            / qreal(sourceValueInfo.maximumValue - sourceValueInfo.defaultValue));
}

qint32 sourceImageProcessingParameterValue(qreal scaledValue, const QGstreamerImageProcessing::SourceParameterValueInfo &valueRange)
{
    if (qFuzzyIsNull(scaledValue))
        return valueRange.defaultValue;

    qint32 value;
    if (scaledValue < 0.0f)
        value = ((scaledValue - (-1.0f)) * (valueRange.defaultValue - valueRange.minimumValue)) + valueRange.minimumValue;
    else
        value = (scaledValue * (valueRange.maximumValue - valueRange.defaultValue)) + valueRange.defaultValue;
    return qBound(valueRange.minimumValue, value, valueRange.maximumValue);
}


std::optional<float> QGstreamerImageProcessing::getV4L2Param(QGstreamerImageProcessing::ProcessingParameter param) const
{
    struct v4l2_control control;
    ::memset(&control, 0, sizeof(control));
    const SourceParameterValueInfo *info = nullptr;
    switch (param) {
    case ContrastAdjustment:
        info = &v4l2Contrast;
        break;
    case SaturationAdjustment:
        info = &v4l2Saturation;
        break;
    case BrightnessAdjustment:
        info = &v4l2Brightness;
        break;
    case HueAdjustment:
        info = &v4l2Hue;
        break;
    case ColorTemperature:
        info = &v4l2ColorTemperature;
        break;
    default:
        return std::nullopt;
    }
    if (!info || !info->cid)
        return std::nullopt;

    control.id = info->cid;

    const int fd = qt_safe_open(m_camera->v4l2Device().toLocal8Bit().constData(), O_RDONLY);
    if (fd == -1) {
        qWarning() << "Unable to open the camera" << m_camera->v4l2Device()
                   << "for read to get the parameter value:" << qt_error_string(errno);
        return std::nullopt;
    }

    const bool ret = (::ioctl(fd, VIDIOC_G_CTRL, &control) == 0);

    qt_safe_close(fd);

    if (!ret) {
        qWarning() << "Unable to get the parameter value:" << param << ":" << qt_error_string(errno);
        return std::nullopt;
    }

    if (param == ColorTemperature)
        return control.value;
    return scaledImageProcessingParameterValue(control.value, *info);
}


bool QGstreamerImageProcessing::setV4L2Param(ProcessingParameter parameter, const QVariant &value)
{
    struct v4l2_control control;
    ::memset(&control, 0, sizeof(control));

    switch (parameter) {
    case QPlatformCameraImageProcessing::WhiteBalancePreset: {
        if (!v4l2AutoWhiteBalanceSupported)
            return false;
        const QCameraImageProcessing::WhiteBalanceMode mode = value.value<QCameraImageProcessing::WhiteBalanceMode>();
        if (mode != QCameraImageProcessing::WhiteBalanceAuto && mode != QCameraImageProcessing::WhiteBalanceManual)
            return false;
        control.id = V4L2_CID_AUTO_WHITE_BALANCE;
        control.value = (mode == QCameraImageProcessing::WhiteBalanceAuto);
        m_whiteBalanceMode = mode;
        break;
    }
    case QPlatformCameraImageProcessing::ColorTemperature:
        control.id = v4l2ColorTemperature.cid;
        control.value = qBound(v4l2ColorTemperature.minimumValue, value.toInt(), v4l2ColorTemperature.maximumValue);
        break;

    case QPlatformCameraImageProcessing::ContrastAdjustment: // falling back
        control.id = v4l2Contrast.cid;
        control.value = sourceImageProcessingParameterValue(value.toFloat(), v4l2Contrast);
        break;
    case QPlatformCameraImageProcessing::SaturationAdjustment: // falling back
        control.id = v4l2Saturation.cid;
        control.value = sourceImageProcessingParameterValue(value.toFloat(), v4l2Saturation);
        break;
    case QPlatformCameraImageProcessing::BrightnessAdjustment: // falling back
        control.id = v4l2Brightness.cid;
        control.value = sourceImageProcessingParameterValue(value.toFloat(), v4l2Brightness);
        break;
    case QPlatformCameraImageProcessing::HueAdjustment: // falling back
        control.id = v4l2Hue.cid;
        control.value = sourceImageProcessingParameterValue(value.toFloat(), v4l2Brightness);
        break;
    default:
        return false;
    }

    if (!control.id)
        return false;

    const int fd = qt_safe_open(m_camera->v4l2Device().toLocal8Bit().constData(), O_RDONLY);
    if (fd == -1) {
        qWarning() << "Unable to open the camera" << m_camera->v4l2Device()
                   << "for read to get the parameter value:" << qt_error_string(errno);
        return false;
    }

    if (::ioctl(fd, VIDIOC_S_CTRL, &control) != 0) {
        qDebug() << "setting" << parameter << control.id << control.value;
        qWarning() << "Unable to set the parameter value:" << parameter << ":" << qt_error_string(errno);
        return false;
    }

    qt_safe_close(fd);
    return true;
}
#endif


QT_END_NAMESPACE
