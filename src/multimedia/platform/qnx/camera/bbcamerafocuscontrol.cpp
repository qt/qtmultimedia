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
#include "bbcamerafocuscontrol_p.h"

#include "bbcamerasession_p.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

BbCameraFocusControl::BbCameraFocusControl(BbCameraSession *session, QObject *parent)
    : QPlatformCameraFocus(parent)
    , m_session(session)
    , m_focusMode(QCamera::FocusModeAuto)
    , m_customFocusPoint(QPointF(0, 0))
{
    connect(m_session, SIGNAL(statusChanged(QCamera::Status)), this, SLOT(statusChanged(QCamera::Status)));
}

QCamera::FocusMode BbCameraFocusControl::focusMode() const
{
    camera_focusmode_t focusMode = CAMERA_FOCUSMODE_OFF;

    const camera_error_t result = camera_get_focus_mode(m_session->handle(), &focusMode);
    if (result != CAMERA_EOK) {
        qWarning() << "Unable to retrieve focus mode from camera:" << result;
        return QCamera::FocusModeAuto;
    }

    switch (focusMode) {
    case CAMERA_FOCUSMODE_EDOF:
        return QCamera::FocusModeHyperfocal;
    case CAMERA_FOCUSMODE_MANUAL:
        return QCamera::FocusModeManual;
    case CAMERA_FOCUSMODE_CONTINUOUS_MACRO: // fall through
    case CAMERA_FOCUSMODE_MACRO:
        return QCamera::FocusModeAutoNear;
    case CAMERA_FOCUSMODE_AUTO: // fall through
    case CAMERA_FOCUSMODE_CONTINUOUS_AUTO:
        return QCamera::FocusModeAuto;
    case CAMERA_FOCUSMODE_OFF:
    default:
        return QCamera::FocusModeAuto;
    }
}

void BbCameraFocusControl::setFocusMode(QCamera::FocusMode mode)
{
    if (m_focusMode == mode)
        return;

    camera_focusmode_t focusMode = CAMERA_FOCUSMODE_OFF;

    switch (mode) {
    case QCamera::FocusModeHyperfocal:
    case QCamera::FocusModeInfinity: // not 100%, but close
        focusMode = CAMERA_FOCUSMODE_EDOF;
        break;
    case QCamera::FocusModeManual:
        focusMode = CAMERA_FOCUSMODE_MANUAL;
        break;
    case QCamera::FocusModeAutoNear:
        focusMode = CAMERA_FOCUSMODE_MACRO;
        break;
    case QCamera::FocusModeAuto:
    case QCamera::FocusModeAutoFar:
        focusMode = CAMERA_FOCUSMODE_CONTINUOUS_AUTO;
        break;
    }

    const camera_error_t result = camera_set_focus_mode(m_session->handle(), focusMode);

    if (result != CAMERA_EOK) {
        qWarning() << "Unable to set focus mode:" << result;
        return;
    }

    m_focusMode = mode;
    emit focusModeChanged(m_focusMode);
}

bool BbCameraFocusControl::isFocusModeSupported(QCamera::FocusMode mode) const
{
    if (m_session->state() == QCamera::UnloadedState)
        return false;

    if (mode == QCamera::FocusModeHyperfocal)
        return false; //TODO how to check?
    else if (mode == QCamera::FocusModeManual)
        return camera_has_feature(m_session->handle(), CAMERA_FEATURE_MANUALFOCUS);
    else if (mode == QCamera::FocusModeAuto)
        return camera_has_feature(m_session->handle(), CAMERA_FEATURE_AUTOFOCUS);
    else if (mode == QCamera::FocusModeAutoNear)
        return camera_has_feature(m_session->handle(), CAMERA_FEATURE_MACROFOCUS);

    return false;
}

QPointF BbCameraFocusControl::focusPoint() const
{
    return m_customFocusPoint;
}

void BbCameraFocusControl::setCustomFocusPoint(const QPointF &point)
{
    if (m_customFocusPoint == point)
        return;

    m_customFocusPoint = point;
    emit customFocusPointChanged(m_customFocusPoint);

    updateCustomFocusRegion();
}

void BbCameraFocusControl::updateCustomFocusRegion()
{
    // get the size of the viewfinder
    int viewfinderWidth = 0;
    int viewfinderHeight = 0;

    if (!retrieveViewfinderSize(&viewfinderWidth, &viewfinderHeight))
        return;

    // define a 40x40 pixel focus region around the custom focus point
    camera_region_t focusRegion;
    focusRegion.left = qMax(0, static_cast<int>(m_customFocusPoint.x() * viewfinderWidth) - 20);
    focusRegion.top = qMax(0, static_cast<int>(m_customFocusPoint.y() * viewfinderHeight) - 20);
    focusRegion.width = 40;
    focusRegion.height = 40;

    camera_error_t result = camera_set_focus_regions(m_session->handle(), 1, &focusRegion);
    if (result != CAMERA_EOK) {
        qWarning() << "Unable to set focus region:" << result;
        return;
    }

    // re-set focus mode to apply focus region changes
    camera_focusmode_t focusMode = CAMERA_FOCUSMODE_OFF;
    result = camera_get_focus_mode(m_session->handle(), &focusMode);
    camera_set_focus_mode(m_session->handle(), focusMode);
}

