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

#include <QtCore/QDebug>

#include "dsimagecapturecontrol.h"

QT_BEGIN_NAMESPACE

DSImageCaptureControl::DSImageCaptureControl(DSCameraSession *session)
    : QCameraImageCaptureControl(session)
    , m_session(session)
{
    connect(m_session, &DSCameraSession::imageExposed,
            this, &DSImageCaptureControl::imageExposed);
    connect(m_session, &DSCameraSession::imageCaptured,
            this, &DSImageCaptureControl::imageCaptured);
    connect(m_session, &DSCameraSession::imageSaved,
            this, &DSImageCaptureControl::imageSaved);
    connect(m_session, &DSCameraSession::readyForCaptureChanged,
            this, &DSImageCaptureControl::readyForCaptureChanged);
    connect(m_session, &DSCameraSession::captureError,
            this, &DSImageCaptureControl::error);
    connect(m_session, &DSCameraSession::imageAvailable,
            this, &DSImageCaptureControl::imageAvailable);
}

DSImageCaptureControl::~DSImageCaptureControl() = default;

bool DSImageCaptureControl::isReadyForCapture() const
{
    return m_session->isReadyForCapture();
}

int DSImageCaptureControl::capture(const QString &fileName)
{
   return m_session->captureImage(fileName);
}

QCameraImageCapture::DriveMode DSImageCaptureControl::driveMode() const
{
    return QCameraImageCapture::SingleImageCapture;
}

void DSImageCaptureControl::setDriveMode(QCameraImageCapture::DriveMode mode)
{
    if (mode != QCameraImageCapture::SingleImageCapture)
        qWarning("Drive mode not supported.");
}

QT_END_NAMESPACE

