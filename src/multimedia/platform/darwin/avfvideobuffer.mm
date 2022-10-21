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

#include <private/qvideotexturehelper_p.h>

#import <AVFoundation/AVFoundation.h>
#import <Metal/Metal.h>

QT_USE_NAMESPACE

AVFVideoBuffer::AVFVideoBuffer(AVFVideoSinkInterface *sink, CVImageBufferRef buffer)
    : QAbstractVideoBuffer(sink->rhi() ? QVideoFrame::RhiTextureHandle : QVideoFrame::NoHandle, sink->rhi()),
      sink(sink),
      m_buffer(buffer)
{
//    m_type = QVideoFrame::NoHandle;
//    qDebug() << "RHI" << rhi;
    CVPixelBufferRetain(m_buffer);
    m_pixelFormat = fromCVVideoPixelFormat(CVPixelBufferGetPixelFormatType(m_buffer));
}

AVFVideoBuffer::~AVFVideoBuffer()
{
    AVFVideoBuffer::unmap();
    for (int i = 0; i < 3; ++i)
        if (cvMetalTexture[i])
            CFRelease(cvMetalTexture[i]);
#if defined(Q_OS_MACOS)
    if (cvOpenGLTexture)
        CVOpenGLTextureRelease(cvOpenGLTexture);
#elif defined(Q_OS_IOS)
    if (cvOpenGLESTexture)
        CFRelease(cvOpenGLESTexture);
#endif
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
    Q_ASSERT(mapData.nPlanes <= 3);

    if (!mapData.nPlanes) {
        // single plane
        mapData.bytesPerLine[0] = CVPixelBufferGetBytesPerRow(m_buffer);
        mapData.data[0] = static_cast<uchar*>(CVPixelBufferGetBaseAddress(m_buffer));
        mapData.size[0] = CVPixelBufferGetDataSize(m_buffer);
        mapData.nPlanes = mapData.data[0] ? 1 : 0;
        return mapData;
    }

    // For a bi-planar or tri-planar format we have to set the parameters correctly:
    for (int i = 0; i < mapData.nPlanes; ++i) {
        mapData.bytesPerLine[i] = CVPixelBufferGetBytesPerRowOfPlane(m_buffer, i);
        mapData.size[i] = mapData.bytesPerLine[i]*CVPixelBufferGetHeightOfPlane(m_buffer, i);
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

static MTLPixelFormat rhiTextureFormatToMetalFormat(QRhiTexture::Format f)
{
    switch (f) {
    default:
    case QRhiTexture::UnknownFormat:
        return MTLPixelFormatInvalid;
    case QRhiTexture::RGBA8:
        return MTLPixelFormatRGBA8Unorm;
    case QRhiTexture::BGRA8:
        return MTLPixelFormatBGRA8Unorm;
    case QRhiTexture::R8:
        return MTLPixelFormatR8Unorm;
    case QRhiTexture::RG8:
        return MTLPixelFormatRG8Unorm;
    case QRhiTexture::R16:
        return MTLPixelFormatR16Unorm;
    case QRhiTexture::RG16:
        return MTLPixelFormatRG16Unorm;

    case QRhiTexture::RGBA16F:
        return MTLPixelFormatRGBA16Float;
    case QRhiTexture::RGBA32F:
        return MTLPixelFormatRGBA32Float;
    case QRhiTexture::R16F:
        return MTLPixelFormatR16Float;
    case QRhiTexture::R32F:
        return MTLPixelFormatR32Float;
    }
}


quint64 AVFVideoBuffer::textureHandle(int plane) const
{
    auto *textureDescription = QVideoTextureHelper::textureDescription(m_pixelFormat);
    int bufferPlanes = CVPixelBufferGetPlaneCount(m_buffer);
//    qDebug() << "texture handle" << plane << rhi << (rhi->backend() == QRhi::Metal) << bufferPlanes;
    if (plane > 0 && plane >= bufferPlanes)
        return 0;
    if (!rhi)
        return 0;
    if (rhi->backend() == QRhi::Metal) {
        if (!cvMetalTexture[plane]) {
            size_t width = CVPixelBufferGetWidth(m_buffer);
            size_t height = CVPixelBufferGetHeight(m_buffer);
            width = textureDescription->widthForPlane(width, plane);
            height = textureDescription->heightForPlane(height, plane);

            // Create a CoreVideo pixel buffer backed Metal texture image from the texture cache.
            auto ret = CVMetalTextureCacheCreateTextureFromImage(
                            kCFAllocatorDefault,
                            sink->cvMetalTextureCache,
                            m_buffer, nil,
                            rhiTextureFormatToMetalFormat(textureDescription->textureFormat[plane]),
                            width, height,
                            plane,
                            &cvMetalTexture[plane]);

            if (ret != kCVReturnSuccess)
                qWarning() << "texture creation failed" << ret;
//            auto t = CVMetalTextureGetTexture(cvMetalTexture[plane]);
//            qDebug() << "    metal texture is" << quint64(cvMetalTexture[plane]) << width << height;
//            qDebug() << "    " << t.iosurfacePlane << t.pixelFormat << t.width << t.height;
        }

        // Get a Metal texture using the CoreVideo Metal texture reference.
//        qDebug() << "    -> " << quint64(CVMetalTextureGetTexture(cvMetalTexture[plane]));
        return cvMetalTexture[plane] ? quint64(CVMetalTextureGetTexture(cvMetalTexture[plane])) : 0;
    } else if (rhi->backend() == QRhi::OpenGLES2) {
#if QT_CONFIG(opengl)
#ifdef Q_OS_MACOS
        CVOpenGLTextureCacheFlush(sink->cvOpenGLTextureCache, 0);
        CVReturn cvret;
        // Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
        cvret = CVOpenGLTextureCacheCreateTextureFromImage(
                        kCFAllocatorDefault,
                        sink->cvOpenGLTextureCache,
                        m_buffer,
                        nil,
                        &cvOpenGLTexture);

        Q_ASSERT(CVOpenGLTextureGetTarget(cvOpenGLTexture) == GL_TEXTURE_RECTANGLE);
        // Get an OpenGL texture name from the CVPixelBuffer-backed OpenGL texture image.
        return CVOpenGLTextureGetName(cvOpenGLTexture);
#endif
#ifdef Q_OS_IOS
        CVOpenGLESTextureCacheFlush(sink->cvOpenGLESTextureCache, 0);
        CVReturn cvret;
        // Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
        cvret = CVOpenGLESTextureCacheCreateTextureFromImage(
                        kCFAllocatorDefault,
                        sink->cvOpenGLESTextureCache,
                        m_buffer,
                        nil,
                        GL_TEXTURE_2D,
                        GL_RGBA,
                        CVPixelBufferGetWidth(m_buffer),
                        CVPixelBufferGetHeight(m_buffer),
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        0,
                        &cvOpenGLESTexture);

        // Get an OpenGL texture name from the CVPixelBuffer-backed OpenGL texture image.
        return CVOpenGLESTextureGetName(cvOpenGLESTexture);
#endif
#endif
    }
    return 0;
}


QVideoFrameFormat::PixelFormat AVFVideoBuffer::fromCVVideoPixelFormat(unsigned avPixelFormat) const
{
#ifdef Q_OS_MACOS
    if (sink->rhi() && sink->rhi()->backend() == QRhi::OpenGLES2) {
        if (avPixelFormat == kCVPixelFormatType_32BGRA)
            return QVideoFrameFormat::Format_SamplerRect;
        else
            qWarning() << "Accelerated macOS OpenGL video supports BGRA only, got CV pixel format" << avPixelFormat;
    }
#endif
    return fromCVPixelFormat(avPixelFormat);
}

QVideoFrameFormat::PixelFormat AVFVideoBuffer::fromCVPixelFormat(unsigned avPixelFormat)
{
    switch (avPixelFormat) {
    case kCVPixelFormatType_32ARGB:
        return QVideoFrameFormat::Format_ARGB8888;
    case kCVPixelFormatType_32BGRA:
        return QVideoFrameFormat::Format_BGRA8888;
    case kCVPixelFormatType_420YpCbCr8Planar:
    case kCVPixelFormatType_420YpCbCr8PlanarFullRange:
        return QVideoFrameFormat::Format_YUV420P;
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
    case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
        return QVideoFrameFormat::Format_NV12;
    case kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange:
    case kCVPixelFormatType_420YpCbCr10BiPlanarFullRange:
        return QVideoFrameFormat::Format_P010;
    case kCVPixelFormatType_422YpCbCr8:
        return QVideoFrameFormat::Format_UYVY;
    case kCVPixelFormatType_422YpCbCr8_yuvs:
        return QVideoFrameFormat::Format_YUYV;
    case kCVPixelFormatType_OneComponent8:
        return QVideoFrameFormat::Format_Y8;
    case q_kCVPixelFormatType_OneComponent16:
        return QVideoFrameFormat::Format_Y16;

    case kCMVideoCodecType_JPEG:
    case kCMVideoCodecType_JPEG_OpenDML:
        return QVideoFrameFormat::Format_Jpeg;
    default:
        return QVideoFrameFormat::Format_Invalid;
    }
}

bool AVFVideoBuffer::toCVPixelFormat(QVideoFrameFormat::PixelFormat qtFormat, unsigned &conv)
{
    switch (qtFormat) {
    case QVideoFrameFormat::Format_ARGB8888:
        conv = kCVPixelFormatType_32ARGB;
        break;
    case QVideoFrameFormat::Format_BGRA8888:
        conv = kCVPixelFormatType_32BGRA;
        break;
    case QVideoFrameFormat::Format_YUV420P:
        conv = kCVPixelFormatType_420YpCbCr8PlanarFullRange;
        break;
    case QVideoFrameFormat::Format_NV12:
        conv = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
        break;
    case QVideoFrameFormat::Format_P010:
        conv = kCVPixelFormatType_420YpCbCr10BiPlanarFullRange;
        break;
    case QVideoFrameFormat::Format_UYVY:
        conv = kCVPixelFormatType_422YpCbCr8;
        break;
    case QVideoFrameFormat::Format_YUYV:
        conv = kCVPixelFormatType_422YpCbCr8_yuvs;
        break;
    case QVideoFrameFormat::Format_Y8:
        conv = kCVPixelFormatType_OneComponent8;
        break;
    case QVideoFrameFormat::Format_Y16:
        conv = q_kCVPixelFormatType_OneComponent16;
        break;
    default:
        return false;
    }

    return true;
}
