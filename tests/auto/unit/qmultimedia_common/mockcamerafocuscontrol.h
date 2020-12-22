/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qcamerafocuscontrol.h"
#include "qcamerafocus.h"

class MockCameraFocusControl : public QCameraFocusControl
{
    Q_OBJECT
public:
    MockCameraFocusControl(QObject *parent = 0):
        QCameraFocusControl(parent),
        m_focusMode(QCameraFocus::AutoFocus),
        m_focusPointMode(QCameraFocus::FocusPointAuto),
        m_focusPoint(0.5, 0.5)
    {
        m_zones << QCameraFocusZone(QRectF(0.45, 0.45, 0.1, 0.1));
    }

    ~MockCameraFocusControl() {}

    QCameraFocus::FocusModes focusMode() const
    {
        return m_focusMode;
    }

    void setFocusMode(QCameraFocus::FocusModes mode)
    {
        if (isFocusModeSupported(mode))
            m_focusMode = mode;
    }

    bool isFocusModeSupported(QCameraFocus::FocusModes mode) const
    {
        return mode == QCameraFocus::AutoFocus || mode == QCameraFocus::ContinuousFocus;
    }

    QCameraFocus::FocusPointMode focusPointMode() const
    {
        return m_focusPointMode;
    }

    void setFocusPointMode(QCameraFocus::FocusPointMode mode)
    {
        if (isFocusPointModeSupported(mode))
            m_focusPointMode = mode;
    }

    bool isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const
    {
        switch (mode) {
        case QCameraFocus::FocusPointAuto:
        case QCameraFocus::FocusPointCenter:
        case QCameraFocus::FocusPointCustom:
            return true;
        default:
            return false;
        }
    }

    QPointF customFocusPoint() const
    {
        return m_focusPoint;
    }

    void setCustomFocusPoint(const QPointF &point)
    {
        m_focusPoint = point;
        focusZonesChange(0.50, 0.50, 0.3, 0.3);
    }

    QCameraFocusZoneList focusZones() const
    {
        return m_zones;
    }

    // helper function to emit Focus Zones Changed signals
    void focusZonesChange(qreal left, qreal top, qreal width, qreal height)
    {
        QCameraFocusZone myZone(QRectF(left, top, width, height));
        if (m_zones.last().area() != myZone.area()) {
            m_zones.clear();
            m_zones << myZone;
            emit focusZonesChanged();
        }
    }

    qreal maximumOpticalZoom() const
    {
        return m_maxOpticalZoom;
    }

    qreal maximumDigitalZoom() const
    {
        return m_maxDigitalZoom;
    }

    qreal currentOpticalZoom() const
    {
        return m_opticalZoom;
    }

    qreal currentDigitalZoom() const
    {
        return m_digitalZoom;
    }

    qreal requestedOpticalZoom() const
    {
        return m_opticalZoom;
    }

    qreal requestedDigitalZoom() const
    {
        return m_digitalZoom;
    }

    void zoomTo(qreal optical, qreal digital)
    {
        optical = qBound<qreal>(1.0, optical, maximumOpticalZoom());
        digital = qBound<qreal>(1.0, digital, maximumDigitalZoom());

        if (!qFuzzyCompare(digital, m_digitalZoom)) {
            m_digitalZoom = digital;
            emit requestedDigitalZoomChanged(m_digitalZoom);
            emit currentDigitalZoomChanged(m_digitalZoom);
        }

        if (!qFuzzyCompare(optical, m_opticalZoom)) {
            m_opticalZoom = optical;
            emit requestedOpticalZoomChanged(m_opticalZoom);
            emit currentOpticalZoomChanged(m_opticalZoom);
        }

        maxOpticalDigitalZoomChange(4.0, 5.0);
    }

    // helper function to emit maximum Optical and Digital Zoom Changed signals
    void maxOpticalDigitalZoomChange(qreal maxOptical, qreal maxDigital)
    {
        if (maxOptical != m_maxOpticalZoom) {
            m_maxOpticalZoom = maxOptical;
            emit maximumOpticalZoomChanged(m_maxOpticalZoom);
        }

        if (maxDigital != m_maxDigitalZoom) {
            m_maxDigitalZoom = maxDigital;
            emit maximumDigitalZoomChanged(m_maxDigitalZoom);
        }
    }

private:
    QCameraFocus::FocusModes m_focusMode;
    QCameraFocus::FocusPointMode m_focusPointMode;
    QPointF m_focusPoint;
    // to emit focus zone changed signal
    QCameraFocusZoneList m_zones;

    qreal m_opticalZoom = 1.;
    qreal m_digitalZoom = 1.;
    qreal m_maxOpticalZoom = 3.;
    qreal m_maxDigitalZoom = 4.;
};

#endif // MOCKCAMERAFOCUSCONTROL_H
