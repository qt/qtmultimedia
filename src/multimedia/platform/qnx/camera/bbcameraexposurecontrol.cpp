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
#include "bbcameraexposurecontrol_p.h"

#include "bbcamerasession_p.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

BbCameraExposureControl::BbCameraExposureControl(BbCameraSession *session, QObject *parent)
    : QPlatformCameraExposure(parent)
    , m_session(session)
    , m_requestedExposureMode(QCamera::ExposureAuto)
{
    connect(m_session, SIGNAL(statusChanged(QCamera::Status)), this, SLOT(statusChanged(QCamera::Status)));
}

bool BbCameraExposureControl::isParameterSupported(ExposureParameter parameter) const
{
    switch (parameter) {
    case QPlatformCameraExposure::ISO:
        return false;
    case QPlatformCameraExposure::ShutterSpeed:
        return false;
    case QPlatformCameraExposure::ExposureCompensation:
        return false;
    case QPlatformCameraExposure::FlashPower:
        return false;
    case QPlatformCameraExposure::FlashCompensation:
        return false;
    case QPlatformCameraExposure::TorchPower:
        return false;
    case QPlatformCameraExposure::ExposureMode:
        return true;
    case QPlatformCameraExposure::MeteringMode:
        return false;
    default:
        return false;
    }
}

QVariantList BbCameraExposureControl::supportedParameterRange(ExposureParameter parameter, bool *continuous) const
{
    if (parameter != QPlatformCameraExposure::ExposureMode) // no other parameter supported by BB10 API at the moment
        return QVariantList();

    if (m_session->status() != QCamera::ActiveStatus) // we can query supported exposure modes only with active viewfinder
        return QVariantList();

    if (continuous)
        *continuous = false;

    int supported = 0;
    camera_scenemode_t modes[20];
    const camera_error_t result = camera_get_scene_modes(m_session->handle(), 20, &supported, modes);
    if (result != CAMERA_EOK) {
        qWarning() << "Unable to retrieve supported scene modes:" << result;
        return QVariantList();
    }

    QVariantList exposureModes;
    for (int i = 0; i < supported; ++i) {
        switch (modes[i]) {
        case CAMERA_SCENE_AUTO:
            exposureModes << QVariant::fromValue(QCamera::ExposureAuto);
            break;
        case CAMERA_SCENE_SPORTS:
            exposureModes << QVariant::fromValue(QCamera::ExposureSports);
            break;
        case CAMERA_SCENE_CLOSEUP:
            exposureModes << QVariant::fromValue(QCamera::ExposurePortrait);
            break;
        case CAMERA_SCENE_ACTION:
            exposureModes << QVariant::fromValue(QCamera::ExposureSports);
            break;
        case CAMERA_SCENE_BEACHANDSNOW:
            exposureModes << QVariant::fromValue(QCamera::ExposureBeach) << QVariant::fromValue(QCamera::ExposureSnow);
            break;
        case CAMERA_SCENE_NIGHT:
            exposureModes << QVariant::fromValue(QCamera::ExposureNight);
            break;
        default: break;
        }
    }

    return exposureModes;
}

QVariant BbCameraExposureControl::requestedValue(ExposureParameter parameter) const
{
    if (parameter != QPlatformCameraExposure::ExposureMode) // no other parameter supported by BB10 API at the moment
        return QVariant();

    return QVariant::fromValue(m_requestedExposureMode);
}

QVariant BbCameraExposureControl::actualValue(ExposureParameter parameter) const
{
    if (parameter != QPlatformCameraExposure::ExposureMode) // no other parameter supported by BB10 API at the moment
        return QVariantList();

    if (m_session->status() != QCamera::ActiveStatus) // we can query actual scene modes only with active viewfinder
        return QVariantList();

    camera_scenemode_t sceneMode = CAMERA_SCENE_DEFAULT;
    const camera_error_t result = camera_get_scene_mode(m_session->handle(), &sceneMode);

    if (result != CAMERA_EOK) {
        qWarning() << "Unable to retrieve scene mode:" << result;
        return QVariant();
    }

    switch (sceneMode) {
    case CAMERA_SCENE_AUTO:
        return QVariant::fromValue(QCamera::ExposureAuto);
    case CAMERA_SCENE_SPORTS:
        return QVariant::fromValue(QCamera::ExposureSports);
    case CAMERA_SCENE_CLOSEUP:
        return QVariant::fromValue(QCamera::ExposurePortrait);
    case CAMERA_SCENE_ACTION:
        return QVariant::fromValue(QCamera::ExposureSports);
    case CAMERA_SCENE_BEACHANDSNOW:
        return (m_requestedExposureMode == QCamera::ExposureBeach ? QVariant::fromValue(QCamera::ExposureBeach)
                                                                          : QVariant::fromValue(QCamera::ExposureSnow));
    case CAMERA_SCENE_NIGHT:
        return QVariant::fromValue(QCamera::ExposureNight);
    default:
        break;
    }

    return QVariant();
}

