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

#include "qgstreamercamerafocus_p.h"
#include "qgstreamercamera_p.h"

#include <QDebug>
#include <QtCore/qcoreevent.h>
#include <QtCore/qmetaobject.h>

#include <private/qgstutils_p.h>

#define ZOOM_PROPERTY "zoom"
#define MAX_ZOOM_PROPERTY "max-zoom"

//#define CAMERABIN_DEBUG 1

QT_BEGIN_NAMESPACE

QGstreamerCameraFocus::QGstreamerCameraFocus(QGstreamerCamera *session)
  : QPlatformCameraFocus(session),
    m_camera(session),
    m_focusMode(QCamera::FocusModeAuto)
{
#if QT_CONFIG(gstreamer_photography)
    auto photography = m_camera->photography();
    if (photography)
        gst_photography_set_focus_mode(m_camera->photography(), GST_PHOTOGRAPHY_FOCUS_MODE_CONTINUOUS_NORMAL);
#endif
}

QGstreamerCameraFocus::~QGstreamerCameraFocus()
{
}

QCamera::FocusMode QGstreamerCameraFocus::focusMode() const
{
    return m_focusMode;
}

void QGstreamerCameraFocus::setFocusMode(QCamera::FocusMode mode)
{
    if (mode == m_focusMode)
        return;

#if QT_CONFIG(gstreamer_photography)
    auto photography = m_camera->photography();
    if (photography) {
        GstPhotographyFocusMode photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_CONTINUOUS_NORMAL;

        switch (mode) {
        case QCamera::FocusModeAutoNear:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_MACRO;
            break;
        case QCamera::FocusModeAutoFar:
            // not quite, but hey :)
            Q_FALLTHROUGH();
        case QCamera::FocusModeHyperfocal:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_HYPERFOCAL;
            break;
        case QCamera::FocusModeInfinity:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_INFINITY;
            break;
        case QCamera::FocusModeManual:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_MANUAL;
            break;
        default: // QCamera::FocusModeAuto:
            break;
        }

        if (gst_photography_set_focus_mode(photography, photographyMode)) {
            m_focusMode = mode;
            emit focusModeChanged(m_focusMode);
        }
    }
#endif
}

bool QGstreamerCameraFocus::isFocusModeSupported(QCamera::FocusMode mode) const
{
    Q_UNUSED(mode);

#if QT_CONFIG(gstreamer_photography)
    auto photography = m_camera->photography();
    if (photography)
        return true;
#endif

    return false;
}

QPlatformCameraFocus::ZoomRange QGstreamerCameraFocus::zoomFactorRange() const
{
    // We do some heuristics here and support zooming in until a
    // resolution of 320x240 or max 4x

    return { 1., 1. };
}


void QGstreamerCameraFocus::zoomTo(float newZoomFactor, float /*rate*/)
{
    auto range = zoomFactorRange();
    newZoomFactor = qBound(range.min, newZoomFactor, range.max);
    // #### Now do the zooming
}

QT_END_NAMESPACE
