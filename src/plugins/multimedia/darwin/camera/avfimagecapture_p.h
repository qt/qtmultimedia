// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFCAMERAIMAGECAPTURE_H
#define AVFCAMERAIMAGECAPTURE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#import <AVFoundation/AVFoundation.h>

#include <QtCore/qqueue.h>
#include <QtCore/qsemaphore.h>
#include <QtCore/qsharedpointer.h>
#include <private/qplatformimagecapture_p.h>
#include "avfcamerasession_p.h"

QT_BEGIN_NAMESPACE

class AVFImageCapture : public QPlatformImageCapture
{
Q_OBJECT
public:
    struct CaptureRequest {
        int captureId;
        QSharedPointer<QSemaphore> previewReady;
    };

    AVFImageCapture(QImageCapture *parent = nullptr);
    ~AVFImageCapture() override;

    bool isReadyForCapture() const override;

    AVCaptureStillImageOutput *stillImageOutput() const {return m_stillImageOutput;}

    int doCapture(const QString &fileName);
    int capture(const QString &fileName) override;
    int captureToBuffer() override;

    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;
    bool applySettings();

    void setCaptureSession(QPlatformMediaCaptureSession *session);

private Q_SLOTS:
    void updateCaptureConnection();
    void updateReadyStatus();
    void onNewViewfinderFrame(const QVideoFrame &frame);
    void onCameraChanged();

private:
    void makeCapturePreview(CaptureRequest request, const QVideoFrame &frame, int rotation);
    bool videoCaptureDeviceIsValid() const;

    AVFCameraService *m_service = nullptr;
    AVFCameraSession *m_session = nullptr;
    AVFCamera *m_cameraControl = nullptr;
    bool m_ready = false;
    int m_lastCaptureId = 0;
    AVCaptureStillImageOutput *m_stillImageOutput;
    AVCaptureConnection *m_videoConnection = nullptr;

    QMutex m_requestsMutex;
    QQueue<CaptureRequest> m_captureRequests;
    QImageEncoderSettings m_settings;
};

Q_DECLARE_TYPEINFO(AVFImageCapture::CaptureRequest, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif
