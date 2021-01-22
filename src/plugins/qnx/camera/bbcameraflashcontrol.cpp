/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/
#include "bbcameraflashcontrol.h"

#include "bbcamerasession.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

BbCameraFlashControl::BbCameraFlashControl(BbCameraSession *session, QObject *parent)
    : QCameraFlashControl(parent)
    , m_session(session)
    , m_flashMode(QCameraExposure::FlashAuto)
{
}

QCameraExposure::FlashModes BbCameraFlashControl::flashMode() const
{
    return m_flashMode;
}

void BbCameraFlashControl::setFlashMode(QCameraExposure::FlashModes mode)
{
    if (m_flashMode == mode)
        return;

    if (m_session->status() != QCamera::ActiveStatus) // can only be changed when viewfinder is active
        return;

    if (m_flashMode == QCameraExposure::FlashVideoLight) {
        const camera_error_t result = camera_config_videolight(m_session->handle(), CAMERA_VIDEOLIGHT_OFF);
        if (result != CAMERA_EOK)
            qWarning() << "Unable to switch off video light:" << result;
    }

    m_flashMode = mode;

    if (m_flashMode == QCameraExposure::FlashVideoLight) {
        const camera_error_t result = camera_config_videolight(m_session->handle(), CAMERA_VIDEOLIGHT_ON);
        if (result != CAMERA_EOK)
            qWarning() << "Unable to switch on video light:" << result;
    } else {
        camera_flashmode_t flashMode = CAMERA_FLASH_AUTO;

        if (m_flashMode.testFlag(QCameraExposure::FlashAuto)) flashMode = CAMERA_FLASH_AUTO;
        else if (mode.testFlag(QCameraExposure::FlashOff)) flashMode = CAMERA_FLASH_OFF;
        else if (mode.testFlag(QCameraExposure::FlashOn)) flashMode = CAMERA_FLASH_ON;

        const camera_error_t result = camera_config_flash(m_session->handle(), flashMode);
        if (result != CAMERA_EOK)
            qWarning() << "Unable to configure flash:" << result;
    }
}

bool BbCameraFlashControl::isFlashModeSupported(QCameraExposure::FlashModes mode) const
{
    bool supportsVideoLight = false;
    if (m_session->handle() != CAMERA_HANDLE_INVALID) {
        supportsVideoLight = camera_has_feature(m_session->handle(), CAMERA_FEATURE_VIDEOLIGHT);
    }

    return  (mode == QCameraExposure::FlashOff ||
             mode == QCameraExposure::FlashOn ||
             mode == QCameraExposure::FlashAuto ||
             ((mode == QCameraExposure::FlashVideoLight) && supportsVideoLight));
}

bool BbCameraFlashControl::isFlashReady() const
{
    //TODO: check for flash charge-level here?!?
    return true;
}

QT_END_NAMESPACE
