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

#include "avfvideobuffer_p.h"
#include <private/qrhi_p.h>
#include <private/qrhimetal_p.h>
#include <private/qrhigles2_p.h>
#include <CoreVideo/CVMetalTexture.h>
#include <CoreVideo/CVMetalTextureCache.h>
#include <QtGui/qopenglcontext.h>

#import <AVFoundation/AVFoundation.h>
#import <Metal/Metal.h>

QT_USE_NAMESPACE

AVFVideoBuffer::AVFVideoBuffer(QRhi *rhi, CVImageBufferRef buffer)
    : QAbstractVideoBuffer(rhi ? QVideoFrame::RhiTextureHandle : QVideoFrame::NoHandle),
      rhi(rhi),
      m_buffer(buffer)
{
//    m_type = QVideoFrame::NoHandle;
//    qDebug() << "RHI" << rhi;
    CVPixelBufferRetain(m_buffer);
}

AVFVideoBuffer::~AVFVideoBuffer()
{
    AVFVideoBuffer::unmap();
    if (cvMetalTexture)
        CFRelease(cvMetalTexture);
    if (cvMetalTextureCache)
        CFRelease(cvMetalTextureCache);
    if (cvOpenGLTexture)
        CFRelease(cvOpenGLTexture);
    if (cvOpenGLTextureCache)
        CFRelease(cvOpenGLTextureCache);
    CVPixelBufferRelease(m_buffer);
}

AVFVideoBuffer::MapData AVFVideoBuffer::map(QVideoFrame::MapMode mode)
{
    MapData mapData;

    if (m_mode == QVideoFrame::NotMapped) {
        CVPixelBufferLockBaseAddress(m_buffer, mode == QVideoFrame::ReadOnly
                                                           ? kCVPixelBufferLock_ReadOnly
                                                           : 0);
        m_mode = mode;
    }

    mapData.nPlanes = CVPixelBufferGetPlaneCount(m_buffer);
    mapData.nBytes = CVPixelBufferGetDataSize(m_buffer);
    Q_ASSERT(mapData.nPlanes <= 3);

    if (!mapData.nPlanes) {
        // single plane
        mapData.bytesPerLine[0] = CVPixelBufferGetBytesPerRow(m_buffer);
        mapData.data[0] = static_cast<uchar*>(CVPixelBufferGetBaseAddress(m_buffer));
        mapData.nPlanes = mapData.data[0] ? 1 : 0;
        return mapData;
    }

    // For a bi-planar or tri-planar format we have to set the parameters correctly:
    for (int i = 0; i < mapData.nPlanes; ++i) {
        mapData.bytesPerLine[i] = CVPixelBufferGetBytesPerRowOfPlane(m_buffer, i);
        mapData.data[i] = static_cast<uchar*>(CVPixelBufferGetBaseAddressOfPlane(m_buffer, i));
    }

    return mapData;
}

void AVFVideoBuffer::unmap()
{
    if (m_mode != QVideoFrame::NotMapped) {
        CVPixelBufferUnlockBaseAddress(m_buffer, m_mode == QVideoFrame::ReadOnly
                                                               ? kCVPixelBufferLock_ReadOnly
                                                               : 0);
        m_mode = QVideoFrame::NotMapped;
    }
}

quint64 AVFVideoBuffer::textureHandle(int plane) const
{
//    qDebug() << "texture handle" << plane << rhi << (rhi->backend() == QRhi::Metal);
    if (plane != 0)
        return 0;
    if (!rhi)
        return 0;
    if (rhi->backend() == QRhi::Metal) {
        if (metalTexture == nil) {
            const auto *metal = static_cast<const QRhiMetalNativeHandles *>(rhi->nativeHandles());

            // Create a Metal Core Video texture cache from the pixel buffer.
            if (CVMetalTextureCacheCreate(
                            kCFAllocatorDefault,
                            nil,
                            (id<MTLDevice>)metal->dev,
                            nil,
                            &cvMetalTextureCache) != kCVReturnSuccess)
                qWarning() << "texture cache creation failed";

            // Create a CoreVideo pixel buffer backed Metal texture image from the texture cache.
            auto ret = CVMetalTextureCacheCreateTextureFromImage(
                            kCFAllocatorDefault,
                            cvMetalTextureCache,
                            m_buffer, nil,
                            MTLPixelFormatBGRA8Unorm,
                            CVPixelBufferGetWidth(m_buffer), CVPixelBufferGetHeight(m_buffer),
                            0,
                            &cvMetalTexture);
            if (ret != kCVReturnSuccess)
                qWarning() << "texture creation failed" << ret;
            metalTexture = CVMetalTextureGetTexture(cvMetalTexture);
        }
//        qDebug() << "    -> " << quint64(metalTexture);

        // Get a Metal texture using the CoreVideo Metal texture reference.
        return quint64(metalTexture);
    } else if (rhi->backend() == QRhi::OpenGLES2) {
        const auto *gl = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());

        auto nsGLContext = gl->context->nativeInterface<QNativeInterface::QCocoaGLContext>()->nativeContext();
        auto nsGLPixelFormat = nsGLContext.pixelFormat.CGLPixelFormatObj;

        CVReturn cvret;
        // Create an OpenGL CoreVideo texture cache from the pixel buffer.
        cvret  = CVOpenGLTextureCacheCreate(
                        kCFAllocatorDefault,
                        nullptr,
                        reinterpret_cast<CGLContextObj>(nsGLContext.CGLContextObj),
                        nsGLPixelFormat,
                        nil,
                        &cvOpenGLTextureCache);

        // Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
        cvret = CVOpenGLTextureCacheCreateTextureFromImage(
                        kCFAllocatorDefault,
                        cvOpenGLTextureCache,
                        m_buffer,
                        nil,
                        &cvOpenGLTexture);

        // Get an OpenGL texture name from the CVPixelBuffer-backed OpenGL texture image.
        return CVOpenGLTextureGetName(cvOpenGLTexture);

    }
    return 0;
#ifdef Q_OS_IOS
    // Called from the render thread, so there is a current OpenGL context

    if (!m_renderer->m_textureCache) {
        CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault,
                                                    nullptr,
                                                    [EAGLContext currentContext],
                                                    nullptr,
                                                    &m_renderer->m_textureCache);

        if (err != kCVReturnSuccess)
            qWarning("Error creating texture cache");
    }

    if (m_renderer->m_textureCache && !m_texture) {
        CVOpenGLESTextureCacheFlush(m_renderer->m_textureCache, 0);

        CVReturn err = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                                    m_renderer->m_textureCache,
                                                                    m_buffer,
                                                                    nullptr,
                                                                    GL_TEXTURE_2D,
                                                                    GL_RGBA,
                                                                    CVPixelBufferGetWidth(m_buffer),
                                                                    CVPixelBufferGetHeight(m_buffer),
                                                                    GL_RGBA,
                                                                    GL_UNSIGNED_BYTE,
                                                                    0,
                                                                    &m_texture);
        if (err != kCVReturnSuccess)
            qWarning("Error creating texture from buffer");
    }

    if (m_texture)
        return CVOpenGLESTextureGetName(m_texture);
    else
        return 0;
#endif
}
