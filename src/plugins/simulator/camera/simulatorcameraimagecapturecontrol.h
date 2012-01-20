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

#ifndef SIMULATORCAMERAIMAGECAPTURECONTROL_H
#define SIMULATORCAMERAIMAGECAPTURECONTROL_H

#include "qcameraimagecapturecontrol.h"

QT_USE_NAMESPACE

class SimulatorCameraService;
class SimulatorCameraSession;
class SimulatorCameraControl;

/*
 * Control for image capture operations.
 */
class SimulatorCameraImageCaptureControl : public QCameraImageCaptureControl
{
    Q_OBJECT

public: // Contructors & Destrcutor

    SimulatorCameraImageCaptureControl(SimulatorCameraSession *session, SimulatorCameraService *service);
    ~SimulatorCameraImageCaptureControl();

public: // QCameraImageCaptureControl

    bool isReadyForCapture() const;

    // Drive Mode
    QCameraImageCapture::DriveMode driveMode() const;
    void setDriveMode(QCameraImageCapture::DriveMode mode);

    // Capture
    int capture(const QString &fileName);
    void cancelCapture();

private:
    void updateReadyForCapture(bool ready);

    bool mReadyForCapture;
    SimulatorCameraSession *m_session;
    SimulatorCameraService *m_service;
    SimulatorCameraControl *m_cameraControl;
    QCameraImageCapture::DriveMode m_driveMode;
};

#endif // SIMULATORCAMERAIMAGECAPTURECONTROL_H
