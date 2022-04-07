/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfvideoframerenderer.h"

#include <QtMultimedia/qabstractvideosurface.h>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QWindow>
#include <QOpenGLShaderProgram>
#include <QtPlatformHeaders/QCocoaNativeContext>

#ifdef QT_DEBUG_AVF
#include <QtCore/qdebug.h>
#endif

#import <CoreVideo/CVBase.h>
#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

AVFVideoFrameRenderer::AVFVideoFrameRenderer(QAbstractVideoSurface *surface, QObject *parent)
    : QObject(parent)
    , m_videoLayerRenderer(nullptr)
    , m_surface(surface)
    , m_offscreenSurface(nullptr)
    , m_glContext(nullptr)
    , m_currentBuffer(1)
    , m_isContextShared(true)
{
    m_fbo[0] = nullptr;
    m_fbo[1] = nullptr;
}

AVFVideoFrameRenderer::~AVFVideoFrameRenderer()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif

    [m_videoLayerRenderer release];
    delete m_fbo[0];
    delete m_fbo[1];
    delete m_offscreenSurface;
    delete m_glContext;

    if (m_useCoreProfile) {
        glDeleteVertexArrays(1, &m_quadVao);
        glDeleteBuffers(2, m_quadVbos);
        delete m_shader;
    }
}

GLuint AVFVideoFrameRenderer::renderLayerToTexture(AVPlayerLayer *layer)
{
    //Is layer valid
    if (!layer)
        return 0;

    //If the glContext isn't shared, it doesn't make sense to return a texture for us
    if (m_offscreenSurface && !m_isContextShared)
        return 0;

    QOpenGLFramebufferObject *fbo = initRenderer(layer);

    if (!fbo)
        return 0;

    renderLayerToFBO(layer, fbo);
    if (m_glContext)
        m_glContext->doneCurrent();

    return fbo->texture();
}

QImage AVFVideoFrameRenderer::renderLayerToImage(AVPlayerLayer *layer)
{
    //Is layer valid
    if (!layer) {
        return QImage();
    }

    QOpenGLFramebufferObject *fbo = initRenderer(layer);

    if (!fbo)
        return QImage();

    renderLayerToFBO(layer, fbo);
    QImage fboImage = fbo->toImage();
    if (m_glContext)
        m_glContext->doneCurrent();

    return fboImage;
}

