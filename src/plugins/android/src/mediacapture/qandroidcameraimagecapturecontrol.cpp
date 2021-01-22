/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qandroidcameraimagecapturecontrol.h"

#include "qandroidcamerasession.h"

QT_BEGIN_NAMESPACE

QAndroidCameraImageCaptureControl::QAndroidCameraImageCaptureControl(QAndroidCameraSession *session)
    : QCameraImageCaptureControl()
    , m_session(session)
{
    connect(m_session, SIGNAL(readyForCaptureChanged(bool)), this, SIGNAL(readyForCaptureChanged(bool)));
    connect(m_session, SIGNAL(imageExposed(int)), this, SIGNAL(imageExposed(int)));
    connect(m_session, SIGNAL(imageCaptured(int,QImage)), this, SIGNAL(imageCaptured(int,QImage)));
    connect(m_session, SIGNAL(imageMetadataAvailable(int,QString,QVariant)), this, SIGNAL(imageMetadataAvailable(int,QString,QVariant)));
    connect(m_session, SIGNAL(imageAvailable(int,QVideoFrame)), this, SIGNAL(imageAvailable(int,QVideoFrame)));
    connect(m_session, SIGNAL(imageSaved(int,QString)), this, SIGNAL(imageSaved(int,QString)));
    connect(m_session, SIGNAL(imageCaptureError(int,int,QString)), this, SIGNAL(error(int,int,QString)));
}

bool QAndroidCameraImageCaptureControl::isReadyForCapture() const
{
    return m_session->isReadyForCapture();
}

QCameraImageCapture::DriveMode QAndroidCameraImageCaptureControl::driveMode() const
{
    return m_session->driveMode();
}

void QAndroidCameraImageCaptureControl::setDriveMode(QCameraImageCapture::DriveMode mode)
{
    m_session->setDriveMode(mode);
}

int QAndroidCameraImageCaptureControl::capture(const QString &fileName)
{
    return m_session->capture(fileName);
}

void QAndroidCameraImageCaptureControl::cancelCapture()
{
    m_session->cancelCapture();
}

QT_END_NAMESPACE
