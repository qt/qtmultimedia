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
#include "qqnxcamera_p.h"

#include "qqnxmediacapture_p.h"
#include <qcameradevice.h>
#include <qmediadevices.h>

#include <camera/camera_api.h>
#include <camera/camera_3a.h>

QT_BEGIN_NAMESPACE

QQnxCamera::QQnxCamera(QCamera *parent)
    : QPlatformCamera(parent)
{
    m_camera = QMediaDevices::defaultVideoInput();
}

bool QQnxCamera::isActive() const
{
    return m_handle != CAMERA_HANDLE_INVALID;
}

void QQnxCamera::setActive(bool active)
{
    if (active) {
        if (m_handle)
            return;
        auto error = camera_open(m_cameraUnit, CAMERA_MODE_RO|CAMERA_MODE_PWRITE, &m_handle);
        if (error != CAMERA_EOK) {
            qWarning() << "Failed to open camera" << error;
            return;
        }
    } else {
        if (!m_handle)
            return;
        auto error = camera_close(m_handle);
        m_handle = CAMERA_HANDLE_INVALID;
        if (error != CAMERA_EOK) {
            qWarning() << "Failed to close camera" << error;
            return;
        }
    }
}

void QQnxCamera::setCamera(const QCameraDevice &camera)
{
    if (m_camera == camera)
        return;
    m_camera = camera;
    m_cameraUnit = camera_unit_t(camera.id().toUInt());
}

void QQnxCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    if (m_session == session)
        return;
    m_session = static_cast<QQnxMediaCaptureSession *>(session);
}

camera_focusmode_t qnxFocusMode(QCamera::FocusMode mode)
{
    switch (mode) {
    default:
    case QCamera::FocusModeAuto:
    case QCamera::FocusModeAutoFar:
    case QCamera::FocusModeInfinity:
        return CAMERA_FOCUSMODE_CONTINUOUS_AUTO;
    case QCamera::FocusModeAutoNear:
        return CAMERA_FOCUSMODE_CONTINUOUS_MACRO;
    case QCamera::FocusModeHyperfocal:
        return CAMERA_FOCUSMODE_EDOF;
    case QCamera::FocusModeManual:
        return CAMERA_FOCUSMODE_MANUAL;
    }
}

bool QQnxCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    if (!m_handle)
        return false;

    camera_focusmode_t focusModes[CAMERA_FOCUSMODE_NUMFOCUSMODES];
    int nFocusModes = 0;
    auto error = camera_get_focus_modes(m_handle, CAMERA_FOCUSMODE_NUMFOCUSMODES, &nFocusModes, focusModes);
    if (error != CAMERA_EOK || nFocusModes == 0)
        return false;

    auto qnxMode = qnxFocusMode(mode);
    for (int i = 0; i < nFocusModes; ++i) {
        if (focusModes[i] == qnxMode)
            return true;
    }
    return false;
}

void QQnxCamera::setFocusMode(QCamera::FocusMode mode)
{
    if (!m_handle)
        return;

    auto qnxMode = qnxFocusMode(mode);
    const camera_error_t result = camera_set_focus_mode(m_handle, qnxMode);

    if (result != CAMERA_EOK) {
        qWarning() << "Unable to set focus mode:" << result;
        return;
    }

    focusModeChanged(mode);
}

camera_handle_t QQnxCamera::handle() const
{
    return m_handle;
}

QT_END_NAMESPACE
