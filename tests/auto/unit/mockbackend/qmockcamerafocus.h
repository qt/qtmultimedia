/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKCAMERAFOCUSCONTROL_H
#define MOCKCAMERAFOCUSCONTROL_H

#include "private/qplatformcamerafocus_p.h"
#include "qcamerafocus.h"

class QMockCameraFocus : public QPlatformCameraFocus
{
    Q_OBJECT
public:
    QMockCameraFocus(QObject *parent = 0):
        QPlatformCameraFocus(parent),
        m_focusMode(QCameraFocus::AutoFocus),
        m_focusPoint(0.5, 0.5)
    {
    }

    ~QMockCameraFocus() {}

    QCameraFocus::FocusMode focusMode() const
    {
        return m_focusMode;
    }

    void setFocusMode(QCameraFocus::FocusMode mode)
    {
        if (isFocusModeSupported(mode))
            m_focusMode = mode;
    }

    bool isFocusModeSupported(QCameraFocus::FocusMode mode) const
    {
        return mode == QCameraFocus::FocusModeAuto;
    }

    QPointF focusPoint() const
    {
        return m_focusPoint;
    }

    void setCustomFocusPoint(const QPointF &point)
    {
        m_focusPoint = point;
    }

    ZoomRange zoomFactorRange() const {
        return { 1., m_maxZoom };
    }

    void zoomTo(float zoom, float /*rate*/)
    {
        m_zoom = zoom;
    }

    void setMaxZoomFactor(float factor) {
        m_maxZoom = factor;
        emit maximumZoomFactorChanged(m_maxZoom);
    }

private:
    QCameraFocus::FocusMode m_focusMode;
    QPointF m_focusPoint;

    float m_zoom = 1.;
    float m_maxZoom = 4.;
};

#endif // MOCKCAMERAFOCUSCONTROL_H
