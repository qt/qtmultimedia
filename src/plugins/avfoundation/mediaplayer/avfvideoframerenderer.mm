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
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOffscreenSurface>

#include <QtCore/private/qcore_mac_p.h>

#ifdef QT_DEBUG_AVF
#include <QtCore/qdebug.h>
#endif

#ifdef Q_OS_MACOS
#import <AppKit/AppKit.h>
#include <CoreVideo/CVOpenGLTextureCache.h>
#endif

#import <CoreVideo/CVBase.h>
#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

AVFVideoFrameRenderer::AVFVideoFrameRenderer(QAbstractVideoSurface *surface, QObject *parent)
    : QObject(parent)
    , m_glContext(nullptr)
    , m_offscreenSurface(nullptr)
    , m_surface(surface)
    , m_textureCache(nullptr)
    , m_videoOutput(nullptr)
    , m_isContextShared(true)
{
    m_videoOutput = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:@{
        (NSString *)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
        (NSString *)kCVPixelBufferOpenGLCompatibilityKey: @YES
    }];
    [m_videoOutput setDelegate:nil queue:nil];

#ifdef Q_OS_MACOS
    m_fbo[0] = nullptr;
    m_fbo[1] = nullptr;
#endif
}

AVFVideoFrameRenderer::~AVFVideoFrameRenderer()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif

    [m_videoOutput release];
    if (m_textureCache)
        CFRelease(m_textureCache);
    delete m_offscreenSurface;
    delete m_glContext;

#ifdef Q_OS_MACOS
    delete m_fbo[0];
    delete m_fbo[1];
#endif
}

#ifdef Q_OS_MACOS
GLuint AVFVideoFrameRenderer::renderLayerToFBO(AVPlayerLayer *layer, QSize *size)
{
    QCFType<CVOGLTextureRef> texture = renderLayerToTexture(layer, size);
    if (!texture)
        return 0;

    Q_ASSERT(size);

    // Do we have FBO's already?
    if ((!m_fbo[0] && !m_fbo[0]) || (m_fbo[0]->size() != *size)) {
        delete m_fbo[0];
        delete m_fbo[1];
        m_fbo[0] = new QOpenGLFramebufferObject(*size);
        m_fbo[1] = new QOpenGLFramebufferObject(*size);
    }

    // Switch buffer target
    m_currentFBO = !m_currentFBO;
    QOpenGLFramebufferObject *fbo = m_fbo[m_currentFBO];

    if (!fbo || !fbo->bind())
        return 0;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, size->width(), size->height());

    if (!m_blitter.isCreated())
        m_blitter.create();

    m_blitter.bind(GL_TEXTURE_RECTANGLE);
    m_blitter.blit(CVOpenGLTextureGetName(texture), QMatrix4x4(), QMatrix3x3());
    m_blitter.release();

    glFinish();

    fbo->release();
    return fbo->texture();
}
#endif

CVOGLTextureRef AVFVideoFrameRenderer::renderLayerToTexture(AVPlayerLayer *layer, QSize *size)
{
    initRenderer();

    // If the glContext isn't shared, it doesn't make sense to return a texture for us
    if (!m_isContextShared)
        return nullptr;

    size_t width = 0, height = 0;
    auto texture = createCacheTextureFromLayer(layer, width, height);
    if (size)
        *size = QSize(width, height);
    return texture;
}

CVPixelBufferRef AVFVideoFrameRenderer::copyPixelBufferFromLayer(AVPlayerLayer *layer,
    size_t& width, size_t& height)
{
    //Is layer valid
    if (!layer) {
#ifdef QT_DEBUG_AVF
        qWarning("copyPixelBufferFromLayer: invalid layer");
#endif
        return nullptr;
    }

    AVPlayerItem *item = layer.player.currentItem;
    if (![item.outputs containsObject:m_videoOutput])
        [item addOutput:m_videoOutput];

    CFTimeInterval currentCAFrameTime = CACurrentMediaTime();
    CMTime currentCMFrameTime = [m_videoOutput itemTimeForHostTime:currentCAFrameTime];

    // Happens when buffering / loading
    if (CMTimeCompare(currentCMFrameTime, kCMTimeZero) < 0)
        return nullptr;

    if (![m_videoOutput hasNewPixelBufferForItemTime:currentCMFrameTime])
        return nullptr;

    CVPixelBufferRef pixelBuffer = [m_videoOutput copyPixelBufferForItemTime:currentCMFrameTime
                                                   itemTimeForDisplay:nil];
    if (!pixelBuffer) {
#ifdef QT_DEBUG_AVF
        qWarning("copyPixelBufferForItemTime returned nil");
        CMTimeShow(currentCMFrameTime);
#endif
        return nullptr;
    }

    width = CVPixelBufferGetWidth(pixelBuffer);
    height = CVPixelBufferGetHeight(pixelBuffer);
    return pixelBuffer;
}

