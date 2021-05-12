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
        initV4L2Controls();
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

static bool isColorBalanceParameter(QPlatformCameraImageProcessing::ProcessingParameter param)
{
    return param >= QPlatformCameraImageProcessing::ContrastAdjustment && param <= QPlatformCameraImageProcessing::BrightnessAdjustment;
}

void QGstreamerImageProcessing::updateColorBalanceValues()
{
    for (int i = ContrastAdjustment; i <= BrightnessAdjustment; ++i)
        colorBalanceParameters[i] = {};

    GstColorBalance *balance = m_camera->colorBalance();
    if (!balance)
        return;

    const GList *controls = gst_color_balance_list_channels(balance);

    for (const GList *item = controls; item; item = g_list_next (item)) {
        GstColorBalanceChannel *channel = (GstColorBalanceChannel *)item->data;
        int cur_value = gst_color_balance_get_value (balance, channel);

        int index = -1;
        if (!g_ascii_strcasecmp (channel->label, "brightness")) {
            index = BrightnessAdjustment;
        } else if (!g_ascii_strcasecmp (channel->label, "contrast")) {
            index = ContrastAdjustment;
        } else if (!g_ascii_strcasecmp (channel->label, "saturation")) {
            index = SaturationAdjustment;
        } else if (!g_ascii_strcasecmp (channel->label, "hue")) {
            index = HueAdjustment;
        }
        if (index < 0)
            continue;
        colorBalanceParameters[index] = { channel, cur_value, channel->min_value, channel->max_value };
    }
}

bool QGstreamerImageProcessing::setColorBalanceValue(ProcessingParameter parameter, qreal value)
{
    Q_ASSERT(isColorBalanceParameter(parameter));

    GstColorBalance *balance = m_camera->colorBalance();
    if (!balance)
        return false;

    auto &p = colorBalanceParameters[parameter];
    if (!p.channel)
        return false;

    p.setScaledValue(value);
    gst_color_balance_set_value (balance, p.channel, p.current);
    return true;
}

bool QGstreamerImageProcessing::setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode)
{
    if (!isWhiteBalanceModeSupported(mode))
        return false;
#if QT_CONFIG(gstreamer_photography)
    if (auto *photography = m_camera->photography()) {
        GstPhotographyWhiteBalanceMode gstMode = GST_PHOTOGRAPHY_WB_MODE_AUTO;
        switch (mode) {
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
        case QCameraImageProcessing::WhiteBalanceAuto:
        default:
            break;
        }
        if (gst_photography_set_white_balance_mode(photography, gstMode)) {
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

bool QGstreamerImageProcessing::setColorFilter(QCameraImageProcessing::ColorFilter filter)
{
#if QT_CONFIG(gstreamer_photography)
    if (GstPhotography *photography = m_camera->photography()) {
        GstPhotographyColorToneMode mode = GST_PHOTOGRAPHY_COLOR_TONE_MODE_NORMAL;
        switch (filter) {
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
        case QCameraImageProcessing::ColorFilterNone:
        default:
            break;
        }
        if (gst_photography_set_color_tone_mode(photography, mode)) {
            m_colorFilter = filter;
            return true;
        }
    }
#else
    Q_UNUSED(filter);
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
    if (isColorBalanceParameter(parameter))
        return colorBalanceParameters[parameter].channel != nullptr;

#if QT_CONFIG(linux_v4l)
    if (m_camera->isV4L2Camera()) {
        switch (parameter) {
        case WhiteBalancePreset:
            return v4l2AutoWhiteBalanceSupported;
        case ColorTemperature:
            return v4l2ColorTemperatureSupported;
        case ColorFilter:
        default:
            // v4l2 doesn't have photography
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

    return false;
}

bool QGstreamerImageProcessing::isParameterValueSupported(QPlatformCameraImageProcessing::ProcessingParameter parameter, const QVariant &value) const
{
    switch (parameter) {
    case ContrastAdjustment:
    case BrightnessAdjustment:
    case SaturationAdjustment:
        if (qAbs(value.toReal()) > 1)
            return false;
        return isParameterSupported(parameter);
    case ColorTemperature: {
#if QT_CONFIG(linux_v4l)
        if (m_camera->isV4L2Camera()) {
            int temp = value.toInt();
            return v4l2ColorTemperatureSupported && v4l2MinColorTemp <= temp && temp <= v4l2MaxColorTemp;
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

void QGstreamerImageProcessing::setParameter(QPlatformCameraImageProcessing::ProcessingParameter parameter, const QVariant &value)
{
    if (isColorBalanceParameter(parameter))
        setColorBalanceValue(parameter, value.toDouble());

#if QT_CONFIG(linux_v4l)
    if (m_camera->isV4L2Camera()) {
        if (setV4L2Param(parameter, value))
            return;
    }
#endif

    switch (parameter) {
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
}

void QGstreamerImageProcessing::update()
{
    updateColorBalanceValues();
#if QT_CONFIG(linux_v4l)
    initV4L2Controls();
#endif
}

#if QT_CONFIG(linux_v4l)
void QGstreamerImageProcessing::initV4L2Controls()
{
    v4l2AutoWhiteBalanceSupported = false;
    v4l2ColorTemperatureSupported = false;

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

    if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2AutoWhiteBalanceSupported = true;
        struct v4l2_control control;
        control.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
        control.value = true;
        ::ioctl(fd, VIDIOC_S_CTRL, &control);
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
    if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2MinColorTemp = queryControl.minimum;
        v4l2MaxColorTemp = queryControl.maximum;
        v4l2CurrentColorTemp = queryControl.default_value;
        v4l2ColorTemperatureSupported = true;
        struct v4l2_control control;
        control.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
        control.value = 0;
        if (::ioctl(fd, VIDIOC_G_CTRL, &control) == 0)
            v4l2CurrentColorTemp = control.value;
    }

    qt_safe_close(fd);

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
        control.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
        control.value = qBound(v4l2MinColorTemp, value.toInt(), v4l2MaxColorTemp);
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
        qWarning() << "Unable to set the parameter value:" << parameter << ":" << qt_error_string(errno);
        return false;
    }

    qt_safe_close(fd);
    return true;
}
#endif


QT_END_NAMESPACE
