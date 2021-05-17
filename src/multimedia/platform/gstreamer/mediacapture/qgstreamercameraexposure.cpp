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

#include "qgstreamercameraexposure_p.h"
#include "qgstreamercamera_p.h"
#include <private/qgst_p.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

QGstreamerCameraExposure::QGstreamerCameraExposure(QGstreamerCamera *session)
    :QPlatformCameraExposure(session),
     m_camera(session)
{
#if QT_CONFIG(gstreamer_photography)
    hasPhotography = m_camera->photography() != nullptr;
#endif
}

QGstreamerCameraExposure::~QGstreamerCameraExposure()
{
}

bool QGstreamerCameraExposure::isParameterSupported(ExposureParameter parameter) const
{
#if QT_CONFIG(gstreamer_photography)
    if (hasPhotography) {
        switch (parameter) {
        case QPlatformCameraExposure::ExposureCompensation:
        case QPlatformCameraExposure::ISO:
        case QPlatformCameraExposure::Aperture:
        case QPlatformCameraExposure::ShutterSpeed:
            return true;
        default:
            break;
        }
    }
#else
    Q_UNUSED(parameter);
#endif
    return false;
}

QVariantList QGstreamerCameraExposure::supportedParameterRange(ExposureParameter parameter,
                                                        bool *continuous) const
{
    QVariantList res;

#if QT_CONFIG(gstreamer_photography)
    if (hasPhotography) {
        if (continuous)
            *continuous = false;

        switch (parameter) {
        case QPlatformCameraExposure::ExposureCompensation:
            if (continuous)
                *continuous = true;
            res << -2.0 << 2.0;
            break;
        case QPlatformCameraExposure::ISO:
            res << 100 << 200 << 400;
            break;
        case QPlatformCameraExposure::Aperture:
            res << 2.8;
            break;
        default:
            break;
        }
    }
#else
    Q_UNUSED(parameter);
    Q_UNUSED(continuous);
#endif

    return res;
}

QVariant QGstreamerCameraExposure::requestedValue(ExposureParameter parameter) const
{
    return m_requestedValues.value(parameter);
}

QVariant QGstreamerCameraExposure::actualValue(ExposureParameter parameter) const
{
#if QT_CONFIG(gstreamer_photography)
    if (hasPhotography) {
        switch (parameter) {
        case QPlatformCameraExposure::ExposureCompensation:
        {
            gfloat ev;
            gst_photography_get_ev_compensation(m_camera->photography(), &ev);
            return QVariant(ev);
        }
        case QPlatformCameraExposure::ISO:
        {
            guint isoSpeed = 0;
            gst_photography_get_iso_speed(m_camera->photography(), &isoSpeed);
            return QVariant(isoSpeed);
        }
        case QPlatformCameraExposure::Aperture:
            return QVariant(2.8);
        case QPlatformCameraExposure::ShutterSpeed:
        {
            guint32 shutterSpeed = 0;
            gst_photography_get_exposure(m_camera->photography(), &shutterSpeed);

            return QVariant(shutterSpeed/1000000.0);
        }
        case QPlatformCameraExposure::ExposureMode:
        {
            GstPhotographySceneMode sceneMode;
            gst_photography_get_scene_mode(m_camera->photography(), &sceneMode);

            switch (sceneMode) {
            case GST_PHOTOGRAPHY_SCENE_MODE_PORTRAIT:
                return QVariant::fromValue(QCamera::ExposurePortrait);
            case GST_PHOTOGRAPHY_SCENE_MODE_SPORT:
                return QVariant::fromValue(QCamera::ExposureSports);
            case GST_PHOTOGRAPHY_SCENE_MODE_NIGHT:
                return QVariant::fromValue(QCamera::ExposureNight);
            case GST_PHOTOGRAPHY_SCENE_MODE_MANUAL:
                return QVariant::fromValue(QCamera::ExposureManual);
            case GST_PHOTOGRAPHY_SCENE_MODE_LANDSCAPE:
                return QVariant::fromValue(QCamera::ExposureLandscape);
            case GST_PHOTOGRAPHY_SCENE_MODE_SNOW:
                return QVariant::fromValue(QCamera::ExposureSnow);
            case GST_PHOTOGRAPHY_SCENE_MODE_BEACH:
                return QVariant::fromValue(QCamera::ExposureBeach);
            case GST_PHOTOGRAPHY_SCENE_MODE_ACTION:
                return QVariant::fromValue(QCamera::ExposureAction);
            case GST_PHOTOGRAPHY_SCENE_MODE_NIGHT_PORTRAIT:
                return QVariant::fromValue(QCamera::ExposureNightPortrait);
            case GST_PHOTOGRAPHY_SCENE_MODE_THEATRE:
                return QVariant::fromValue(QCamera::ExposureTheatre);
            case GST_PHOTOGRAPHY_SCENE_MODE_SUNSET:
                return QVariant::fromValue(QCamera::ExposureSunset);
            case GST_PHOTOGRAPHY_SCENE_MODE_STEADY_PHOTO:
                return QVariant::fromValue(QCamera::ExposureSteadyPhoto);
            case GST_PHOTOGRAPHY_SCENE_MODE_FIREWORKS:
                return QVariant::fromValue(QCamera::ExposureFireworks);
            case GST_PHOTOGRAPHY_SCENE_MODE_PARTY:
                return QVariant::fromValue(QCamera::ExposureParty);
            case GST_PHOTOGRAPHY_SCENE_MODE_CANDLELIGHT:
                return QVariant::fromValue(QCamera::ExposureCandlelight);
            case GST_PHOTOGRAPHY_SCENE_MODE_BARCODE:
                return QVariant::fromValue(QCamera::ExposureBarcode);
            //no direct mapping available so mapping to auto mode
            case GST_PHOTOGRAPHY_SCENE_MODE_CLOSEUP:
            case GST_PHOTOGRAPHY_SCENE_MODE_AUTO:
            default:
                return QVariant::fromValue(QCamera::ExposureAuto);
            }
        }
        default:
            break;
        }
    }
#else
    Q_UNUSED(parameter);
#endif
    return QVariant();
}

