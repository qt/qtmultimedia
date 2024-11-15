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

Q_STATIC_LOGGING_CATEGORY(qLcVideotoolbox, "qt.multimedia.ffmpeg.videotoolbox");

namespace QFFmpeg
{

namespace {
CVMetalTextureCacheRef &mtc(void *&cache) { return reinterpret_cast<CVMetalTextureCacheRef &>(cache); }

class VideoToolBoxTextureHandles : public QVideoFrameTexturesHandles
{
public:
    ~VideoToolBoxTextureHandles();
    quint64 textureHandle(QRhi &, int plane) override;

    TextureConverterBackendPtr parentConverterBackend; // ensures the backend is deleted after the texture

    QRhi *rhi = nullptr;
    CVMetalTextureRef cvMetalTexture[3] = {};

#if defined(Q_OS_MACOS)
    CVOpenGLTextureRef cvOpenGLTexture = nullptr;
#elif defined(Q_OS_IOS)
    CVOpenGLESTextureRef cvOpenGLESTexture = nullptr;
#endif

    CVImageBufferRef m_buffer = nullptr;
};
}

VideoToolBoxTextureConverter::VideoToolBoxTextureConverter(QRhi *targetRhi)
    : TextureConverterBackend(targetRhi)
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

QVideoFrameTexturesHandlesUPtr
VideoToolBoxTextureConverter::createTextureHandles(AVFrame *frame,
                                                   QVideoFrameTexturesHandlesUPtr /*oldHandles*/)
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

    auto textureHandles = std::make_unique<VideoToolBoxTextureHandles>();
    textureHandles->parentConverterBackend = shared_from_this();
    textureHandles->m_buffer = buffer;
    textureHandles->rhi = rhi;
    CVPixelBufferRetain(buffer);

    auto *textureDescription = QVideoTextureHelper::textureDescription(pixelFormat);
    int bufferPlanes = CVPixelBufferGetPlaneCount(buffer);
    //    qDebug() << "XXXXX createTextureHandles" << pixelFormat << bufferPlanes << buffer;

    if (rhi->backend() == QRhi::Metal) {
        // First check that all planes have pixel-formats that we can handle,
        // before we create any Metal textures.
        for (int plane = 0; plane < bufferPlanes; ++plane) {
            const MTLPixelFormat metalPixelFormatForPlane =
                rhiTextureFormatToMetalFormat(textureDescription->rhiTextureFormat(plane, rhi));
            if (metalPixelFormatForPlane == MTLPixelFormatInvalid)
                return nullptr;
        }

        for (int plane = 0; plane < bufferPlanes; ++plane) {
            size_t width = CVPixelBufferGetWidth(buffer);
            size_t height = CVPixelBufferGetHeight(buffer);
            width = textureDescription->widthForPlane(width, plane);
            height = textureDescription->heightForPlane(height, plane);

            // Tested to be valid in prior loop.
            const MTLPixelFormat metalPixelFormatForPlane =
                rhiTextureFormatToMetalFormat(textureDescription->rhiTextureFormat(plane, rhi));

            // Create a CoreVideo pixel buffer backed Metal texture image from the texture cache.
            auto ret = CVMetalTextureCacheCreateTextureFromImage(
                            kCFAllocatorDefault,
                            mtc(cvMetalTextureCache),
                            buffer, nil,
                            metalPixelFormatForPlane,
                            width, height,
                            plane,
                            &textureHandles->cvMetalTexture[plane]);

            if (ret != kCVReturnSuccess)
                qWarning() << "texture creation failed" << ret;
//            auto t = CVMetalTextureGetTexture(textureHandles->cvMetalTexture[plane]);
//            qDebug() << "    metal texture for plane" << plane << "is" << quint64(textureHandles->cvMetalTexture[plane]) << width << height;
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
                        &textureHandles->cvOpenGLTexture);
        if (cvret != kCVReturnSuccess) {
            qCWarning(qLcVideotoolbox) << "OpenGL texture creation failed" << cvret;
            return nullptr;
        }

        Q_ASSERT(CVOpenGLTextureGetTarget(textureHandles->cvOpenGLTexture) == GL_TEXTURE_RECTANGLE);
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
                        &textureHandles->cvOpenGLESTexture);
        if (cvret != kCVReturnSuccess) {
            qCWarning(qLcVideotoolbox) << "OpenGL ES texture creation failed" << cvret;
            return nullptr;
        }
#endif
#endif
    }

    return textureHandles;
}

VideoToolBoxTextureHandles::~VideoToolBoxTextureHandles()
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

quint64 VideoToolBoxTextureHandles::textureHandle(QRhi &, int plane)
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
