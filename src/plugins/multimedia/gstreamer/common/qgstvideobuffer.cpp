// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstvideobuffer_p.h"
#include <private/qvideotexturehelper_p.h>
#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtCore/qloggingcategory.h>

#include <gst/video/video.h>
#include <gst/video/video-frame.h>
#include <gst/video/gstvideometa.h>
#include <gst/pbutils/gstpluginsbaseversion.h>

#include <common/qgstutils_p.h>

#include <optional>

#if QT_CONFIG(gstreamer_gl)
#  include <QtGui/rhi/qrhi.h>
#  include <QtGui/qopenglcontext.h>
#  include <QtGui/qopenglfunctions.h>
#  include <QtGui/qopengl.h>

#  include <gst/gl/gstglconfig.h>
#  include <gst/gl/gstglmemory.h>
#  include <gst/gl/gstglsyncmeta.h>

#  if QT_CONFIG(gstreamer_gl_egl)
#    include <EGL/egl.h>
#    include <EGL/eglext.h>
#  endif

#  if QT_CONFIG(gstreamer_gl_egl) && QT_CONFIG(linux_dmabuf)
#    include <common/qgstreameregldisplay_p.h>
#    include <gst/allocators/gstdmabuf.h>
#  endif
#endif

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcGstVideoBuffer, "qt.multimedia.gstreamer.videobuffer");

QGstVideoBuffer::QGstVideoBuffer(QGstBufferHandle buffer, const QGstVideoInfo &videoInfo,
                                 const QVideoFrameFormat &frameFormat)
    : QHwVideoBuffer(QVideoFrame::NoHandle),
      m_memoryFormat(buffer.isValid() ? qMemoryFormatFromGstBuffer(buffer.get())
                                      : QGstCaps::CpuMemory),
      m_frameFormat(frameFormat),
      m_videoInfo(videoInfo),
      m_buffer(std::move(buffer))
{
    m_type = m_memoryFormat != QGstCaps::CpuMemory ? QVideoFrame::RhiTextureHandle
                                                   : QVideoFrame::NoHandle;
}

QGstVideoBuffer::~QGstVideoBuffer()
{
    Q_ASSERT(m_mode == QVideoFrame::NotMapped);
}

QAbstractVideoBuffer::MapData QGstVideoBuffer::map(QVideoFrame::MapMode mode)
{
    const GstMapFlags flags = GstMapFlags(((mode & QVideoFrame::ReadOnly) ? GST_MAP_READ : 0)
                | ((mode & QVideoFrame::WriteOnly) ? GST_MAP_WRITE : 0));

    MapData mapData;
    if (mode == QVideoFrame::NotMapped || m_mode != QVideoFrame::NotMapped)
        return mapData;

    const GstVideoInfo &gstVideoInfo = m_videoInfo.gstVideoInfo;
    if (!gstVideoInfo.finfo || gstVideoInfo.finfo->n_planes == 0) { // Encoded
        if (gst_buffer_map(m_buffer.get(), &m_frame.map[0], flags)) {
            mapData.planeCount = 1;
            mapData.bytesPerLine[0] = -1;
            mapData.dataSize[0] = m_frame.map[0].size;
            mapData.data[0] = static_cast<uchar *>(m_frame.map[0].data);

            m_mode = mode;
        }
    } else if (gst_video_frame_map(&m_frame, &gstVideoInfo, m_buffer.get(), flags)) {
        mapData.planeCount = GST_VIDEO_FRAME_N_PLANES(&m_frame);

        for (guint i = 0; i < GST_VIDEO_FRAME_N_PLANES(&m_frame); ++i) {
            mapData.bytesPerLine[i] = GST_VIDEO_FRAME_PLANE_STRIDE(&m_frame, i);
            mapData.data[i] = static_cast<uchar *>(GST_VIDEO_FRAME_PLANE_DATA(&m_frame, i));
            mapData.dataSize[i] = mapData.bytesPerLine[i]*GST_VIDEO_FRAME_COMP_HEIGHT(&m_frame, i);
        }

        m_mode = mode;
    }
    return mapData;
}

void QGstVideoBuffer::unmap()
{
    if (m_mode != QVideoFrame::NotMapped) {
        if (!m_videoInfo.gstVideoInfo.finfo || m_videoInfo.gstVideoInfo.finfo->n_planes == 0)
            gst_buffer_unmap(m_buffer.get(), &m_frame.map[0]);
        else
            gst_video_frame_unmap(&m_frame);
    }
    m_mode = QVideoFrame::NotMapped;
}

