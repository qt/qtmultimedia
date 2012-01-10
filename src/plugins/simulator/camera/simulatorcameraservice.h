/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef SIMULATORCAMERACAPTURESERVICE_H
#define SIMULATORCAMERACAPTURESERVICE_H

#include <qmediaservice.h>

#include "../qsimulatormultimediaconnection_p.h"

QTM_BEGIN_NAMESPACE
namespace Simulator {
class MultimediaConnection;
}
QTM_END_NAMESPACE
class SimulatorCameraSession;
class SimulatorCameraControl;
class SimulatorCameraImageCaptureControl;
class SimulatorCameraExposureControl;
class SimulatorVideoRendererControl;
class QSimulatorVideoInputDeviceControl;

class SimulatorCameraService : public QMediaService
{
    Q_OBJECT

public:
    SimulatorCameraService(const QString &service, QTM_PREPEND_NAMESPACE(Simulator::MultimediaConnection) *cameraConnection,
                  QObject *parent = 0);
    virtual ~SimulatorCameraService();

    QMediaControl *requestControl(const char *name);
    void releaseControl(QMediaControl *);

private slots:
    void updateCameraData(const QtMobility::QCameraData &data);
    void changeCamera(const QString &name, const QtMobility::QCameraData::QCameraDetails &details);
    void updateCameraPicture(const QString &name);

private:
    void updateCurrentDeviceImage(const QString &imagePath);
    SimulatorCameraSession *mCaptureSession;
    SimulatorCameraControl *mCameraControl;
    SimulatorCameraImageCaptureControl *mImageCaptureControl;
    SimulatorCameraExposureControl *mExposureControl;

    QSimulatorVideoInputDeviceControl *mVideoInputDeviceControl;

    QMediaControl *mVideoOutput;

    SimulatorVideoRendererControl *mVideoRendererControl;
};

#endif // CAMERACAPTURESERVICE_H
