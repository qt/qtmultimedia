/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
