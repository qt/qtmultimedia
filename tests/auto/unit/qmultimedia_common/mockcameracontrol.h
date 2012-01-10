/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef MOCKCAMERACONTROL_H
#define MOCKCAMERACONTROL_H

#include "qcameracontrol.h"

class MockCameraControl : public QCameraControl
{
    friend class MockCaptureControl;
    Q_OBJECT
public:
    MockCameraControl(QObject *parent = 0):
            QCameraControl(parent),
            m_state(QCamera::UnloadedState),
            m_captureMode(QCamera::CaptureStillImage),
            m_status(QCamera::UnloadedStatus),
            m_propertyChangesSupported(false)
    {
    }

    ~MockCameraControl() {}

    void start() { m_state = QCamera::ActiveState; }
    virtual void stop() { m_state = QCamera::UnloadedState; }
    QCamera::State state() const { return m_state; }
    void setState(QCamera::State state) {
        if (m_state != state) {
            m_state = state;

            switch (state) {
            case QCamera::UnloadedState:
                m_status = QCamera::UnloadedStatus;
                break;
            case QCamera::LoadedState:
                m_status = QCamera::LoadedStatus;
                break;
            case QCamera::ActiveState:
                m_status = QCamera::ActiveStatus;
                break;
            default:
                emit error(QCamera::NotSupportedFeatureError, "State not supported.");
                return;
            }

            emit stateChanged(m_state);
            emit statusChanged(m_status);
        }
    }

    QCamera::Status status() const { return m_status; }

    QCamera::CaptureMode captureMode() const { return m_captureMode; }
    void setCaptureMode(QCamera::CaptureMode mode)
    {
        if (m_captureMode != mode) {
            if (m_state == QCamera::ActiveState && !m_propertyChangesSupported)
                return;
            m_captureMode = mode;
            emit captureModeChanged(mode);
        }
    }

    bool isCaptureModeSupported(QCamera::CaptureMode mode) const
    {
        return mode == QCamera::CaptureStillImage || mode == QCamera::CaptureVideo;
    }

    QCamera::LockTypes supportedLocks() const
    {
        return QCamera::LockExposure | QCamera::LockFocus | QCamera::LockWhiteBalance;
    }

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const
    {
        Q_UNUSED(status);
        if (changeType == QCameraControl::ImageEncodingSettings && m_captureMode == QCamera::CaptureVideo)
            return true;
        else if (changeType== QCameraControl::VideoEncodingSettings)
            return true;
        else
            return m_propertyChangesSupported;
    }

    /* helper method to emit the signal error */
    void setError(QCamera::Error err, QString errorString)
    {
        emit error(err, errorString);
    }

    /* helper method to emit the signal statusChaged */
    void setStatus(QCamera::Status newStatus)
    {
        m_status = newStatus;
        emit statusChanged(newStatus);
    }

    QCamera::State m_state;
    QCamera::CaptureMode m_captureMode;
    QCamera::Status m_status;
    bool m_propertyChangesSupported;
};



#endif // MOCKCAMERACONTROL_H