bool QGstreamerCameraExposure::setValue(ExposureParameter parameter, const QVariant& value)
{

#if QT_CONFIG(gstreamer_photography)
    if (hasPhotography) {
        QVariant oldValue = actualValue(parameter);

        switch (parameter) {
        case QPlatformCameraExposure::ExposureCompensation:
            gst_photography_set_ev_compensation(m_camera->photography(), value.toReal());
            break;
        case QPlatformCameraExposure::ISO:
            gst_photography_set_iso_speed(m_camera->photography(), value.toInt());
            break;
        case QPlatformCameraExposure::Aperture:
            gst_photography_set_aperture(m_camera->photography(), guint(value.toReal()*1000000));
            break;
        case QPlatformCameraExposure::ShutterSpeed:
            gst_photography_set_exposure(m_camera->photography(), guint(value.toReal()*1000000));
            break;
        case QPlatformCameraExposure::ExposureMode:
        {
            QCamera::ExposureMode mode = value.value<QCamera::ExposureMode>();
            GstPhotographySceneMode sceneMode;

            gst_photography_get_scene_mode(m_camera->photography(), &sceneMode);

            switch (mode) {
            case QCamera::ExposureManual:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_MANUAL;
                break;
            case QCamera::ExposurePortrait:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_PORTRAIT;
                break;
            case QCamera::ExposureSports:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_SPORT;
                break;
            case QCamera::ExposureNight:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_NIGHT;
                break;
            case QCamera::ExposureAuto:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_AUTO;
                break;
            case QCamera::ExposureLandscape:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_LANDSCAPE;
                break;
            case QCamera::ExposureSnow:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_SNOW;
                break;
            case QCamera::ExposureBeach:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_BEACH;
                break;
            case QCamera::ExposureAction:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_ACTION;
                break;
            case QCamera::ExposureNightPortrait:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_NIGHT_PORTRAIT;
                break;
            case QCamera::ExposureTheatre:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_THEATRE;
                break;
            case QCamera::ExposureSunset:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_SUNSET;
                break;
            case QCamera::ExposureSteadyPhoto:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_STEADY_PHOTO;
                break;
            case QCamera::ExposureFireworks:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_FIREWORKS;
                break;
            case QCamera::ExposureParty:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_PARTY;
                break;
            case QCamera::ExposureCandlelight:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_CANDLELIGHT;
                break;
            case QCamera::ExposureBarcode:
                sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_BARCODE;
                break;
            default:
                break;
            }

            gst_photography_set_scene_mode(m_camera->photography(), sceneMode);
            break;
        }
        default:
            return false;
        }

        if (!qFuzzyCompare(m_requestedValues.value(parameter).toReal(), value.toReal())) {
            m_requestedValues[parameter] = value;
            emit requestedValueChanged(parameter);
        }

        QVariant newValue = actualValue(parameter);
        if (!qFuzzyCompare(oldValue.toReal(), newValue.toReal()))
            emit actualValueChanged(parameter);

        return true;
    }
#else
    Q_UNUSED(parameter);
    Q_UNUSED(value);
#endif

    return false;
}

QCamera::FlashMode QGstreamerCameraExposure::flashMode() const
{
#if QT_CONFIG(gstreamer_photography)
    if (hasPhotography) {
        GstPhotographyFlashMode flashMode;
        gst_photography_get_flash_mode(m_camera->photography(), &flashMode);

        QCamera::FlashMode mode;
        switch (flashMode) {
        default:
        case GST_PHOTOGRAPHY_FLASH_MODE_AUTO:
            mode = QCamera::FlashAuto;
            break;
        case GST_PHOTOGRAPHY_FLASH_MODE_OFF:
            mode = QCamera::FlashOff;
            break;
        case GST_PHOTOGRAPHY_FLASH_MODE_ON:
            mode = QCamera::FlashOn;
            break;
        }
        return mode;
    }
#endif
    return QCamera::FlashAuto;
}

void QGstreamerCameraExposure::setFlashMode(QCamera::FlashMode mode)
{
    Q_UNUSED(mode);

#if QT_CONFIG(gstreamer_photography)
    if (hasPhotography) {
        GstPhotographyFlashMode flashMode;
        gst_photography_get_flash_mode(m_camera->photography(), &flashMode);

        switch (mode) {
            case QCamera::FlashAuto:
                flashMode = GST_PHOTOGRAPHY_FLASH_MODE_AUTO;
                break;
            case QCamera::FlashOff:
                flashMode = GST_PHOTOGRAPHY_FLASH_MODE_OFF;
                break;
            case QCamera::FlashOn:
                flashMode = GST_PHOTOGRAPHY_FLASH_MODE_ON;
                break;
        }

        gst_photography_set_flash_mode(m_camera->photography(), flashMode);
    }
#endif
}

bool QGstreamerCameraExposure::isFlashModeSupported(QCamera::FlashMode mode) const
{
#if QT_CONFIG(gstreamer_photography)
    if (hasPhotography)
        return true;
#endif

    return mode == QCamera::FlashAuto;
}

bool QGstreamerCameraExposure::isFlashReady() const
{
#if QT_CONFIG(gstreamer_photography)
    if (hasPhotography)
        return true;
#endif

    return false;
}

QT_END_NAMESPACE