QOpenGLFramebufferObject *AVFVideoFrameRenderer::initRenderer(AVPlayerLayer *layer)
{

    //Get size from AVPlayerLayer
    m_targetSize = QSize(layer.bounds.size.width, layer.bounds.size.height);

    QOpenGLContext *shareContext = !m_glContext && m_surface
        ? qobject_cast<QOpenGLContext*>(m_surface->property("GLContext").value<QObject*>())
        : nullptr;

    //Make sure we have an OpenGL context to make current
    if ((shareContext && shareContext != QOpenGLContext::currentContext())
        || (!QOpenGLContext::currentContext() && !m_glContext)) {

        //Create Hidden QWindow surface to create context in this thread
        delete m_offscreenSurface;
        m_offscreenSurface = new QWindow();
        m_offscreenSurface->setSurfaceType(QWindow::OpenGLSurface);
        //Needs geometry to be a valid surface, but size is not important
        m_offscreenSurface->setGeometry(0, 0, 1, 1);
        m_offscreenSurface->create();

        delete m_glContext;
        m_glContext = new QOpenGLContext();
        m_glContext->setFormat(m_offscreenSurface->requestedFormat());

        if (shareContext) {
            m_glContext->setShareContext(shareContext);
            m_isContextShared = true;
        } else {
#ifdef QT_DEBUG_AVF
            qWarning("failed to get Render Thread context");
#endif
            m_isContextShared = false;
        }
        if (!m_glContext->create()) {
            qWarning("failed to create QOpenGLContext");
            return nullptr;
        }

        // CARenderer must be re-created with different current context, so release it now.
        // See lines below where m_videoLayerRenderer is constructed.
        if (m_videoLayerRenderer) {
            [m_videoLayerRenderer release];
            m_videoLayerRenderer = nullptr;
        }

        if (m_useCoreProfile) {
            glDeleteVertexArrays(1, &m_quadVao);
            glDeleteBuffers(2, m_quadVbos);
            delete m_shader;
            m_shader = nullptr;
        }
    }

    //Need current context
    if (m_glContext)
        m_glContext->makeCurrent(m_offscreenSurface);

    if (!m_metalDevice)
        m_metalDevice = MTLCreateSystemDefaultDevice();

    if (@available(macOS 10.13, *)) {
        m_useCoreProfile = m_metalDevice && (QOpenGLContext::currentContext()->format().profile() ==
                                             QSurfaceFormat::CoreProfile);
    } else {
        m_useCoreProfile = false;
    }

    // Create the CARenderer if needed for no Core OpenGL
    if (!m_videoLayerRenderer) {
        if (!m_useCoreProfile) {
            m_videoLayerRenderer = [CARenderer rendererWithCGLContext: CGLGetCurrentContext()
                                                              options: nil];
            [m_videoLayerRenderer retain];
        } else if (@available(macOS 10.13, *)) {
            // This is always true when m_useCoreProfile is true, but the compiler wants the check
            // anyway
            // Setup Core OpenGL shader, VAO, VBOs and metal renderer
            m_shader = new QOpenGLShaderProgram();
            m_shader->create();
            if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(#version 150 core
                                                   in vec2 qt_VertexPosition;
                                                   in vec2 qt_VertexTexCoord;
                                                   out vec2 qt_TexCoord;
                                                   void main()
                                                   {
                                                       qt_TexCoord = qt_VertexTexCoord;
                                                       gl_Position = vec4(qt_VertexPosition, 0.0f, 1.0f);
                                                   })")) {
                qCritical() << "Vertex shader compilation failed" << m_shader->log();
            }
            if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(#version 150 core
                                                   in vec2 qt_TexCoord;
                                                   out vec4 fragColor;
                                                   uniform sampler2DRect videoFrame;
                                                   void main(void)
                                                   {
                                                       ivec2 textureDim = textureSize(videoFrame);
                                                       fragColor = texture(videoFrame, qt_TexCoord * textureDim);
                                                   })")) {
                qCritical() << "Fragment shader compilation failed" << m_shader->log();
            }

            // Setup quad where the video frame will be attached
            GLfloat vertices[] = {
                -1.0f, -1.0f,
                 1.0f, -1.0f,
                -1.0f,  1.0f,
                 1.0f,  1.0f,
            };

            GLfloat uvs[] = {
                 0.0f,  0.0f,
                 1.0f,  0.0f,
                 0.0f,  1.0f,
                 1.0f,  1.0f,
            };

            glGenVertexArrays(1, &m_quadVao);
            glBindVertexArray(m_quadVao);

            // Create vertex buffer objects for vertices
            glGenBuffers(2, m_quadVbos);

            // Setup vertices
            glBindBuffer(GL_ARRAY_BUFFER, m_quadVbos[0]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
            glEnableVertexAttribArray(0);

            // Setup uvs
            glBindBuffer(GL_ARRAY_BUFFER, m_quadVbos[1]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
            glEnableVertexAttribArray(1);

            glBindVertexArray(0);

            // Setup shared Metal/OpenGL pixel buffer and textures
            m_NSGLContext = static_cast<QCocoaNativeContext*>((QOpenGLContext::currentContext()->nativeHandle().data()))->context();
            m_CGLPixelFormat = m_NSGLContext.pixelFormat.CGLPixelFormatObj;

            NSDictionary* cvBufferProperties = @{
                static_cast<NSString*>(kCVPixelBufferOpenGLCompatibilityKey) : @YES,
                static_cast<NSString*>(kCVPixelBufferMetalCompatibilityKey): @YES,
            };

            CVPixelBufferCreate(kCFAllocatorDefault, static_cast<size_t>(m_targetSize.width()),
                                static_cast<size_t>(m_targetSize.height()), kCVPixelFormatType_32BGRA,
                                static_cast<CFDictionaryRef>(cvBufferProperties), &m_CVPixelBuffer);

            m_textureName = createGLTexture(reinterpret_cast<CGLContextObj>(m_NSGLContext.CGLContextObj),
                                            m_CGLPixelFormat, m_CVGLTextureCache, m_CVPixelBuffer,
                                            m_CVGLTexture);
            m_metalTexture = createMetalTexture(m_metalDevice, m_CVMTLTextureCache, m_CVPixelBuffer,
                                                MTLPixelFormatBGRA8Unorm,
                                                static_cast<size_t>(m_targetSize.width()),
                                                static_cast<size_t>(m_targetSize.height()),
                                                m_CVMTLTexture);

            m_videoLayerRenderer = [CARenderer rendererWithMTLTexture:m_metalTexture options:nil];
            [m_videoLayerRenderer retain];
        }
    }

    //Set/Change render source if needed
    if (m_videoLayerRenderer.layer != layer) {
        m_videoLayerRenderer.layer = layer;
        m_videoLayerRenderer.bounds = layer.bounds;
    }

    //Do we have FBO's already?
    if ((!m_fbo[0] && !m_fbo[0]) || (m_fbo[0]->size() != m_targetSize)) {
        delete m_fbo[0];
        delete m_fbo[1];
        m_fbo[0] = new QOpenGLFramebufferObject(m_targetSize);
        m_fbo[1] = new QOpenGLFramebufferObject(m_targetSize);
    }

    //Switch buffer target
    m_currentBuffer = !m_currentBuffer;
    return m_fbo[m_currentBuffer];
}

