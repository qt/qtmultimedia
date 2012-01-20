/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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
        m_opticalZoom(1.0),
        m_digitalZoom(1.0),
        m_focusMode(QCameraFocus::AutoFocus),
        m_focusPointMode(QCameraFocus::FocusPointAuto),
        m_focusPoint(0.5, 0.5),
        m_maxOpticalZoom(3.0),
        m_maxDigitalZoom(4.0)

    {
        m_zones << QCameraFocusZone(QRectF(0.45, 0.45, 0.1, 0.1));
    }

    ~MockCameraFocusControl() {}

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
        return mode == QCameraFocus::AutoFocus || mode == QCameraFocus::ContinuousFocus;
    }

    qreal maximumOpticalZoom() const
    {
        return m_maxOpticalZoom;
    }

    qreal maximumDigitalZoom() const
    {
        return m_maxDigitalZoom;
    }

    qreal opticalZoom() const
    {
        return m_opticalZoom;
    }

    qreal digitalZoom() const
    {
        return m_digitalZoom;
    }

    void zoomTo(qreal optical, qreal digital)
    {
        optical = qBound<qreal>(1.0, optical, maximumOpticalZoom());
        digital = qBound<qreal>(1.0, digital, maximumDigitalZoom());

        if (!qFuzzyCompare(digital, m_digitalZoom)) {
            m_digitalZoom = digital;
            emit digitalZoomChanged(m_digitalZoom);
        }

        if (!qFuzzyCompare(optical, m_opticalZoom)) {
            m_opticalZoom = optical;
            emit opticalZoomChanged(m_opticalZoom);
        }

        maxOpticalDigitalZoomChange(4.0, 5.0);
        focusZonesChange(0.50, 0.50, 0.3, 0.3);
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
    }

    QCameraFocusZoneList focusZones() const
    {
        return m_zones;
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

private:
    qreal m_opticalZoom;
    qreal m_digitalZoom;
    QCameraFocus::FocusMode m_focusMode;
    QCameraFocus::FocusPointMode m_focusPointMode;
    QPointF m_focusPoint;
    // to emit maximum Optical and Digital Zoom Changed signals
    qreal m_maxOpticalZoom;
    qreal m_maxDigitalZoom;
    // to emit focus zone changed signal
    QCameraFocusZoneList m_zones;
};

#endif // MOCKCAMERAFOCUSCONTROL_H
