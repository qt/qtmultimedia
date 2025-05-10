// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <avfvideosink_p.h>
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
    ~AVFCameraRenderer() override;

    void reconfigure() override;
    void setOutputSettings() override;

    void configureAVCaptureSession(AVFCameraSession *cameraSession);
    void syncHandleViewfinderFrame(const QVideoFrame &frame);

    AVCaptureVideoDataOutput *videoDataOutput() const;

    AVFCaptureFramesDelegate *captureDelegate() const;
    void resetCaptureDelegate() const;

    void setPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat,
                        QVideoFrameFormat::ColorRange colorRange);

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
