// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCamera>
#include <QCameraDevice>
#include <QImageCapture>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QPermission>
#include <QVideoSink>
#include <QVideoWidget>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>

// Globals so that the snippet bodies can stay focused on the relevant API.
QCamera *camera = nullptr;
QImageCapture *imageCapture = nullptr;
QMediaRecorder *recorder = nullptr;
QVideoWidget *viewfinder = nullptr;

//! [Camera overview check]
bool checkCameraAvailability()
{
    return !QMediaDevices::videoInputs().isEmpty();
}
//! [Camera overview check]

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
QCameraDevice cameraDevice = myCamera.cameraDevice();

if (cameraDevice.position() == QCameraDevice::FrontFace)
    qDebug() << "The camera is on the front face of the hardware system.";
else if (cameraDevice.position() == QCameraDevice::BackFace)
    qDebug() << "The camera is on the back face of the hardware system.";
//! [Camera info]
}

void overview_camera_by_position()
{
//! [Camera overview position]
camera = new QCamera(QCameraDevice::FrontFace);
//! [Camera overview position]
}

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

void overview_surface()
{
QVideoSink *mySink = nullptr;
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

void overview_still()
{
//! [Camera overview capture]
QMediaCaptureSession captureSession;
camera = new QCamera;
captureSession.setCamera(camera);
imageCapture = new QImageCapture;
captureSession.setImageCapture(imageCapture);

camera->start(); // Viewfinder frames start flowing

// on shutter button pressed
imageCapture->capture();
//! [Camera overview capture]
}

void overview_movie()
{
//! [Camera overview movie]
QMediaCaptureSession captureSession;
camera = new QCamera;
captureSession.setCamera(camera);
recorder = new QMediaRecorder;
captureSession.setRecorder(recorder);

camera->start();

// setup output format for the recorder
QMediaFormat format(QMediaFormat::MPEG4);
format.setVideoCodec(QMediaFormat::VideoCodec::H264);
format.setAudioCodec(QMediaFormat::AudioCodec::MP3);
recorder->setMediaFormat(format);

// on shutter button pressed
recorder->record();

// sometime later, or on another press
recorder->stop();
//! [Camera overview movie]
}

void image_capture()
{
//! [Camera]
QMediaCaptureSession captureSession;
camera = new QCamera;
captureSession.setCamera(camera);

viewfinder = new QVideoWidget;
viewfinder->show();
captureSession.setVideoOutput(viewfinder);

imageCapture = new QImageCapture;
captureSession.setImageCapture(imageCapture);

camera->start();
//! [Camera]

//! [Camera keys]
// on shutter button pressed
imageCapture->capture();
//! [Camera keys]
}

void camerafocus()
{
//! [Camera zoom]
camera->setZoomFactor(camera->maximumZoomFactor()); // zoom in as much as possible
//! [Camera zoom]
}

void cameraimageprocessing()
{
//! [Camera image whitebalance]
camera->setWhiteBalanceMode(QCamera::WhiteBalanceManual);
camera->setColorTemperature(5600);
//! [Camera image whitebalance]
}

void camerapermission()
{
//! [Camera permission]
qApp->requestPermission(QCameraPermission{}, [](const QPermission &permission) {
    if (permission.status() == Qt::PermissionStatus::Granted)
        camera->setActive(true);
});
//! [Camera permission]
}