bool QGstVideoBuffer::isDmaBuf() const
{
    return m_memoryFormat == QGstCaps::DMABuf;
}

#if QT_CONFIG(gstreamer_gl_egl) && QT_CONFIG(linux_dmabuf)

using QtMultimediaPrivate::DRMFormat;

static std::optional<DRMFormat>
fourccFromGstVideoFormat(const GstVideoFormat format, int plane, bool singleEGLImage)
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    constexpr DRMFormat argb_fourcc = DRMFormat::ARGB8888;
    constexpr DRMFormat rgba_fourcc = DRMFormat::ABGR8888;
    constexpr DRMFormat rgb_fourcc = DRMFormat::BGR888;
    constexpr DRMFormat rg_fourcc = DRMFormat::GR88;
#else
    constexpr DRMFormat argb_fourcc = DRMFormat::BGRA8888;
    constexpr DRMFormat rgba_fourcc = DRMFormat::RGBA8888;
    constexpr DRMFormat rgb_fourcc = DRMFormat::RGB888;
    constexpr DRMFormat rg_fourcc = DRMFormat::RG88;
#endif

    switch (format) {
    case GST_VIDEO_FORMAT_RGB16:
    case GST_VIDEO_FORMAT_BGR16:
        return DRMFormat::RGB565;

    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
        return rgb_fourcc;

    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
        return argb_fourcc;

    case GST_VIDEO_FORMAT_AYUV:
        if (singleEGLImage) return DRMFormat::AYUV;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ABGR:
    case GST_VIDEO_FORMAT_xBGR:
        return rgba_fourcc;

    case GST_VIDEO_FORMAT_GRAY8:
        return DRMFormat::R8;

    case GST_VIDEO_FORMAT_YUY2:
        return DRMFormat::YUYV;

    case GST_VIDEO_FORMAT_UYVY:
        return DRMFormat::UYVY;

    case GST_VIDEO_FORMAT_GRAY16_LE:
    case GST_VIDEO_FORMAT_GRAY16_BE:
        if (singleEGLImage) return DRMFormat::R16;
        return rg_fourcc;

    case GST_VIDEO_FORMAT_NV12:
        if (singleEGLImage) return DRMFormat::NV12;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_NV21:
        if (singleEGLImage) return DRMFormat::NV21;
        return plane == 0 ? DRMFormat::R8 : rg_fourcc;

    case GST_VIDEO_FORMAT_I420:
        if (singleEGLImage) return DRMFormat::YUV420;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_YV12:
        if (singleEGLImage) return DRMFormat::YVU420;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_Y41B:
        if (singleEGLImage) return DRMFormat::YUV411;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_Y42B:
        if (singleEGLImage) return DRMFormat::YUV422;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_Y444:
        if (singleEGLImage) return DRMFormat::YUV444;
        return DRMFormat::R8;

    case GST_VIDEO_FORMAT_BGR10A2_LE:
        return DRMFormat::BGRA1010102;

    case GST_VIDEO_FORMAT_P010_10LE:
    case GST_VIDEO_FORMAT_P010_10BE:
        if (singleEGLImage) return DRMFormat::P010;
        return plane == 0 ? DRMFormat::R16 : DRMFormat::RG1616;

    default:
        return std::nullopt;
    }
}

static void logGlAndEglErrors(const char *context)
{
    if (!qLcGstVideoBuffer().isDebugEnabled())
        return;

    const GLenum glError = glGetError();
    const EGLint eglError = eglGetError();
    if (glError == GL_NO_ERROR && eglError == EGL_SUCCESS)
        return;

    qCDebug(qLcGstVideoBuffer).nospace()
            << context << ": GL error 0x" << Qt::hex << glError
            << ", EGL error 0x" << eglError;
}
#endif

#if QT_CONFIG(gstreamer_gl)
struct GlTextures
{
    uint count = 0;
    bool owned = false;
    std::array<guint32, QVideoTextureHelper::TextureDescription::maxPlanes> names{};
};