bool BbCameraExposureControl::setValue(ExposureParameter parameter, const QVariant& value)
{
    if (parameter != QPlatformCameraExposure::ExposureMode) // no other parameter supported by BB10 API at the moment
        return false;

    if (m_session->status() != QCamera::ActiveStatus) // we can set actual scene modes only with active viewfinder
        return false;

    camera_scenemode_t sceneMode = CAMERA_SCENE_DEFAULT;

    if (value.isValid()) {
        m_requestedExposureMode = value.value<QCamera::ExposureMode>();
        emit requestedValueChanged(QPlatformCameraExposure::ExposureMode);

        switch (m_requestedExposureMode) {
        case QCamera::ExposureAuto:
            sceneMode = CAMERA_SCENE_AUTO;
            break;
        case QCamera::ExposureSports:
            sceneMode = CAMERA_SCENE_SPORTS;
            break;
        case QCamera::ExposurePortrait:
            sceneMode = CAMERA_SCENE_CLOSEUP;
            break;
        case QCamera::ExposureBeach:
            sceneMode = CAMERA_SCENE_BEACHANDSNOW;
            break;
        case QCamera::ExposureSnow:
            sceneMode = CAMERA_SCENE_BEACHANDSNOW;
            break;
        case QCamera::ExposureNight:
            sceneMode = CAMERA_SCENE_NIGHT;
            break;
        default:
            sceneMode = CAMERA_SCENE_DEFAULT;
            break;
        }
    }

    const camera_error_t result = camera_set_scene_mode(m_session->handle(), sceneMode);

    if (result != CAMERA_EOK) {
        qWarning() << "Unable to set scene mode:" << result;
        return false;
    }

    emit actualValueChanged(QPlatformCameraExposure::ExposureMode);

    return true;
}

void BbCameraExposureControl::statusChanged(QCamera::Status status)
{
    if (status == QCamera::ActiveStatus || status == QCamera::LoadedStatus)
        emit parameterRangeChanged(QPlatformCameraExposure::ExposureMode);
}

QCamera::FlashModes BbCameraExposureControl::flashMode() const
{
    return m_flashMode;
}

void BbCameraExposureControl::setFlashMode(QCamera::FlashModes mode)
{
    if (m_flashMode == mode)
        return;

    if (m_session->status() != QCamera::ActiveStatus) // can only be changed when viewfinder is active
        return;

//    if (m_flashMode == QCamera::FlashVideoLight) {
//        const camera_error_t result = camera_config_videolight(m_session->handle(), CAMERA_VIDEOLIGHT_OFF);
//        if (result != CAMERA_EOK)
//            qWarning() << "Unable to switch off video light:" << result;
//    }

    m_flashMode = mode;

//    if (m_flashMode == QCamera::FlashVideoLight) {
//        const camera_error_t result = camera_config_videolight(m_session->handle(), CAMERA_VIDEOLIGHT_ON);
//        if (result != CAMERA_EOK)
//            qWarning() << "Unable to switch on video light:" << result;
//    } else
    {
        camera_flashmode_t flashMode = CAMERA_FLASH_AUTO;

        if (m_flashMode == QCamera::FlashAuto)
            flashMode = CAMERA_FLASH_AUTO;
        else if (mode == QCamera::FlashOff)
            flashMode = CAMERA_FLASH_OFF;
        else if (mode == QCamera::FlashOn)
            flashMode = CAMERA_FLASH_ON;

        const camera_error_t result = camera_config_flash(m_session->handle(), flashMode);
        if (result != CAMERA_EOK)
            qWarning() << "Unable to configure flash:" << result;
    }
}

bool BbCameraExposureControl::isFlashModeSupported(QCamera::FlashModes mode) const
{
    // ### Videolight maps to QCamera::TorchOn.
//    bool supportsVideoLight = false;
//    if (m_session->handle() != CAMERA_HANDLE_INVALID) {
//        supportsVideoLight = camera_has_feature(m_session->handle(), CAMERA_FEATURE_VIDEOLIGHT);
//    }

    // ### is this correct?
    return true;
}

bool BbCameraExposureControl::isFlashReady() const
{
    //TODO: check for flash charge-level here?!?
    return true;
}

QT_END_NAMESPACE
