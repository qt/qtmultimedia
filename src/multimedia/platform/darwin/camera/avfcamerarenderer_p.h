/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef AVFCAMERARENDERER_H
#define AVFCAMERARENDERER_H

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

#include <QtCore/qobject.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtCore/qmutex.h>
#include <private/avfvideosink_p.h>
#include <private/qvideooutputorientationhandler_p.h>

#include <CoreVideo/CVBase.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <CoreVideo/CVImageBuffer.h>
#ifdef Q_OS_IOS
#include <CoreVideo/CVOpenGLESTexture.h>
#include <CoreVideo/CVOpenGLESTextureCache.h>
#endif

#include <dispatch/dispatch.h>

Q_FORWARD_DECLARE_OBJC_CLASS(AVFCaptureFramesDelegate);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureVideoDataOutput);

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;
class AVFCameraRenderer;
class AVFVideoSink;

class AVFCameraRenderer : public QObject, public AVFVideoSinkInterface
{
Q_OBJECT
public:
    AVFCameraRenderer(QObject *parent = nullptr);
    ~AVFCameraRenderer();

    void reconfigure() override;
    void setOutputSettings(NSDictionary *settings) override;

    void configureAVCaptureSession(AVFCameraSession *cameraSession);
    void syncHandleViewfinderFrame(const QVideoFrame &frame);

    AVCaptureVideoDataOutput *videoDataOutput() const;

    AVFCaptureFramesDelegate *captureDelegate() const;
    void resetCaptureDelegate() const;

    void setPixelFormat(const QVideoFrameFormat::PixelFormat format);

Q_SIGNALS:
    void newViewfinderFrame(const QVideoFrame &frame);

private Q_SLOTS:
    void handleViewfinderFrame();
    void updateCaptureConnection();
public Q_SLOTS:
    void deviceOrientationChanged(int angle = -1);

private:
    AVFCaptureFramesDelegate *m_viewfinderFramesDelegate = nullptr;
    AVFCameraSession *m_cameraSession = nullptr;
    AVCaptureVideoDataOutput *m_videoDataOutput = nullptr;

    bool m_needsHorizontalMirroring = false;

#ifdef Q_OS_IOS
    CVOpenGLESTextureCacheRef m_textureCache = nullptr;
#endif

    QVideoFrame m_lastViewfinderFrame;
    QMutex m_vfMutex;
    dispatch_queue_t m_delegateQueue;
    QVideoOutputOrientationHandler m_orientationHandler;

    friend class CVImageVideoBuffer;
};

QT_END_NAMESPACE

#endif
