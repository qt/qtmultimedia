// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFVIDEOWINDOWCONTROL_H
#define AVFVIDEOWINDOWCONTROL_H

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

#include "private/qplatformvideosink_p.h"

#include <QtCore/private/qcore_mac_p.h>

Q_FORWARD_DECLARE_OBJC_CLASS(CALayer);
Q_FORWARD_DECLARE_OBJC_CLASS(AVPlayerLayer);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureVideoPreviewLayer);

#include <CoreVideo/CVBase.h>
#include <CoreVideo/CVImageBuffer.h>
#include <CoreVideo/CVPixelBuffer.h>

#import "Metal/Metal.h"
#import "MetalKit/MetalKit.h"

QT_BEGIN_NAMESPACE

class AVFVideoSinkInterface;

class AVFVideoSink : public QPlatformVideoSink
{
    Q_OBJECT

public:
    explicit AVFVideoSink(QVideoSink *parent = nullptr);
    ~AVFVideoSink() override;

    // QPlatformVideoSink interface
public:
    void setRhi(QRhi *rhi) override;

    void setNativeSize(QSize size);

    void setVideoSinkInterface(AVFVideoSinkInterface *interface);

private:
    AVFVideoSinkInterface *m_interface = nullptr;
    QRhi *m_rhi = nullptr;
};

class AVFVideoSinkInterface
{
public:
    ~AVFVideoSinkInterface();

    void setVideoSink(AVFVideoSink *sink);


    virtual void reconfigure() = 0;
    virtual void setRhi(QRhi *);
    virtual void setLayer(CALayer *layer);
    virtual void setOutputSettings();

    QRhi *rhi() const { return m_rhi; }

    void updateLayerBounds();
    void nativeSizeChanged() { updateLayerBounds(); }
    QSize nativeSize() const { return m_sink ? m_sink->nativeSize() : QSize(); }

    QCFType<CVMetalTextureCacheRef> cvMetalTextureCache;
#if defined(Q_OS_MACOS)
    QCFType<CVOpenGLTextureCacheRef> cvOpenGLTextureCache;
#elif defined(Q_OS_IOS)
    QCFType<CVOpenGLESTextureCacheRef> cvOpenGLESTextureCache;
#endif
private:
    void freeTextureCaches();

protected:

    AVFVideoSink *m_sink = nullptr;
    QRhi *m_rhi = nullptr;
    CALayer *m_layer = nullptr;
    NSDictionary *m_outputSettings = nullptr;
};


QT_END_NAMESPACE

#endif // AVFVIDEOWINDOWCONTROL_H
