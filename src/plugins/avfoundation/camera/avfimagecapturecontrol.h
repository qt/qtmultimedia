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

#ifndef AVFIMAGECAPTURECONTROL_H
#define AVFIMAGECAPTURECONTROL_H

#import <AVFoundation/AVFoundation.h>

#include <QtCore/qqueue.h>
#include <QtCore/qsemaphore.h>
#include <QtCore/qsharedpointer.h>
#include <QtMultimedia/qcameraimagecapturecontrol.h>
#include "avfcamerasession.h"
#include "avfstoragelocation.h"

QT_BEGIN_NAMESPACE

class AVFImageCaptureControl : public QCameraImageCaptureControl
{
Q_OBJECT
public:
    struct CaptureRequest {
        int captureId;
        QSharedPointer<QSemaphore> previewReady;
    };

    AVFImageCaptureControl(AVFCameraService *service, QObject *parent = nullptr);
    ~AVFImageCaptureControl();

    bool isReadyForCapture() const override;

    QCameraImageCapture::DriveMode driveMode() const override { return QCameraImageCapture::SingleImageCapture; }
    void setDriveMode(QCameraImageCapture::DriveMode ) override {}
    AVCaptureStillImageOutput *stillImageOutput() const {return m_stillImageOutput;}

    int capture(const QString &fileName) override;
    void cancelCapture() override;

private Q_SLOTS:
    void updateCaptureConnection();
    void updateReadyStatus();
    void onNewViewfinderFrame(const QVideoFrame &frame);

private:
    void makeCapturePreview(CaptureRequest request, const QVideoFrame &frame, int rotation);

    AVFCameraService *m_service;
    AVFCameraSession *m_session;
    AVFCameraControl *m_cameraControl;
    bool m_ready;
    int m_lastCaptureId;
    AVCaptureStillImageOutput *m_stillImageOutput;
    AVCaptureConnection *m_videoConnection;
    AVFStorageLocation m_storageLocation;

    QMutex m_requestsMutex;
    QQueue<CaptureRequest> m_captureRequests;
};

Q_DECLARE_TYPEINFO(AVFImageCaptureControl::CaptureRequest, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif
