/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qstring.h>

#include "s60cameraimagecapturecontrol.h"
#include "s60cameraservice.h"
#include "s60imagecapturesession.h"
#include "s60cameracontrol.h"

S60CameraImageCaptureControl::S60CameraImageCaptureControl(QObject *parent) :
    QCameraImageCaptureControl(parent)
{
}

S60CameraImageCaptureControl::S60CameraImageCaptureControl(S60CameraService *service,
                                                           S60ImageCaptureSession *session,
                                                           QObject *parent) :
    QCameraImageCaptureControl(parent),
    m_driveMode(QCameraImageCapture::SingleImageCapture) // Default DriveMode
{
    m_session = session;
    m_service = service;
    m_cameraControl = qobject_cast<S60CameraControl *>(m_service->requestControl(QCameraControl_iid));

    if (!m_cameraControl)
        m_session->setError(KErrGeneral, tr("Unexpected camera error."));

    // Chain these signals from session class
    connect(m_session, SIGNAL(imageCaptured(const int, QImage)),
        this, SIGNAL(imageCaptured(const int, QImage)));
    connect(m_session, SIGNAL(readyForCaptureChanged(bool)),
            this, SIGNAL(readyForCaptureChanged(bool)), Qt::QueuedConnection);
    connect(m_session, SIGNAL(imageSaved(const int, const QString&)),
            this, SIGNAL(imageSaved(const int, const QString&)));
    connect(m_session, SIGNAL(imageExposed(int)),
            this, SIGNAL(imageExposed(int)));
    connect(m_session, SIGNAL(captureError(int, int, const QString&)),
            this, SIGNAL(error(int, int, const QString&)));
}

S60CameraImageCaptureControl::~S60CameraImageCaptureControl()
{
}

bool S60CameraImageCaptureControl::isReadyForCapture() const
{
    if (m_cameraControl && m_cameraControl->captureMode() != QCamera::CaptureStillImage)
        return false;

    return m_session->isDeviceReady();
}

QCameraImageCapture::DriveMode S60CameraImageCaptureControl::driveMode() const
{
    return m_driveMode;
}

void S60CameraImageCaptureControl::setDriveMode(QCameraImageCapture::DriveMode mode)
{
    if (mode != QCameraImageCapture::SingleImageCapture) {
        emit error((m_session->currentImageId() + 1), QCamera::NotSupportedFeatureError, tr("DriveMode not supported."));
        return;
    }

    m_driveMode = mode;
}

int S60CameraImageCaptureControl::capture(const QString &fileName)
{
    if (m_cameraControl && m_cameraControl->captureMode() != QCamera::CaptureStillImage) {
        emit error((m_session->currentImageId() + 1), QCameraImageCapture::NotReadyError, tr("Incorrect CaptureMode."));
        return 0;
    }

    int imageId = m_session->capture(fileName);

    return imageId;
}

void S60CameraImageCaptureControl::cancelCapture()
{
    m_session->cancelCapture();
}

// End of file
