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

#ifndef MOCKCAMERACONTROL_H
#define MOCKCAMERACONTROL_H

#include "qcameracontrol.h"
#include "qcamerainfo.h"
#include <qtimer.h>

class MockCameraControl : public QCameraControl
{
    friend class MockCaptureControl;
    Q_OBJECT
public:
    MockCameraControl(QObject *parent = 0):
            QCameraControl(parent),
            m_state(QCamera::UnloadedState),
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
            }

            emit stateChanged(m_state);
            emit statusChanged(m_status);
        }
    }

    QCamera::Status status() const { return m_status; }

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const
    {
        Q_UNUSED(status);
        if (changeType == QCameraControl::ImageEncodingSettings)
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

    void setCamera(const QCameraInfo &camera)
    {
        m_camera = camera;
    }

    void setVideoSurface(QAbstractVideoSurface *) {}

    QCamera::State m_state;
    QCamera::Status m_status;
    QCameraInfo m_camera;
    bool m_propertyChangesSupported;
};



#endif // MOCKCAMERACONTROL_H