CVOGLTextureRef AVFVideoFrameRenderer::createCacheTextureFromLayer(AVPlayerLayer *layer,
        size_t& width, size_t& height)
{
    CVPixelBufferRef pixelBuffer = copyPixelBufferFromLayer(layer, width, height);

    if (!pixelBuffer)
        return nullptr;

    CVOGLTextureCacheFlush(m_textureCache, 0);

    CVOGLTextureRef texture = nullptr;
#ifdef Q_OS_MACOS
    CVReturn err = CVOpenGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                              m_textureCache,
                                                              pixelBuffer,
                                                              nil,
                                                              &texture);
#else
    CVReturn err = CVOGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault, m_textureCache, pixelBuffer, nullptr,
                                                           GL_TEXTURE_2D, GL_RGBA,
                                                           (GLsizei) width, (GLsizei) height,
                                                           GL_BGRA, GL_UNSIGNED_BYTE, 0,
                                                           &texture);
#endif

    if (!texture || err) {
        qWarning() << "CVOGLTextureCacheCreateTextureFromImage failed error:" << err << m_textureCache;
    }

    CVPixelBufferRelease(pixelBuffer);

    return texture;
}

QImage AVFVideoFrameRenderer::renderLayerToImage(AVPlayerLayer *layer, QSize *size)
{
    size_t width = 0;
    size_t height = 0;
    CVPixelBufferRef pixelBuffer = copyPixelBufferFromLayer(layer, width, height);
    if (size)
        *size = QSize(width, height);

    if (!pixelBuffer)
        return QImage();

    OSType pixelFormat = CVPixelBufferGetPixelFormatType(pixelBuffer);
    if (pixelFormat != kCVPixelFormatType_32BGRA) {
#ifdef QT_DEBUG_AVF
        qWarning("CVPixelBuffer format is not BGRA32 (got: %d)", static_cast<quint32>(pixelFormat));
#endif
        return QImage();
    }

    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    char *data = (char *)CVPixelBufferGetBaseAddress(pixelBuffer);
    size_t stride = CVPixelBufferGetBytesPerRow(pixelBuffer);

    // format here is not relevant, only using for storage
    QImage img = QImage(width, height, QImage::Format_ARGB32);
    for (size_t j = 0; j < height; j++) {
        memcpy(img.scanLine(j), data, width * 4);
        data += stride;
    }

    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    CVPixelBufferRelease(pixelBuffer);
    return img;
}

void AVFVideoFrameRenderer::initRenderer()
{
    // even for using a texture directly, we need to be able to make a context current,
    // so we need an offscreen, and we shouldn't assume we can make the surface context
    // current on that offscreen, so use our own (sharing with it). Slightly
    // excessive but no performance penalty and makes the QImage path easier to maintain

    //Make sure we have an OpenGL context to make current
    if (!m_glContext) {
        //Create OpenGL context and set share context from surface
        QOpenGLContext *shareContext = nullptr;
        if (m_surface)
            shareContext = qobject_cast<QOpenGLContext*>(m_surface->property("GLContext").value<QObject*>());

        m_glContext = new QOpenGLContext();
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
#ifdef QT_DEBUG_AVF
            qWarning("failed to create QOpenGLContext");
#endif
            return;
        }
    }

    if (!m_offscreenSurface) {
        m_offscreenSurface = new QOffscreenSurface();
        m_offscreenSurface->setFormat(m_glContext->format());
        m_offscreenSurface->create();
    }

    // Need current context
    m_glContext->makeCurrent(m_offscreenSurface);

    if (!m_textureCache) {
#ifdef Q_OS_MACOS
        auto *currentContext = NSOpenGLContext.currentContext;
        // Create an OpenGL CoreVideo texture cache from the pixel buffer.
        auto err = CVOpenGLTextureCacheCreate(
                        kCFAllocatorDefault,
                        nullptr,
                        currentContext.CGLContextObj,
                        currentContext.pixelFormat.CGLPixelFormatObj,
                        nil,
                        &m_textureCache);
#else
        CVReturn err = CVOGLTextureCacheCreate(kCFAllocatorDefault, nullptr,
            [EAGLContext currentContext],
            nullptr, &m_textureCache);
#endif
        if (err)
            qWarning("Error at CVOGLTextureCacheCreate %d", err);
    }

}
