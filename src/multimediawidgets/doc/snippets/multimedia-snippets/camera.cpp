// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Camera snippets
// Extracted from src/multimedia/doc/snippets/multimedia-snippets/camera.cpp
#include "qcamera.h"
#include "qvideowidget.h"
#include "qimagecapture.h"

/* Globals so that everything is consistent. */
QCamera *camera = 0;
QImageCapture *imageCapture = 0;
QVideoWidget *viewfinder = 0;

void camera_blah()
{
    //! [Camera]
    QMediaCaptureSession captureSession;
    camera = new QCamera;
    captureSession.setCamera(camera);

    viewfinder = new QVideoWidget();
    viewfinder->show();
    captureSession.setVideoOutput(viewfinder);

    imageCapture = new QImageCapture;
    captureSession.setImageCapture(imageCapture);

    camera->start();
    //! [Camera]

    //! [Camera keys]
    //on shutter button pressed
    imageCapture->capture();
    //! [Camera keys]
}

