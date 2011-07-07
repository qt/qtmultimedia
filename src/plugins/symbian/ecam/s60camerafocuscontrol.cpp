/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#include <QtCore/qstring.h>

#include "s60camerafocuscontrol.h"
#include "s60cameraservice.h"
#include "s60imagecapturesession.h"
#include "s60cameraconstants.h"

S60CameraFocusControl::S60CameraFocusControl(QObject *parent) :
    QCameraFocusControl(parent)
{
}

S60CameraFocusControl::S60CameraFocusControl(S60ImageCaptureSession *session, QObject *parent) :
    QCameraFocusControl(parent),
    m_session(0),
    m_service(0),
    m_advancedSettings(0),
    m_isFocusLocked(false),
    m_opticalZoomValue(KDefaultOpticalZoom),
    m_digitalZoomValue(KDefaultDigitalZoom),
    m_focusMode(KDefaultFocusMode)
{
    m_session = session;

    connect(m_session, SIGNAL(advancedSettingChanged()), this, SLOT(resetAdvancedSetting()));
    m_advancedSettings = m_session->advancedSettings();

    TRAPD(err, m_session->doSetZoomFactorL(m_opticalZoomValue, m_digitalZoomValue));
    if (err)
        m_session->setError(KErrNotSupported, tr("Setting default zoom factors failed."));
}

S60CameraFocusControl::~S60CameraFocusControl()
{
}

QCameraFocus::FocusMode S60CameraFocusControl::focusMode() const
{
    return m_focusMode;
}

void S60CameraFocusControl::setFocusMode(QCameraFocus::FocusMode mode)
{
    if (isFocusModeSupported(mode)) {
        // FocusMode and FocusRange are set. Focusing is triggered by setting
        // the corresponding FocusType active by calling searchAndLock in LocksControl.
        m_focusMode = mode;
        if (m_advancedSettings)
            m_advancedSettings->setFocusMode(m_focusMode);
        else
            m_session->setError(KErrGeneral, tr("Unable to set focus mode before camera is started."));
    } else {
        m_session->setError(KErrNotSupported, tr("Requested focus mode is not supported."));
    }
}

bool S60CameraFocusControl::isFocusModeSupported(QCameraFocus::FocusMode mode) const
{
    if (m_advancedSettings) {
        return m_advancedSettings->supportedFocusModes() & mode;
    } else {
        if (mode == QCameraFocus::AutoFocus)
            return m_session->isFocusSupported();
    }

    return false;
}

qreal S60CameraFocusControl::maximumOpticalZoom() const
{
    return m_session->maximumZoom();
}

qreal S60CameraFocusControl::maximumDigitalZoom() const
{
    return m_session->maxDigitalZoom();
}

qreal S60CameraFocusControl::opticalZoom() const
{
    return m_session->opticalZoomFactor();
}

qreal S60CameraFocusControl::digitalZoom() const
{
    return m_session->digitalZoomFactor();
}

void S60CameraFocusControl::zoomTo(qreal optical, qreal digital)
{
    TRAPD(err, m_session->doSetZoomFactorL(optical, digital));
    if (err)
        m_session->setError(KErrNotSupported, tr("Requested zoom factor is not supported."));

    // Query new values
    if (m_opticalZoomValue != m_session->opticalZoomFactor()) {
        m_opticalZoomValue = m_session->opticalZoomFactor();
        emit opticalZoomChanged(m_opticalZoomValue);
    }
    if (m_digitalZoomValue != m_session->digitalZoomFactor()) {
        m_digitalZoomValue = m_session->digitalZoomFactor();
        emit digitalZoomChanged(m_digitalZoomValue);
    }
}

void S60CameraFocusControl::resetAdvancedSetting()
{
    m_advancedSettings = m_session->advancedSettings();
}

QCameraFocus::FocusPointMode S60CameraFocusControl::focusPointMode() const
{
    // Not supported in Symbian
    return QCameraFocus::FocusPointAuto;
}

void S60CameraFocusControl::setFocusPointMode(QCameraFocus::FocusPointMode mode)
{
    if (mode != QCameraFocus::FocusPointAuto)
        m_session->setError(KErrNotSupported, tr("Requested focus point mode is not supported."));
}

bool S60CameraFocusControl::isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const
{
    // Not supported in Symbian
    if (mode == QCameraFocus::FocusPointAuto)
        return true;
    else
        return false;
}

QPointF S60CameraFocusControl::customFocusPoint() const
{
    // Not supported in Symbian, return image center
    return QPointF(0.5, 0.5);
}

void S60CameraFocusControl::setCustomFocusPoint(const QPointF &point)
{
    // Not supported in Symbian
    Q_UNUSED(point);
    m_session->setError(KErrNotSupported, tr("Setting custom focus point is not supported."));
}

QCameraFocusZoneList S60CameraFocusControl::focusZones() const
{
    // Not supported in Symbian
    return QCameraFocusZoneList(); // Return empty list
}

// End of file