void AVFVideoFrameRenderer::renderLayerToFBO(AVPlayerLayer *layer, QOpenGLFramebufferObject *fbo)
{
    //Start Rendering
    //NOTE: This rendering method will NOT work on iOS as there is no CARenderer in iOS
    if (!fbo->bind()) {
        qWarning("AVFVideoRender FBO failed to bind");
        return;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, m_targetSize.width(), m_targetSize.height());

    if (m_useCoreProfile) {
        CGLLockContext(m_NSGLContext.CGLContextObj);
        m_shader->bind();
        glBindVertexArray(m_quadVao);
    } else {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        // Render to FBO with inverted Y
        glOrtho(0.0, m_targetSize.width(), 0.0, m_targetSize.height(), 0.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
    }

    [m_videoLayerRenderer beginFrameAtTime:CACurrentMediaTime() timeStamp:nullptr];
    [m_videoLayerRenderer addUpdateRect:layer.bounds];
    [m_videoLayerRenderer render];
    [m_videoLayerRenderer endFrame];

    if (m_useCoreProfile) {
        glActiveTexture(0);
        glBindTexture(GL_TEXTURE_RECTANGLE, m_textureName);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindTexture(GL_TEXTURE_RECTANGLE, 0);

        glBindVertexArray(0);

        m_shader->release();

        CGLFlushDrawable(m_NSGLContext.CGLContextObj);
        CGLUnlockContext(m_NSGLContext.CGLContextObj);
    } else {
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }

    glFinish(); //Rendering needs to be done before passing texture to video frame

    fbo->release();
}

GLuint AVFVideoFrameRenderer::createGLTexture(CGLContextObj cglContextObj, CGLPixelFormatObj cglPixelFormtObj, CVOpenGLTextureCacheRef cvglTextureCache,
                                            CVPixelBufferRef cvPixelBufferRef, CVOpenGLTextureRef cvOpenGLTextureRef)
{
    CVReturn cvret;
    // Create an OpenGL CoreVideo texture cache from the pixel buffer.
    cvret  = CVOpenGLTextureCacheCreate(
                    kCFAllocatorDefault,
                    nil,
                    cglContextObj,
                    cglPixelFormtObj,
                    nil,
                    &cvglTextureCache);

    // Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
    cvret = CVOpenGLTextureCacheCreateTextureFromImage(
                    kCFAllocatorDefault,
                    cvglTextureCache,
                    cvPixelBufferRef,
                    nil,
                    &cvOpenGLTextureRef);

    // Get an OpenGL texture name from the CVPixelBuffer-backed OpenGL texture image.
    return CVOpenGLTextureGetName(cvOpenGLTextureRef);
}

id<MTLTexture> AVFVideoFrameRenderer::createMetalTexture(id<MTLDevice> mtlDevice, CVMetalTextureCacheRef cvMetalTextureCacheRef, CVPixelBufferRef cvPixelBufferRef,
                                               MTLPixelFormat pixelFormat, size_t width, size_t height, CVMetalTextureRef cvMetalTextureRef)
{
    CVReturn cvret;
    // Create a Metal Core Video texture cache from the pixel buffer.
    cvret = CVMetalTextureCacheCreate(
                    kCFAllocatorDefault,
                    nil,
                    mtlDevice,
                    nil,
                    &cvMetalTextureCacheRef);

    // Create a CoreVideo pixel buffer backed Metal texture image from the texture cache.
    cvret = CVMetalTextureCacheCreateTextureFromImage(
                    kCFAllocatorDefault,
                    cvMetalTextureCacheRef,
                    cvPixelBufferRef, nil,
                    pixelFormat,
                    width, height,
                    0,
                    &cvMetalTextureRef);

    // Get a Metal texture using the CoreVideo Metal texture reference.
    return CVMetalTextureGetTexture(cvMetalTextureRef);
}
