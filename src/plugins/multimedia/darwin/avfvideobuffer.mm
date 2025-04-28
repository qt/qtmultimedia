// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfvideobuffer_p.h"
#include <rhi/qrhi.h>
#include <CoreVideo/CVMetalTexture.h>
#include <CoreVideo/CVMetalTextureCache.h>
#include <QtGui/qopenglcontext.h>
#include <QtCore/qloggingcategory.h>

#include <private/qvideotexturehelper_p.h>
#include <QtMultimedia/private/qavfhelpers_p.h>

#import <AVFoundation/AVFoundation.h>
#import <Metal/Metal.h>

QT_USE_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcVideoBuffer, "qt.multimedia.darwin.videobuffer")

AVFVideoBuffer::AVFVideoBuffer(AVFVideoSinkInterface *sink, QCFType<CVImageBufferRef> buffer)
    : QHwVideoBuffer(sink->rhi() ? QVideoFrame::RhiTextureHandle : QVideoFrame::NoHandle,
                     sink->rhi()),
      sink(sink),
      m_buffer(std::move(buffer))
{
//    m_type = QVideoFrame::NoHandle;
//    qDebug() << "RHI" << m_rhi;
    const bool rhiIsOpenGL = sink && sink->rhi() && sink->rhi()->backend() == QRhi::OpenGLES2;
    m_format = QAVFHelpers::videoFormatForImageBuffer(m_buffer, rhiIsOpenGL);

    if (m_rhi && m_rhi->backend() == QRhi::Metal)
        metalCache = CVMetalTextureCacheRef(CFRetain(sink->cvMetalTextureCache));
}

AVFVideoBuffer::~AVFVideoBuffer()
{
    Q_ASSERT(m_mode == QVideoFrame::NotMapped);
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

    mapData.planeCount = CVPixelBufferGetPlaneCount(m_buffer);
    Q_ASSERT(mapData.planeCount <= 3);

    if (!mapData.planeCount) {
        // single plane
        mapData.bytesPerLine[0] = CVPixelBufferGetBytesPerRow(m_buffer);
        mapData.data[0] = static_cast<uchar*>(CVPixelBufferGetBaseAddress(m_buffer));
        mapData.dataSize[0] = CVPixelBufferGetDataSize(m_buffer);
        mapData.planeCount = mapData.data[0] ? 1 : 0;
        return mapData;
    }

    // For a bi-planar or tri-planar format we have to set the parameters correctly:
    for (int i = 0; i < mapData.planeCount; ++i) {
        mapData.bytesPerLine[i] = CVPixelBufferGetBytesPerRowOfPlane(m_buffer, i);
        mapData.dataSize[i] = mapData.bytesPerLine[i]*CVPixelBufferGetHeightOfPlane(m_buffer, i);
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
    case QRhiTexture::RED_OR_ALPHA8:
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


quint64 AVFVideoBuffer::textureHandle(QRhi &, int plane)
{
    auto *textureDescription = QVideoTextureHelper::textureDescription(m_format.pixelFormat());
    int bufferPlanes = CVPixelBufferGetPlaneCount(m_buffer);
//    qDebug() << "texture handle" << plane << m_rhi << (m_rhi->backend() == QRhi::Metal) << bufferPlanes;
    if (plane > 0 && plane >= bufferPlanes)
        return 0;
    if (!m_rhi)
        return 0;
    if (m_rhi->backend() == QRhi::Metal) {
        if (!cvMetalTexture[plane]) {
            size_t width = CVPixelBufferGetWidth(m_buffer);
            size_t height = CVPixelBufferGetHeight(m_buffer);
            QSize planeSize = textureDescription->rhiPlaneSize(QSize(width, height), plane, m_rhi);

            if (!metalCache) {
                qWarning("cannot create texture, Metal texture cache was released?");
                return {};
            }

            // Create a CoreVideo pixel buffer backed Metal texture image from the texture cache.
            const auto pixelFormat = rhiTextureFormatToMetalFormat(textureDescription->rhiTextureFormat(plane, m_rhi));
            if (pixelFormat != MTLPixelFormatInvalid) {
                // Passing invalid pixel format makes Metal API validation
                // to crash (and also makes no sense at all).
                auto ret = CVMetalTextureCacheCreateTextureFromImage(
                                kCFAllocatorDefault,
                                metalCache,
                                m_buffer, nil,
                                pixelFormat,
                                planeSize.width(), planeSize.height(),
                                plane,
                                &cvMetalTexture[plane]);

                if (ret != kCVReturnSuccess)
                    qCWarning(qLcVideoBuffer) << "texture creation failed" << ret;
            } else {
                qCWarning(qLcVideoBuffer) << "requested invalid pixel format:"
                                          << textureDescription->rhiTextureFormat(plane, m_rhi);
            }
        }

        return cvMetalTexture[plane] ? quint64(CVMetalTextureGetTexture(cvMetalTexture[plane])) : 0;
    } else if (m_rhi->backend() == QRhi::OpenGLES2) {
#if QT_CONFIG(opengl)
#ifdef Q_OS_MACOS
        CVOpenGLTextureCacheFlush(sink->cvOpenGLTextureCache, 0);
        // Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
        const CVReturn cvret = CVOpenGLTextureCacheCreateTextureFromImage(
                        kCFAllocatorDefault,
                        sink->cvOpenGLTextureCache,
                        m_buffer,
                        nil,
                        &cvOpenGLTexture);
        if (cvret != kCVReturnSuccess)
            qWarning() << "OpenGL texture creation failed" << cvret;

        Q_ASSERT(CVOpenGLTextureGetTarget(cvOpenGLTexture) == GL_TEXTURE_RECTANGLE);
        // Get an OpenGL texture name from the CVPixelBuffer-backed OpenGL texture image.
        return CVOpenGLTextureGetName(cvOpenGLTexture);
#endif
#ifdef Q_OS_IOS
        CVOpenGLESTextureCacheFlush(sink->cvOpenGLESTextureCache, 0);
        // Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
        const CVReturn cvret = CVOpenGLESTextureCacheCreateTextureFromImage(
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
        if (cvret != kCVReturnSuccess)
            qWarning() << "OpenGL ES texture creation failed" << cvret;

        // Get an OpenGL texture name from the CVPixelBuffer-backed OpenGL texture image.
        return CVOpenGLESTextureGetName(cvOpenGLESTexture);
#endif
#endif
    }
    return 0;
}