class QGstQVideoFrameTextures : public QVideoFrameTextures
{
public:
    QGstQVideoFrameTextures(QRhi *rhi,
                            QSize size,
                            QVideoFrameFormat::PixelFormat format,
                            GlTextures &textures,
                            QGstCaps::MemoryFormat memoryFormat)
        : m_rhi(rhi)
        , m_glTextures(textures)
    {
        QRhiTexture::Flags textureFlags = {};
        if (QVideoTextureHelper::forceGlTextureExternalOesIsSet()
            && m_rhi && rhi->backend() == QRhi::OpenGLES2)
            textureFlags = {QRhiTexture::ExternalOES};

        bool isDmaBuf = memoryFormat == QGstCaps::DMABuf;
        auto fallbackPolicy = isDmaBuf
                ? QVideoTextureHelper::TextureDescription::FallbackPolicy::Disable
                : QVideoTextureHelper::TextureDescription::FallbackPolicy::Enable;

        auto desc = QVideoTextureHelper::textureDescription(format);
        for (uint i = 0; i < textures.count; ++i) {
            // Pass nullptr to rhiPlaneSize to disable fallback in its call to rhiTextureFormat
            QSize planeSize = desc->rhiPlaneSize(size, i, isDmaBuf ? nullptr : m_rhi);
            QRhiTexture::Format format = desc->rhiTextureFormat(i, m_rhi, fallbackPolicy);
            m_textures[i].reset(rhi->newTexture(format, planeSize, 1, textureFlags));
            m_textures[i]->createFrom({textures.names[i], 0});
        }
    }

    ~QGstQVideoFrameTextures() override
    {
        m_rhi->makeThreadLocalNativeContextCurrent();
        auto ctx = QOpenGLContext::currentContext();
        if (m_glTextures.owned && ctx)
            ctx->functions()->glDeleteTextures(int(m_glTextures.count), m_glTextures.names.data());
    }

    QRhiTexture *texture(uint plane) const override
    {
        return plane < m_glTextures.count ? m_textures[plane].get() : nullptr;
    }

private:
    QRhi *m_rhi = nullptr;
    GlTextures m_glTextures;
    std::unique_ptr<QRhiTexture> m_textures[QVideoTextureHelper::TextureDescription::maxPlanes];
};

static GlTextures mapFromGlTexture(const QGstBufferHandle &bufferHandle, GstVideoFrame &frame,
                                   GstVideoInfo &videoInfo)
{
    qCDebug(qLcGstVideoBuffer) << "Mapping textures from GL memory";

    GstBuffer *buffer = bufferHandle.get();
    auto *mem = GST_GL_BASE_MEMORY_CAST(gst_buffer_peek_memory(buffer, 0));
    if (!mem)
        return {};

    if (!gst_video_frame_map(&frame, &videoInfo, buffer, GstMapFlags(GST_MAP_READ|GST_MAP_GL))) {
        qWarning() << "Could not map GL textures";
        return {};
    }

    auto *sync_meta = gst_buffer_get_gl_sync_meta(buffer);
    GstBuffer *sync_buffer = nullptr;
    if (!sync_meta) {
        sync_buffer = gst_buffer_new();
        sync_meta = gst_buffer_add_gl_sync_meta(mem->context, sync_buffer);
    }
    gst_gl_sync_meta_set_sync_point (sync_meta, mem->context);
    gst_gl_sync_meta_wait (sync_meta, mem->context);
    if (sync_buffer)
        gst_buffer_unref(sync_buffer);

    GlTextures textures;
    textures.count = frame.info.finfo->n_planes;

    for (uint i = 0; i < textures.count; ++i)
        textures.names[i] = *(guint32 *)frame.data[i];

    gst_video_frame_unmap(&frame);

    return textures;
}

