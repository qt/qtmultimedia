// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeghwaccel_videotoolbox_p.h"

#if !defined(Q_OS_DARWIN)
#error "Configuration error"
#endif

#include <qvideoframeformat.h>
#include <qffmpegvideobuffer_p.h>
#include <qloggingcategory.h>
#include "private/qvideotexturehelper_p.h"

#include <rhi/qrhi.h>

#include <CoreVideo/CVMetalTexture.h>
#include <CoreVideo/CVMetalTextureCache.h>

#include <qopenglcontext.h>
#ifdef Q_OS_MACOS
#import <AppKit/AppKit.h>
#endif
#ifdef Q_OS_IOS
#import <OpenGLES/EAGL.h>
#endif
#import <Metal/Metal.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcVideotoolbox, "qt.multimedia.ffmpeg.videotoolbox")

namespace QFFmpeg
{

static CVMetalTextureCacheRef &mtc(void *&cache) { return reinterpret_cast<CVMetalTextureCacheRef &>(cache); }

class VideoToolBoxTextureSet : public TextureSet
{
public:
    ~VideoToolBoxTextureSet();
    qint64 textureHandle(int plane) override;

    QRhi *rhi = nullptr;
    CVMetalTextureRef cvMetalTexture[3] = {};

#if defined(Q_OS_MACOS)
    CVOpenGLTextureRef cvOpenGLTexture = nullptr;
#elif defined(Q_OS_IOS)
    CVOpenGLESTextureRef cvOpenGLESTexture = nullptr;
#endif

    CVImageBufferRef m_buffer = nullptr;
};

VideoToolBoxTextureConverter::VideoToolBoxTextureConverter(QRhi *rhi)
    : TextureConverterBackend(rhi)
{
    if (!rhi)
        return;

    if (rhi->backend() == QRhi::Metal) {
        const auto *metal = static_cast<const QRhiMetalNativeHandles *>(rhi->nativeHandles());

        // Create a Metal Core Video texture cache from the pixel buffer.
        Q_ASSERT(!cvMetalTextureCache);
        if (CVMetalTextureCacheCreate(
                        kCFAllocatorDefault,
                        nil,
                        (id<MTLDevice>)metal->dev,
                        nil,
                        &mtc(cvMetalTextureCache)) != kCVReturnSuccess) {
            qWarning() << "Metal texture cache creation failed";
            rhi = nullptr;
        }
    } else if (rhi->backend() == QRhi::OpenGLES2) {
#if QT_CONFIG(opengl)
#ifdef Q_OS_MACOS
        const auto *gl = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());

        auto nsGLContext = gl->context->nativeInterface<QNativeInterface::QCocoaGLContext>()->nativeContext();
        auto nsGLPixelFormat = nsGLContext.pixelFormat.CGLPixelFormatObj;

        // Create an OpenGL CoreVideo texture cache from the pixel buffer.
        if (CVOpenGLTextureCacheCreate(
                        kCFAllocatorDefault,
                        nullptr,
                        reinterpret_cast<CGLContextObj>(nsGLContext.CGLContextObj),
                        nsGLPixelFormat,
                        nil,
                        &cvOpenGLTextureCache)) {
            qWarning() << "OpenGL texture cache creation failed";
            rhi = nullptr;
        }
#endif
#ifdef Q_OS_IOS
        // Create an OpenGL CoreVideo texture cache from the pixel buffer.
        if (CVOpenGLESTextureCacheCreate(
                        kCFAllocatorDefault,
                        nullptr,
                        [EAGLContext currentContext],
                        nullptr,
                        &cvOpenGLESTextureCache)) {
            qWarning() << "OpenGL texture cache creation failed";
            rhi = nullptr;
        }
#endif
#else
        rhi = nullptr;
#endif // QT_CONFIG(opengl)
    }
}

VideoToolBoxTextureConverter::~VideoToolBoxTextureConverter()
{
    freeTextureCaches();
}