bool BbCameraFocusControl::retrieveViewfinderSize(int *width, int *height)
{
    if (!width || !height)
        return false;

    camera_error_t result = CAMERA_EOK;
    if (m_session->captureMode() & QCamera::CaptureStillImage)
        result = camera_get_photovf_property(m_session->handle(),
                                             CAMERA_IMGPROP_WIDTH, width,
                                             CAMERA_IMGPROP_HEIGHT, height);
    else if (m_session->captureMode() & QCamera::CaptureVideo)
        result = camera_get_videovf_property(m_session->handle(),
                                             CAMERA_IMGPROP_WIDTH, width,
                                             CAMERA_IMGPROP_HEIGHT, height);

    if (result != CAMERA_EOK) {
        qWarning() << "Unable to retrieve viewfinder size:" << result;
        return false;
    }

    return true;
}


qreal BbCameraFocusControl::maximumOpticalZoom() const
{
    //TODO: optical zoom support not available in BB10 API yet
    return 1.0;
}

qreal BbCameraFocusControl::maximumDigitalZoom() const
{
    return m_maximumZoomFactor;
}

qreal BbCameraFocusControl::requestedOpticalZoom() const
{
    //TODO: optical zoom support not available in BB10 API yet
    return 1.0;
}

qreal BbCameraFocusControl::requestedDigitalZoom() const
{
    return currentDigitalZoom();
}

qreal BbCameraFocusControl::currentOpticalZoom() const
{
    //TODO: optical zoom support not available in BB10 API yet
    return 1.0;
}

qreal BbCameraFocusControl::currentDigitalZoom() const
{
    if (m_session->status() != QCamera::ActiveStatus)
        return 1.0;

    unsigned int zoomFactor = 0;
    camera_error_t result = CAMERA_EOK;

    if (m_session->captureMode() & QCamera::CaptureStillImage)
        result = camera_get_photovf_property(m_session->handle(), CAMERA_IMGPROP_ZOOMFACTOR, &zoomFactor);
    else if (m_session->captureMode() & QCamera::CaptureVideo)
        result = camera_get_videovf_property(m_session->handle(), CAMERA_IMGPROP_ZOOMFACTOR, &zoomFactor);

    if (result != CAMERA_EOK)
        return 1.0;

    return zoomFactor;
}

void BbCameraFocusControl::zoomTo(qreal optical, qreal digital)
{
    Q_UNUSED(optical);

    if (m_session->status() != QCamera::ActiveStatus)
        return;

    const qreal actualZoom = qBound(m_minimumZoomFactor, digital, m_maximumZoomFactor);

    const camera_error_t result = camera_set_zoom(m_session->handle(), actualZoom, false);

    if (result != CAMERA_EOK) {
        qWarning() << "Unable to change zoom factor:" << result;
        return;
    }

    if (m_requestedZoomFactor != digital) {
        m_requestedZoomFactor = digital;
        emit requestedDigitalZoomChanged(m_requestedZoomFactor);
    }

    emit currentDigitalZoomChanged(actualZoom);
}

void BbCameraFocusControl::statusChanged(QCamera::Status status)
{
    if (status == QCamera::ActiveStatus) {
        // retrieve information about zoom limits
        unsigned int maximumZoomLimit = 0;
        unsigned int minimumZoomLimit = 0;
        bool smoothZoom = false;

        const camera_error_t result = camera_get_zoom_limits(m_session->handle(), &maximumZoomLimit, &minimumZoomLimit, &smoothZoom);
        if (result == CAMERA_EOK) {
            const qreal oldMaximumZoomFactor = m_maximumZoomFactor;
            m_maximumZoomFactor = maximumZoomLimit;

            if (oldMaximumZoomFactor != m_maximumZoomFactor)
                emit maximumDigitalZoomChanged(m_maximumZoomFactor);

            m_minimumZoomFactor = minimumZoomLimit;
            m_supportsSmoothZoom = smoothZoom;
        } else {
            m_maximumZoomFactor = 1.0;
            m_minimumZoomFactor = 1.0;
            m_supportsSmoothZoom = false;
        }
    }
}

QT_END_NAMESPACE