#  if QT_CONFIG(gstreamer_gl_egl) && QT_CONFIG(linux_dmabuf)
static GlTextures mapFromDmaBuffer(QRhi *rhi, const QGstBufferHandle &bufferHandle,
                                   const QGstVideoInfo &videoInfo, Qt::HANDLE eglDisplay,
                                   QFunctionPointer eglImageTargetTexture2D)
{
    qCDebug(qLcGstVideoBuffer) << "Importing textures from DMA buffer";
    logGlAndEglErrors("mapFromDmaBuffer");

    GstBuffer *buffer = bufferHandle.get();

    Q_ASSERT(gst_is_dmabuf_memory(gst_buffer_peek_memory(buffer, 0)));
    Q_ASSERT(eglDisplay);
    Q_ASSERT(eglImageTargetTexture2D);
    Q_ASSERT(rhi);
    Q_ASSERT(rhi->backend() == QRhi::OpenGLES2);

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
    auto glContext = nativeHandles->context;
    if (!glContext) {
        qCWarning(qLcGstVideoBuffer) << "no GL context";
        return {};
    }

    const GstVideoInfo &gstVideoInfo = videoInfo.gstVideoInfo;
    if (!gstVideoInfo.finfo) {
        qCWarning(qLcGstVideoBuffer) << "Missing valid GstVideoInfo for DMABuf GstBuffer";
        return {};
    }

    if (videoInfo.dmaDrmModifier && *videoInfo.dmaDrmModifier != 0) {
        qCWarning(qLcGstVideoBuffer) << "Unsupported non-linear DMABuf modifier:"
                                     << Qt::hex << *videoInfo.dmaDrmModifier;
        return {};
    }

    const GstVideoMeta *videoMeta = gst_buffer_get_video_meta(buffer);
    const GstVideoFormat videoInfoFormat = GST_VIDEO_INFO_FORMAT(&gstVideoInfo);
    GstVideoFormat format = videoMeta ? videoMeta->format : videoInfoFormat;
    if (format == GST_VIDEO_FORMAT_UNKNOWN)
        format = videoInfoFormat;

    const int nPlanes = videoMeta ? videoMeta->n_planes : GST_VIDEO_INFO_N_PLANES(&gstVideoInfo);
    const int nMemoryBlocks = gst_buffer_n_memory(buffer);
    static const bool externalOes = QVideoTextureHelper::forceGlTextureExternalOesIsSet();
    static const bool singleEGLImage =
            externalOes || qEnvironmentVariableIntValue("QT_GSTREAMER_FORCE_SINGLE_EGLIMAGE") != 0;

    qCDebug(qLcGstVideoBuffer) << "format:" << gst_video_format_to_string(format)
                               << "nPlanes:" << nPlanes
                               << "nMemoryBlocks:" << nMemoryBlocks
                               << "externalOes:" << externalOes
                               << "singleEGLImage:" << singleEGLImage;

    constexpr int maxPlanes = 4;
    Q_ASSERT(nPlanes >= 1
             && nPlanes <= maxPlanes
             && (nMemoryBlocks == 1 || nMemoryBlocks == nPlanes));

    const int nEGLImages = singleEGLImage ? 1 : nPlanes;
    std::array<EGLAttrib, maxPlanes> planeFourcc{};
    for (int plane = 0; plane < nEGLImages; ++plane) {
        const std::optional<DRMFormat> fourcc = fourccFromGstVideoFormat(format, plane, singleEGLImage);
        if (!fourcc) {
            qCWarning(qLcGstVideoBuffer) << "Unsupported format for DMABuf:"
                                         << gst_video_format_to_string(format) << "plane:" << plane
                                         << "singleEGLImage" << singleEGLImage;
            return {};
        }
        planeFourcc[plane] = EGLAttrib(quint32(*fourcc));
    }

    GlTextures textures = {};
    textures.owned = true;
    textures.count = nEGLImages;

    QOpenGLFunctions functions(glContext);
    functions.glGenTextures(int(textures.count), textures.names.data());
    logGlAndEglErrors("glGenTextures");

    std::array<int, maxPlanes> fds{-1, -1, -1, -1};
    for (int i = 0; i < nMemoryBlocks && i < maxPlanes; ++i) {
        fds[i] = gst_dmabuf_memory_get_fd(gst_buffer_peek_memory(buffer, i));
    }

    auto fdForPlane = [&](int plane) -> EGLAttrib {
        if (plane < 0 || plane >= maxPlanes || plane >= nMemoryBlocks)
            return fds[0];
        return (fds[plane] >= 0) ? fds[plane] : fds[0];
    };

    auto compWidth = [&](int plane) -> EGLAttrib {
        return singleEGLImage ? GST_VIDEO_INFO_WIDTH(&gstVideoInfo)
                              : GST_VIDEO_INFO_COMP_WIDTH(&gstVideoInfo, plane);
    };

    auto compHeight = [&](int plane) -> EGLAttrib {
        return singleEGLImage ? GST_VIDEO_INFO_HEIGHT(&gstVideoInfo)
                              : GST_VIDEO_INFO_COMP_HEIGHT(&gstVideoInfo, plane);
    };

    auto planeOffset = [&](int plane) -> EGLAttrib {
        // videoMeta/videoInfo offset can be incorrect when each plane has a separate memory black.
        if (nPlanes == nMemoryBlocks)
            return 0;
        if (videoMeta)
            return videoMeta->offset[plane];
        return GST_VIDEO_INFO_PLANE_OFFSET(&gstVideoInfo, plane);
    };

    auto planeStride = [&](int plane) -> EGLAttrib {
        if (videoMeta)
            return videoMeta->stride[plane];
        return GST_VIDEO_INFO_PLANE_STRIDE(&gstVideoInfo, plane);
    };

    for (int plane = 0; plane < nEGLImages; ++plane) {
        constexpr int maxAttrCount = 31;
        std::array<EGLAttrib, maxAttrCount> attr;
        int i = 0;

        const int width = compWidth(plane);
        const int height = compHeight(plane);

        attr[i++] = EGL_WIDTH;
        attr[i++] = width;
        attr[i++] = EGL_HEIGHT;
        attr[i++] = height;
        attr[i++] = EGL_LINUX_DRM_FOURCC_EXT;
        attr[i++] = planeFourcc[plane];

        attr[i++] = EGL_DMA_BUF_PLANE0_FD_EXT;
        attr[i++] = fdForPlane(plane);
        attr[i++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
        attr[i++] = planeOffset(plane);
        attr[i++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
        attr[i++] = planeStride(plane);

        if (singleEGLImage && nPlanes > 1) {
            attr[i++] = EGL_DMA_BUF_PLANE1_FD_EXT;
            attr[i++] = fdForPlane(1);
            attr[i++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
            attr[i++] = planeOffset(1);
            attr[i++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
            attr[i++] = planeStride(1);
        }

        if (singleEGLImage && nPlanes > 2) {
            attr[i++] = EGL_DMA_BUF_PLANE2_FD_EXT;
            attr[i++] = fdForPlane(2);
            attr[i++] = EGL_DMA_BUF_PLANE2_OFFSET_EXT;
            attr[i++] = planeOffset(2);
            attr[i++] = EGL_DMA_BUF_PLANE2_PITCH_EXT;
            attr[i++] = planeStride(2);
        }

        if (singleEGLImage && nPlanes > 3) {
            attr[i++] = EGL_DMA_BUF_PLANE3_FD_EXT;
            attr[i++] = fdForPlane(3);
            attr[i++] = EGL_DMA_BUF_PLANE3_OFFSET_EXT;
            attr[i++] = planeOffset(3);
            attr[i++] = EGL_DMA_BUF_PLANE3_PITCH_EXT;
            attr[i++] = planeStride(3);
        }

        attr[i++] = EGL_NONE;
        Q_ASSERT(i <= maxAttrCount);

        EGLImage image = eglCreateImage(eglDisplay,
                                        EGL_NO_CONTEXT,
                                        EGL_LINUX_DMA_BUF_EXT,
                                        nullptr,
                                        attr.data());
        if (image == EGL_NO_IMAGE_KHR) {
            qCWarning(qLcGstVideoBuffer) << "could not create EGL image for plane" << plane
                                         << ", EGL error 0x" << Qt::hex << eglGetError();
            continue;
        }
        logGlAndEglErrors("eglCreateImage");

        #ifdef GL_OES_EGL_image_external
                GLenum target = externalOes ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;
        #else
                GLenum target = GL_TEXTURE_2D;
        #endif
        functions.glBindTexture(target, textures.names[plane]);

        auto EGLImageTargetTexture2D = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglImageTargetTexture2D;
        EGLImageTargetTexture2D(target, image);
        logGlAndEglErrors("glEGLImageTargetTexture2DOES");

        eglDestroyImage(eglDisplay, image);
    }

    return textures;
}
#endif
#endif

QVideoFrameTexturesUPtr QGstVideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr& /*oldTextures*/)
{
#if QT_CONFIG(gstreamer_gl)
    GlTextures textures = {};
    if (m_memoryFormat == QGstCaps::GLTexture)
        textures = mapFromGlTexture(m_buffer, m_frame, m_videoInfo.gstVideoInfo);

#  if QT_CONFIG(gstreamer_gl_egl) && QT_CONFIG(linux_dmabuf)
    else if (m_memoryFormat == QGstCaps::DMABuf && qGstEglCanMapDmaBuf()
             && rhi.backend() == QRhi::OpenGLES2)
        textures = mapFromDmaBuffer(&rhi, m_buffer, m_videoInfo, qGstEglDisplay(),
                                    qGstEglImageTargetTexture2D());

#  endif
    if (textures.count > 0)
        return std::make_unique<QGstQVideoFrameTextures>(
                &rhi, QSize{ m_videoInfo.gstVideoInfo.width, m_videoInfo.gstVideoInfo.height },
                m_frameFormat.pixelFormat(), textures, m_memoryFormat);
#endif
    return {};
}

QT_END_NAMESPACE
