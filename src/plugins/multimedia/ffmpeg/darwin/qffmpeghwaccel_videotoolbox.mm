// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_videotoolbox_p.h>

#if !defined(Q_OS_DARWIN)
#error "Configuration error"
#endif

#include <QtCore/qloggingcategory.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>

#include <QtGui/qopenglcontext.h>
#include <QtGui/rhi/qrhi.h>

#include <QtMultimedia/private/qavfhelpers_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtMultimedia/qvideoframeformat.h>

#include <CoreVideo/CVMetalTexture.h>
#include <CoreVideo/CVMetalTextureCache.h>

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

class VideoToolBoxTextureHandles : public QVideoFrameTexturesHandles
{
public:
    ~VideoToolBoxTextureHandles();
    quint64 textureHandle(QRhi &, int plane) override;

    TextureConverterBackendPtr parentConverterBackend; // ensures the backend is deleted after the texture

    QRhi *rhi = nullptr;
    QCFType<CVMetalTextureRef> m_cvMetalTexture[3] = {};

#if defined(Q_OS_MACOS)
    QCFType<CVOpenGLTextureRef> m_cvOpenGLTexture;
#elif defined(Q_OS_IOS)
    QCFType<CVOpenGLESTextureRef> m_cvOpenGLESTexture;
#endif

    QAVFHelpers::QSharedCVPixelBuffer m_buffer;
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
        Q_ASSERT(!m_cvMetalTextureCache);
        CVMetalTextureCacheRef outCvMetalTexCacheRef = nullptr;
        CVReturn cvReturn = CVMetalTextureCacheCreate(
            kCFAllocatorDefault,
            nil,
            (id<MTLDevice>)metal->dev,
            nil,
            &outCvMetalTexCacheRef);
        if (cvReturn != kCVReturnSuccess) {
            qCWarning(qLcVideotoolbox) << "Metal texture cache creation failed";
            rhi = nullptr;
        }

        Q_ASSERT(outCvMetalTexCacheRef);
        m_cvMetalTextureCache = QCFType<CVMetalTextureCacheRef>{ outCvMetalTexCacheRef };

    } else if (rhi->backend() == QRhi::OpenGLES2) {
#if QT_CONFIG(opengl)
#ifdef Q_OS_MACOS
        const auto *gl = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());

        // Create an OpenGL CoreVideo texture cache from the pixel buffer.
        NSOpenGLContext *nsGLContext =
            gl->context->nativeInterface<QNativeInterface::QCocoaGLContext>()->nativeContext();
        CGLPixelFormatObj nsGLPixelFormat = nsGLContext.pixelFormat.CGLPixelFormatObj;
        CVOpenGLTextureCacheRef outCvGlTexCacheRef = nullptr;
        CVReturn cvReturn = CVOpenGLTextureCacheCreate(
            kCFAllocatorDefault,
            nullptr,
            reinterpret_cast<CGLContextObj>(nsGLContext.CGLContextObj),
            nsGLPixelFormat,
            nil,
            &outCvGlTexCacheRef);
        if (cvReturn != kCVReturnSuccess) {
            qCWarning(qLcVideotoolbox) << "OpenGL texture cache creation failed";
            rhi = nullptr;
        }

        Q_ASSERT(outCvGlTexCacheRef);
        m_cvOpenGLTextureCache = QCFType<CVOpenGLTextureCacheRef>{ outCvGlTexCacheRef };

#endif
#ifdef Q_OS_IOS
        // Create an OpenGL CoreVideo texture cache from the pixel buffer.
        CVOpenGLESTextureCacheRef outCvGlEsTexCacheRef = nullptr;
        CVReturn cvReturn = CVOpenGLESTextureCacheCreate(
            kCFAllocatorDefault,
            nullptr,
            [EAGLContext currentContext],
            nullptr,
            &outCvGlEsTexCacheRef);
        if (cvReturn != kCVReturnSuccess) {
            qCWarning(qLcVideotoolbox) << "OpenGLES texture cache creation failed";
            rhi = nullptr;
        }

        Q_ASSERT(outCvGlEsTexCacheRef);
        m_cvOpenGLESTextureCache = QCFType<CVOpenGLESTextureCacheRef>{ outCvGlEsTexCacheRef };

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
    m_cvMetalTextureCache = nullptr;
#if defined(Q_OS_MACOS)
    m_cvOpenGLTextureCache = nullptr;
