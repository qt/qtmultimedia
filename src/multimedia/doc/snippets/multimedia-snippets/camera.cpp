// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/* Camera snippets */

#include "qcamera.h"
#include "qcameradevice.h"
#include "qmediarecorder.h"
#include "qmediadevices.h"
#include "qmediacapturesession.h"
#include "qimagecapture.h"
#include "qvideosink.h"
#include <QtMultimediaWidgets/qvideowidget.h>
#include <QtGui/qscreen.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qimage.h>

/* Globals so that everything is consistent. */
QCamera *camera = 0;
QMediaRecorder *recorder = 0;
QImageCapture *imageCapture = 0;
QVideoWidget *viewfinder = 0;

//! [Camera overview check]
bool checkCameraAvailability()
{
    if (QMediaDevices::videoInputs().count() > 0)
        return true;
    else
        return false;
}
//! [Camera overview check]

void overview_viewfinder()
{
    //! [Camera overview viewfinder]
    QMediaCaptureSession captureSession;
    camera = new QCamera;
    captureSession.setCamera(camera);
    viewfinder = new QVideoWidget;
    captureSession.setVideoOutput(viewfinder);
    viewfinder->show();

    camera->start(); // to start the camera
    //! [Camera overview viewfinder]
}

void overview_camera_by_position()
{
    //! [Camera overview position]
    camera = new QCamera(QCameraDevice::FrontFace);
    //! [Camera overview position]
}

// -.-
void overview_surface()
{
    QVideoSink *mySink;
    //! [Camera overview surface]
    QMediaCaptureSession captureSession;
    camera = new QCamera;
    captureSession.setCamera(camera);
    mySink = new QVideoSink;
    captureSession.setVideoOutput(mySink);

    camera->start();
    // MyVideoSink::setVideoFrame(..) will be called with video frames
    //! [Camera overview surface]
}

void overview_viewfinder_orientation()
{
    QCamera camera;

    //! [Camera overview viewfinder orientation]
    // Assuming a QImage has been created from the QVideoFrame that needs to be presented
    QImage videoFrame;
    QCameraDevice cameraDevice(camera); // needed to get the camera sensor position and orientation

    // Get the current display orientation
    const QScreen *screen = QGuiApplication::primaryScreen();
    const int screenAngle = screen->angleBetween(screen->nativeOrientation(), screen->orientation());

    int rotation;
    if (cameraDevice.position() == QCameraDevice::BackFace) {
        rotation = (cameraDevice.orientation() - screenAngle) % 360;
    } else {
        // Front position, compensate the mirror
        rotation = (360 - cameraDevice.orientation() + screenAngle) % 360;
    }

    // Rotate the frame so it always shows in the correct orientation
    videoFrame = videoFrame.transformed(QTransform().rotate(rotation));
    //! [Camera overview viewfinder orientation]
}

void overview_still()
{
    //! [Camera overview capture]
    QMediaCaptureSession captureSession;
    camera = new QCamera;
    captureSession.setCamera(camera);
    imageCapture = new QImageCapture;
    captureSession.setImageCapture(imageCapture);

    camera->start(); // Viewfinder frames start flowing

    //on shutter button pressed
    imageCapture->capture();
    //! [Camera overview capture]
}

void overview_movie()
{
    //! [Camera overview movie]
    QMediaCaptureSession captureSession;
    camera = new QCamera;
    captureSession.setCamera(camera);
    recorder = new QMediaRecorder(camera);
    captureSession.setRecorder(recorder);

    camera->start();

    // setup output format for the recorder
    QMediaFormat format(QMediaFormat::MPEG4);
    format.setVideoCodec(QMediaRecorder::VideoCodec::H264);
    format.setAudioCodec(QMediaRecorder::AudioCodec::MP3);
    recorder->setMediaFormat(settings);

    //on shutter button pressed
    recorder->record();

    // sometime later, or on another press
    recorder->stop();
    //! [Camera overview movie]
}

void camera_listing()
{
    //! [Camera listing]
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &cameraDevice : cameras)
        qDebug() << cameraDevice.description();
    //! [Camera listing]
}

void camera_selection()
{
    //! [Camera selection]
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &cameraDevice : cameras) {
        if (cameraDevice.description() == "mycamera")
            camera = new QCamera(cameraDevice);
    }
    //! [Camera selection]
}

void camera_info()
{
    //! [Camera info]
    QCamera myCamera;
    QCameraDevice cameraDevice = camera->cameraDevice();

    if (cameraDevice.position() == QCameraDevice::FrontFace)
        qDebug() << "The camera is on the front face of the hardware system.";
    else if (cameraDevice.position() == QCameraDevice::BackFace)
        qDebug() << "The camera is on the back face of the hardware system.";
    //! [Camera info]
}

void camera_blah()
{
    //! [Camera]
    QMediaCaptureSession captureSession;
    camera = new QCamera;
    captureSession.setCamera(camera);

    viewfinder = new QVideoWidget();
    viewfinder->show();
    captureSession.setVideoOutput(viewfinder);

    imageCapture = new QImageCapture(camera);
    captureSession.setImageCapture(imageCapture);

    camera->start();
    //! [Camera]

    //! [Camera keys]
    //on shutter button pressed
    imageCapture->capture();
    //! [Camera keys]
}

void cameraimageprocessing()
{
    camera = new QCamera;
    //! [Camera image whitebalance]
    camera->setWhiteBalanceMode(QCamera::WhiteBalanceFluorescent);
    //! [Camera image whitebalance]
}

void camerafocus()
{
    //! [Camera custom focus]
    camera->setFocusPointMode(QCamera::FocusModeManual);
    camera->setCustomFocusPoint(QPointF(0.25f, 0.75f)); // A point near the bottom left, 25% away from the corner, near that shiny vase
    //! [Camera custom focus]

    //! [Camera zoom]
    camera->setZoomFactor(3.0);
    //! [Camera zoom]
}
