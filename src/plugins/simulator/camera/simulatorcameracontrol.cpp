/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
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

#include "simulatorcameracontrol.h"

#include <QtCore/qfile.h>

SimulatorCameraControl::SimulatorCameraControl(SimulatorCameraSession *session)
    :QCameraControl(session),
    m_session(session),
    mState(QCamera::UnloadedState),
    mStatus(QCamera::UnloadedStatus)
{
}

SimulatorCameraControl::~SimulatorCameraControl()
{
}

QCamera::CaptureModes SimulatorCameraControl::captureMode() const
{
    return m_session->captureMode();
}

void SimulatorCameraControl::setCaptureMode(QCamera::CaptureModes mode)
{
    if (m_session->captureMode() != mode) {
        m_session->setCaptureMode(mode);
        emit captureModeChanged(mode);
    }
}

void SimulatorCameraControl::setState(QCamera::State state)
{
    if (mState == state) {
        return;
    }

    // Simulator only supports these status
    Q_ASSERT(mStatus == QCamera::UnloadedStatus || mStatus == QCamera::LoadedStatus
             || mStatus == QCamera::ActiveStatus);

    switch (state) {
    case QCamera::UnloadedState: // To UnloadedState - Release resources
        switch (mStatus) {
        case QCamera::LoadedStatus:
            // Unload
            break;
        case QCamera::ActiveStatus:
            // Stop and Unload
            emit stopCamera();
            break;
        default:
            // Unrecognized internal state (Status)
            return;
        }
        mStatus = QCamera::UnloadedStatus;
        emit statusChanged(mStatus);
        break;

    case QCamera::LoadedState: // To LoadedState - Reserve resources OR Stop ViewFinder and Cancel Capture
        switch (mStatus) {
        case QCamera::UnloadedStatus:
            // Load
            mStatus = QCamera::LoadingStatus;
            emit statusChanged(mStatus);
            break;
        case QCamera::ActiveStatus:
            // Stop
            emit stopCamera();
            break;

        default:
            // Unregocnized internal state (Status)
            return;
        }
        mStatus = QCamera::LoadedStatus;
        emit statusChanged(mStatus);
        break;

    case QCamera::ActiveState: // To ActiveState - (Reserve Resources and) Start ViewFinder
        switch (mStatus) {
        case QCamera::UnloadedStatus:
            // Load and Start (setting state handles starting)
            mStatus = QCamera::LoadingStatus;
            emit statusChanged(mStatus);
            mStatus = QCamera::LoadedStatus;
            emit statusChanged(mStatus);
            mStatus = QCamera::StartingStatus;
            emit statusChanged(mStatus);
            emit startCamera();
            break;
        case QCamera::LoadedStatus:
            // Start
            mStatus = QCamera::StartingStatus;
            emit statusChanged(mStatus);
            emit startCamera();
            break;
        default:
            // Unregocnized internal state (Status)
            return;
        }
        mStatus = QCamera::ActiveStatus;
        emit statusChanged(mStatus);
        break;

    default:
        return;
    }

    mState = state;
    emit stateChanged(mState);
}

QCamera::State SimulatorCameraControl::state() const
{
    return mState;
}

bool SimulatorCameraControl::canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const
{
    Q_UNUSED(status);

    switch (changeType) {
    case QCameraControl::CaptureMode:
    case QCameraControl::Viewfinder:
        return true;
    default:
        return false;
    }
}

bool SimulatorCameraControl::isCaptureModeSupported(QCamera::CaptureModes mode) const
{
    return mode == QCamera::CaptureStillImage;
}

QCamera::Status SimulatorCameraControl::status() const
{
    return mStatus;
}