#elif defined(Q_OS_IOS)
    m_cvOpenGLESTextureCache = nullptr;
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
        return nullptr;
    }

    auto cvPixelBufferRef = reinterpret_cast<CVPixelBufferRef>(frame->data[3]);
    Q_ASSERT(cvPixelBufferRef);

    auto textureHandles = std::make_unique<VideoToolBoxTextureHandles>();
    textureHandles->parentConverterBackend = shared_from_this();
    textureHandles->m_buffer = QAVFHelpers::QSharedCVPixelBuffer(
        cvPixelBufferRef,
        QAVFHelpers::QSharedCVPixelBuffer::RefMode::NeedsRef);
    textureHandles->rhi = rhi;

    auto *textureDescription = QVideoTextureHelper::textureDescription(pixelFormat);
    int bufferPlanes = CVPixelBufferGetPlaneCount(textureHandles->m_buffer.get());

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
            size_t width = CVPixelBufferGetWidth(textureHandles->m_buffer.get());
            size_t height = CVPixelBufferGetHeight(textureHandles->m_buffer.get());
            width = textureDescription->widthForPlane(width, plane);
            height = textureDescription->heightForPlane(height, plane);

            // Tested to be valid in prior loop.
            const MTLPixelFormat metalPixelFormatForPlane =
                rhiTextureFormatToMetalFormat(textureDescription->rhiTextureFormat(plane, rhi));

            // Create a CoreVideo pixel buffer backed Metal texture image from the texture cache.
            CVMetalTextureRef outCvMetalTexRef = nullptr;
            auto ret = CVMetalTextureCacheCreateTextureFromImage(
                kCFAllocatorDefault,
                m_cvMetalTextureCache,
                textureHandles->m_buffer.get(),
                nil,
                metalPixelFormatForPlane,
                width, height,
                plane,
                &outCvMetalTexRef);
            if (ret != kCVReturnSuccess) {
                qCWarning(qLcVideotoolbox) << "Metal texture creation failed" << ret;
                return nullptr;
            }

            Q_ASSERT(outCvMetalTexRef);
            textureHandles->m_cvMetalTexture[plane] = QCFType<CVMetalTextureRef>{ outCvMetalTexRef };

        }
    } else if (rhi->backend() == QRhi::OpenGLES2) {
#if QT_CONFIG(opengl)
#ifdef Q_OS_MACOS
        CVOpenGLTextureCacheFlush(m_cvOpenGLTextureCache, 0);
        CVOpenGLTextureRef outCvGlTexRef = nullptr;
        // Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
        const CVReturn cvret = CVOpenGLTextureCacheCreateTextureFromImage(
            kCFAllocatorDefault,
            m_cvOpenGLTextureCache,
            textureHandles->m_buffer.get(),
            nil,
            &outCvGlTexRef);
        if (cvret != kCVReturnSuccess) {
            qCWarning(qLcVideotoolbox) << "OpenGL texture creation failed" << cvret;
            return nullptr;
        }

        Q_ASSERT(outCvGlTexRef);
        Q_ASSERT(CVOpenGLTextureGetTarget(outCvGlTexRef) == GL_TEXTURE_RECTANGLE);
        textureHandles->m_cvOpenGLTexture = QCFType<CVOpenGLTextureRef>{ outCvGlTexRef };

#endif
#ifdef Q_OS_IOS
        CVOpenGLESTextureCacheFlush(m_cvOpenGLESTextureCache, 0);
        CVOpenGLESTextureRef outCvGlTexRef = nullptr;
        // Create a CVPixelBuffer-backed OpenGL texture image from the texture cache.
        const CVReturn cvret = CVOpenGLESTextureCacheCreateTextureFromImage(
            kCFAllocatorDefault,
            m_cvOpenGLESTextureCache,
            textureHandles->m_buffer.get(),
            nil,
            GL_TEXTURE_2D,
            GL_RGBA,
            CVPixelBufferGetWidth(textureHandles->m_buffer.get()),
            CVPixelBufferGetHeight(textureHandles->m_buffer.get()),
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            0,
            &outCvGlTexRef);
        if (cvret != kCVReturnSuccess) {
            qCWarning(qLcVideotoolbox) << "OpenGL ES texture creation failed" << cvret;
            return nullptr;
        }

        Q_ASSERT(outCvGlTexRef);
        textureHandles->m_cvOpenGLESTexture = QCFType<CVOpenGLESTextureRef>{ outCvGlTexRef };

#endif
#endif
    }

    return textureHandles;
}

VideoToolBoxTextureHandles::~VideoToolBoxTextureHandles()
{
}

quint64 VideoToolBoxTextureHandles::textureHandle(QRhi &, int plane)
{
    if (rhi->backend() == QRhi::Metal)
        return m_cvMetalTexture[plane] ? qint64(CVMetalTextureGetTexture(m_cvMetalTexture[plane])) : 0;
#if QT_CONFIG(opengl)
    Q_ASSERT(plane == 0);
#ifdef Q_OS_MACOS
    return CVOpenGLTextureGetName(m_cvOpenGLTexture);
#endif
#ifdef Q_OS_IOS
    return CVOpenGLESTextureGetName(m_cvOpenGLESTexture);
#endif
#endif
    return 0;
}

}

QT_END_NAMESPACE
