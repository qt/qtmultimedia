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

#ifndef AVFCAMERARENDERERCONTROL_H
#define AVFCAMERARENDERERCONTROL_H

#include <QtMultimedia/qvideorenderercontrol.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtCore/qmutex.h>

#import <AVFoundation/AVFoundation.h>

@class AVFCaptureFramesDelegate;

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;
class AVFCameraRendererControl;

class AVFCameraRendererControl : public QVideoRendererControl
{
Q_OBJECT
public:
    AVFCameraRendererControl(QObject *parent = nullptr);
    ~AVFCameraRendererControl();

    QAbstractVideoSurface *surface() const override;
    void setSurface(QAbstractVideoSurface *surface) override;

    void configureAVCaptureSession(AVFCameraSession *cameraSession);
    void syncHandleViewfinderFrame(const QVideoFrame &frame);

    AVCaptureVideoDataOutput *videoDataOutput() const;

    bool supportsTextures() const { return m_supportsTextures; }

#ifdef Q_OS_IOS
    AVFCaptureFramesDelegate *captureDelegate() const;
    void resetCaptureDelegate() const;
#endif

Q_SIGNALS:
    void surfaceChanged(QAbstractVideoSurface *surface);

private Q_SLOTS:
    void handleViewfinderFrame();
    void updateCaptureConnection();

private:
    QAbstractVideoSurface *m_surface;
    AVFCaptureFramesDelegate *m_viewfinderFramesDelegate;
    AVFCameraSession *m_cameraSession;
    AVCaptureVideoDataOutput *m_videoDataOutput;

    bool m_supportsTextures;
    bool m_needsHorizontalMirroring;

#ifdef Q_OS_IOS
    CVOpenGLESTextureCacheRef m_textureCache;
#endif

    QVideoFrame m_lastViewfinderFrame;
    QMutex m_vfMutex;
    dispatch_queue_t m_delegateQueue;

    friend class CVImageVideoBuffer;
};

QT_END_NAMESPACE

#endif
