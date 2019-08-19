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

#ifndef AVFVIDEOFRAMERENDERER_H
#define AVFVIDEOFRAMERENDERER_H

#include <QtCore/QObject>
#include <QtGui/QImage>
#include <QtGui/QOpenGLContext>
#include <QtCore/QSize>

#import "Metal/Metal.h"
#import "MetalKit/MetalKit.h"

@class CARenderer;
@class AVPlayerLayer;

QT_BEGIN_NAMESPACE

class QOpenGLFramebufferObject;
class QOpenGLShaderProgram;
class QWindow;
class QOpenGLContext;
class QAbstractVideoSurface;

class AVFVideoFrameRenderer : public QObject
{
public:
    AVFVideoFrameRenderer(QAbstractVideoSurface *surface, QObject *parent = nullptr);

    virtual ~AVFVideoFrameRenderer();

    GLuint renderLayerToTexture(AVPlayerLayer *layer);
    QImage renderLayerToImage(AVPlayerLayer *layer);

    static GLuint createGLTexture(CGLContextObj cglContextObj, CGLPixelFormatObj cglPixelFormtObj,
                                  CVOpenGLTextureCacheRef cvglTextureCache,
                                  CVPixelBufferRef cvPixelBufferRef,
                                  CVOpenGLTextureRef cvOpenGLTextureRef);

    static id<MTLTexture> createMetalTexture(id<MTLDevice> mtlDevice,
                                             CVMetalTextureCacheRef cvMetalTextureCacheRef,
                                             CVPixelBufferRef cvPixelBufferRef,
                                             MTLPixelFormat pixelFormat, size_t width, size_t height,
                                             CVMetalTextureRef cvMetalTextureRef);

private:
    QOpenGLFramebufferObject* initRenderer(AVPlayerLayer *layer);
    void renderLayerToFBO(AVPlayerLayer *layer, QOpenGLFramebufferObject *fbo);
    void renderLayerToFBOCoreOpenGL(AVPlayerLayer *layer, QOpenGLFramebufferObject *fbo);

    CARenderer *m_videoLayerRenderer;
    QAbstractVideoSurface *m_surface;
    QOpenGLFramebufferObject *m_fbo[2];
    QOpenGLShaderProgram *m_shader = nullptr;
    QWindow *m_offscreenSurface;
    QOpenGLContext *m_glContext;
    QSize m_targetSize;

    bool m_useCoreProfile = false;

    // Shared pixel buffer
    CVPixelBufferRef m_CVPixelBuffer;

    // OpenGL Texture
    CVOpenGLTextureCacheRef m_CVGLTextureCache;
    CVOpenGLTextureRef m_CVGLTexture;
    CGLPixelFormatObj m_CGLPixelFormat;
    GLuint m_textureName = 0;

    // Metal Texture
    CVMetalTextureRef m_CVMTLTexture;
    CVMetalTextureCacheRef m_CVMTLTextureCache;
    id<MTLDevice> m_metalDevice = nil;
    id<MTLTexture> m_metalTexture = nil;

    NSOpenGLContext *m_NSGLContext = nullptr;

    GLuint m_quadVao = 0;
    GLuint m_quadVbos[2];

    uint m_currentBuffer;
    bool m_isContextShared;
};

QT_END_NAMESPACE

#endif // AVFVIDEOFRAMERENDERER_H
