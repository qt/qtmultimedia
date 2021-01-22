/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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

// Camera snippets
// Extracted from src/multimedia/doc/snippets/multimedia-snippets/camera.cpp
#include "qcamera.h"
#include "qcameraviewfinder.h"
#include "qcameraimagecapture.h"

/* Globals so that everything is consistent. */
QCamera *camera = 0;
QCameraViewfinder *viewfinder = 0;
QCameraImageCapture *imageCapture = 0;

void camera_blah()
{
    //! [Camera]
    camera = new QCamera;

    viewfinder = new QCameraViewfinder();
    viewfinder->show();

    camera->setViewfinder(viewfinder);

    imageCapture = new QCameraImageCapture(camera);

    camera->setCaptureMode(QCamera::CaptureStillImage);
    camera->start();
    //! [Camera]
}
