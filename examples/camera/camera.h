/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CAMERA_H
#define CAMERA_H

#include <qcamera.h>
#include <qmediarecorder.h>
#include <qcameraimagecapture.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class Camera;
}
class QCameraViewfinder;
QT_END_NAMESPACE

#include <QMainWindow>
#include <QDir>

QT_USE_NAMESPACE

class Camera : public QMainWindow
{
    Q_OBJECT
public:
    Camera(QWidget *parent = 0);
    ~Camera();

private slots:
    void setCamera(const QByteArray &cameraDevice);

    void startCamera();
    void stopCamera();

    void record();
    void pause();
    void stop();
    void setMuted(bool);

    void toggleLock();
    void takeImage();

    void configureCaptureSettings();
    void configureVideoSettings();
    void configureImageSettings();

    void displayRecorderError();
    void displayCameraError();

    void updateCameraDevice(QAction*);

    void updateCameraState(QCamera::State);
    void updateCaptureMode();
    void updateRecorderState(QMediaRecorder::State state);
    void setExposureCompensation(int index);

    void updateRecordTime();

    void processCapturedImage(int requestId, const QImage& img);
    void updateLockStatus(QCamera::LockStatus, QCamera::LockChangeReason);

    void displayViewfinder();
    void displayCapturedImage();

    void readyForCapture(bool);
    void imageSaved(int, const QString&);

protected:
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent * event);
    void closeEvent(QCloseEvent *event );

private:
    Ui::Camera *ui;

    QCamera *camera;
    QCameraImageCapture *imageCapture;
    QMediaRecorder* mediaRecorder;

    QImageEncoderSettings imageSettings;
    QAudioEncoderSettings audioSettings;
    QVideoEncoderSettings videoSettings;
    QString videoContainerFormat;
    bool isCapturingImage;
    bool applicationExiting;
};

#endif
