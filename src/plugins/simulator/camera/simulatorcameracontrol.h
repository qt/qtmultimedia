/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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


#ifndef SIMULATORCAMERACONTROL_H
#define SIMULATORCAMERACONTROL_H

#include <QHash>
#include <qcameracontrol.h>
#include "simulatorcamerasession.h"

QT_USE_NAMESPACE
QT_USE_NAMESPACE

class SimulatorCameraControl : public QCameraControl
{
    Q_OBJECT
public:
    SimulatorCameraControl(SimulatorCameraSession *session );
    virtual ~SimulatorCameraControl();

    bool isValid() const { return true; }

    QCamera::State state() const;
    void setState(QCamera::State state);

    QCamera::Status status() const;

    QCamera::CaptureMode captureMode() const;
    void setCaptureMode(QCamera::CaptureMode mode);

    bool isCaptureModeSupported(QCamera::CaptureMode mode) const;

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const;

signals:
    void startCamera();
    void stopCamera();

private:
    void updateSupportedResolutions(const QString &device);

    SimulatorCameraSession *m_session;
    QCamera::State mState;
    QCamera::Status mStatus;
    bool m_reloadPending;
};

#endif // CAMERACONTROL_H