void VideoToolBoxTextureConverter::freeTextureCaches()
{
    if (cvMetalTextureCache)
        CFRelease(cvMetalTextureCache);
    cvMetalTextureCache = nullptr;
#if defined(Q_OS_MACOS)
    if (cvOpenGLTextureCache)
        CFRelease(cvOpenGLTextureCache);
    cvOpenGLTextureCache = nullptr;
#elif defined(Q_OS_IOS)
    if (cvOpenGLESTextureCache)
        CFRelease(cvOpenGLESTextureCache);
    cvOpenGLESTextureCache = nullptr;
#endif
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

TextureSet *VideoToolBoxTextureConverter::getTextures(AVFrame *frame)
{
    if (!rhi)
        return nullptr;

    bool needsConversion = false;
    QVideoFrameFormat::PixelFormat pixelFormat = QFFmpegVideoBuffer::toQtPixelFormat(HWAccel::format(frame), &needsConversion);
    if (needsConversion) {
        // qDebug() << "XXXXXXXXXXXX pixel format needs conversion" << pixelFormat << HWAccel::format(frame);
        return nullptr;
    }

    CVPixelBufferRef buffer = (CVPixelBufferRef)frame->data[3];

    auto textureSet = std::make_unique<VideoToolBoxTextureSet>();
    textureSet->m_buffer = buffer;
    textureSet->rhi = rhi;
    CVPixelBufferRetain(buffer);

    auto *textureDescription = QVideoTextureHelper::textureDescription(pixelFormat);
    int bufferPlanes = CVPixelBufferGetPlaneCount(buffer);
//    qDebug() << "XXXXX getTextures" << pixelFormat << bufferPlanes << buffer;

    if (rhi->backend() == QRhi::Metal) {
        for (int plane = 0; plane < bufferPlanes; ++plane) {
            size_t width = CVPixelBufferGetWidth(buffer);
            size_t height = CVPixelBufferGetHeight(buffer);
            width = textureDescription->widthForPlane(width, plane);
            height = textureDescription->heightForPlane(height, plane);

            // Create a CoreVideo pixel buffer backed Metal texture image from the texture cache.
            auto ret = CVMetalTextureCacheCreateTextureFromImage(
                            kCFAllocatorDefault,
                            mtc(cvMetalTextureCache),
                            buffer, nil,
                            rhiTextureFormatToMetalFormat(textureDescription->textureFormat[plane]),
                            width, height,
                            plane,
                            &textureSet->cvMetalTexture[plane]);

            if (ret != kCVReturnSuccess)
                qWarning() << "texture creation failed" << ret;
//            auto t = CVMetalTextureGetTexture(textureSet->cvMetalTexture[plane]);
//            qDebug() << "    metal texture for plane" << plane << "is" << quint64(textureSet->cvMetalTexture[plane]) << width << height;
//            qDebug() << "    " << t.iosurfacePlane << t.pixelFormat << t.width << t.height;
        }
    } else if (rhi->backend() == QRhi::OpenGLES2) {
#if QT_CONFIG(opengl)
#ifdef Q_OS_MACOS
        CVOpenGLTextureCacheFlush(cvOpenGLTextureCache, 0);
        // Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
        const CVReturn cvret = CVOpenGLTextureCacheCreateTextureFromImage(
                        kCFAllocatorDefault,
                        cvOpenGLTextureCache,
                        buffer,
                        nil,
                        &textureSet->cvOpenGLTexture);
        if (cvret != kCVReturnSuccess) {
            qCWarning(qLcVideotoolbox) << "OpenGL texture creation failed" << cvret;
            return nullptr;
        }

        Q_ASSERT(CVOpenGLTextureGetTarget(textureSet->cvOpenGLTexture) == GL_TEXTURE_RECTANGLE);
#endif
#ifdef Q_OS_IOS
        CVOpenGLESTextureCacheFlush(cvOpenGLESTextureCache, 0);
        // Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
        const CVReturn cvret = CVOpenGLESTextureCacheCreateTextureFromImage(
                        kCFAllocatorDefault,
                        cvOpenGLESTextureCache,
                        buffer,
                        nil,
                        GL_TEXTURE_2D,
                        GL_RGBA,
                        CVPixelBufferGetWidth(buffer),
                        CVPixelBufferGetHeight(buffer),
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        0,
                        &textureSet->cvOpenGLESTexture);
        if (cvret != kCVReturnSuccess) {
            qCWarning(qLcVideotoolbox) << "OpenGL ES texture creation failed" << cvret;
            return nullptr;
        }
#endif
#endif
    }

    return textureSet.release();
}

VideoToolBoxTextureSet::~VideoToolBoxTextureSet()
{
    for (int i = 0; i < 4; ++i)
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

qint64 VideoToolBoxTextureSet::textureHandle(int plane)
{
    if (rhi->backend() == QRhi::Metal)
        return cvMetalTexture[plane] ? qint64(CVMetalTextureGetTexture(cvMetalTexture[plane])) : 0;
#if QT_CONFIG(opengl)
    Q_ASSERT(plane == 0);
#ifdef Q_OS_MACOS
    return CVOpenGLTextureGetName(cvOpenGLTexture);
#endif
#ifdef Q_OS_IOS
    return CVOpenGLESTextureGetName(cvOpenGLESTexture);
#endif
#endif
}

}

QT_END_NAMESPACE
