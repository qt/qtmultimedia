// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstvideobuffer_p.h"
#include "qgstreamervideosink_p.h"
#include <private/qvideotexturehelper_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <qguiapplication.h>
#include <QtCore/qloggingcategory.h>

#include <gst/video/video.h>
#include <gst/video/video-frame.h>
#include <gst/video/gstvideometa.h>
#include <gst/pbutils/gstpluginsbaseversion.h>

#include <common/qgstutils_p.h>

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
#    include <gst/allocators/gstdmabuf.h>
#  endif
#endif

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcGstVideoBuffer, "qt.multimedia.gstreamer.videobuffer");

// keep things building without drm_fourcc.h
#define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | \
                                 ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#define DRM_FORMAT_RGBA8888     fourcc_code('R', 'A', '2', '4') /* [31:0] R:G:B:A 8:8:8:8 little endian */
#define DRM_FORMAT_BGRA8888     fourcc_code('B', 'A', '2', '4') /* [31:0] B:G:R:A 8:8:8:8 little endian */
#define DRM_FORMAT_RGB888       fourcc_code('R', 'G', '2', '4') /* [23:0] R:G:B little endian */
#define DRM_FORMAT_RG88         fourcc_code('R', 'G', '8', '8') /* [15:0] R:G 8:8 little endian */
#define DRM_FORMAT_ARGB8888     fourcc_code('A', 'R', '2', '4') /* [31:0] A:R:G:B 8:8:8:8 little endian */
#define DRM_FORMAT_ABGR8888     fourcc_code('A', 'B', '2', '4') /* [31:0] A:B:G:R 8:8:8:8 little endian */
#define DRM_FORMAT_BGR888       fourcc_code('B', 'G', '2', '4') /* [23:0] B:G:R little endian */
#define DRM_FORMAT_GR88         fourcc_code('G', 'R', '8', '8') /* [15:0] G:R 8:8 little endian */
#define DRM_FORMAT_R8           fourcc_code('R', '8', ' ', ' ') /* [7:0] R */
#define DRM_FORMAT_R16          fourcc_code('R', '1', '6', ' ') /* [15:0] R little endian */
#define DRM_FORMAT_RGB565       fourcc_code('R', 'G', '1', '6') /* [15:0] R:G:B 5:6:5 little endian */
#define DRM_FORMAT_RG1616       fourcc_code('R', 'G', '3', '2') /* [31:0] R:G 16:16 little endian */
#define DRM_FORMAT_GR1616       fourcc_code('G', 'R', '3', '2') /* [31:0] G:R 16:16 little endian */
#define DRM_FORMAT_BGRA1010102  fourcc_code('B', 'A', '3', '0') /* [31:0] B:G:R:A 10:10:10:2 little endian */
#define DRM_FORMAT_YUYV         fourcc_code('Y', 'U', 'Y', 'V') /* [31:0] Cr0:Y1:Cb0:Y0 8:8:8:8 little endian */
#define DRM_FORMAT_UYVY         fourcc_code('U', 'Y', 'V', 'Y') /* [31:0] Y1:Cr0:Y0:Cb0 8:8:8:8 little endian */
#define DRM_FORMAT_AYUV         fourcc_code('A', 'Y', 'U', 'V') /* [31:0] A:Y:Cb:Cr 8:8:8:8 little endian */
#define DRM_FORMAT_NV12         fourcc_code('N', 'V', '1', '2') /* 2x2 subsampled Cr:Cb plane */
#define DRM_FORMAT_NV21         fourcc_code('N', 'V', '2', '1') /* 2x2 subsampled Cb:Cr plane */
#define DRM_FORMAT_P010         fourcc_code('P', '0', '1', '0') /* 2x2 subsampled Cr:Cb plane 10 bits per channel */
#define DRM_FORMAT_YUV411       fourcc_code('Y', 'U', '1', '1') /* 4x1 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YUV420       fourcc_code('Y', 'U', '1', '2') /* 2x2 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU420       fourcc_code('Y', 'V', '1', '2') /* 2x2 subsampled Cr (1) and Cb (2) planes */
#define DRM_FORMAT_YUV422       fourcc_code('Y', 'U', '1', '6') /* 2x1 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YUV444       fourcc_code('Y', 'U', '2', '4') /* non-subsampled Cb (1) and Cr (2) planes */

QGstVideoBuffer::QGstVideoBuffer(QGstBufferHandle buffer, const GstVideoInfo &info,
                                 QGstreamerRelayVideoSink *sink, const QVideoFrameFormat &frameFormat,
                                 QGstCaps::MemoryFormat memoryFormat)
    : QHwVideoBuffer((sink && sink->rhi() && memoryFormat != QGstCaps::CpuMemory)
                             ? QVideoFrame::RhiTextureHandle
                             : QVideoFrame::NoHandle,
                     sink ? sink->rhi() : nullptr),
      m_memoryFormat(memoryFormat),
      m_frameFormat(frameFormat),
      m_videoInfo(info),
      m_buffer(std::move(buffer))
{
#if QT_CONFIG(gstreamer_gl_egl)
    if (sink) {
        eglDisplay =  sink->eglDisplay();
        eglImageTargetTexture2D = sink->eglImageTargetTexture2D();
    }
#endif
    Q_UNUSED(m_memoryFormat);
    Q_UNUSED(eglDisplay);
    Q_UNUSED(eglImageTargetTexture2D);
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

    if (m_videoInfo.finfo->n_planes == 0) {         // Encoded
        if (gst_buffer_map(m_buffer.get(), &m_frame.map[0], flags)) {
            mapData.planeCount = 1;
            mapData.bytesPerLine[0] = -1;
            mapData.dataSize[0] = m_frame.map[0].size;
            mapData.data[0] = static_cast<uchar *>(m_frame.map[0].data);

            m_mode = mode;
        }
    } else if (gst_video_frame_map(&m_frame, &m_videoInfo, m_buffer.get(), flags)) {
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
        if (m_videoInfo.finfo->n_planes == 0)
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

static int
fourccFromVideoInfo(const GstVideoInfo * info, int plane, bool singleEGLImage)
{
    GstVideoFormat format = GST_VIDEO_INFO_FORMAT (info);
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    const gint argb_fourcc = DRM_FORMAT_ARGB8888;
    const gint rgba_fourcc = DRM_FORMAT_ABGR8888;
    const gint rgb_fourcc = DRM_FORMAT_BGR888;
    const gint rg_fourcc = DRM_FORMAT_GR88;
#else
    const gint argb_fourcc = DRM_FORMAT_BGRA8888;
    const gint rgba_fourcc = DRM_FORMAT_RGBA8888;
    const gint rgb_fourcc = DRM_FORMAT_RGB888;
    const gint rg_fourcc = DRM_FORMAT_RG88;
#endif

    qCDebug(qLcGstVideoBuffer) << "Getting DRM fourcc for"
                               << gst_video_format_to_string(format)
                               << "plane" << plane;

    switch (format) {
    case GST_VIDEO_FORMAT_RGB16:
    case GST_VIDEO_FORMAT_BGR16:
        return DRM_FORMAT_RGB565;

    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
        return rgb_fourcc;

    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
        return argb_fourcc;

    case GST_VIDEO_FORMAT_AYUV:
        if (singleEGLImage) return DRM_FORMAT_AYUV;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ABGR:
    case GST_VIDEO_FORMAT_xBGR:
        return rgba_fourcc;

    case GST_VIDEO_FORMAT_GRAY8:
        return DRM_FORMAT_R8;

    case GST_VIDEO_FORMAT_YUY2:
        return DRM_FORMAT_YUYV;

    case GST_VIDEO_FORMAT_UYVY:
        return DRM_FORMAT_UYVY;

    case GST_VIDEO_FORMAT_GRAY16_LE:
    case GST_VIDEO_FORMAT_GRAY16_BE:
        if (singleEGLImage) return DRM_FORMAT_R16;
        return rg_fourcc;

    case GST_VIDEO_FORMAT_NV12:
        if (singleEGLImage) return DRM_FORMAT_NV12;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_NV21:
        if (singleEGLImage) return DRM_FORMAT_NV21;
        return plane == 0 ? DRM_FORMAT_R8 : rg_fourcc;

    case GST_VIDEO_FORMAT_I420:
        if (singleEGLImage) return DRM_FORMAT_YUV420;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_YV12:
        if (singleEGLImage) return DRM_FORMAT_YVU420;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_Y41B:
        if (singleEGLImage) return DRM_FORMAT_YUV411;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_Y42B:
        if (singleEGLImage) return DRM_FORMAT_YUV422;
        [[fallthrough]];
    case GST_VIDEO_FORMAT_Y444:
        if (singleEGLImage) return DRM_FORMAT_YUV444;
        return DRM_FORMAT_R8;

#if GST_CHECK_PLUGINS_BASE_VERSION(1,16,0)
    case GST_VIDEO_FORMAT_BGR10A2_LE:
        return DRM_FORMAT_BGRA1010102;
#endif

    case GST_VIDEO_FORMAT_P010_10LE:
    case GST_VIDEO_FORMAT_P010_10BE:
        if (singleEGLImage) return DRM_FORMAT_P010;
        return plane == 0 ? DRM_FORMAT_R16 : DRM_FORMAT_RG1616;

    default:
        qWarning() << "Unsupported format for DMABuf:" << gst_video_format_to_string(format);
        return -1;
    }
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
    qCDebug(qLcGstVideoBuffer) << "mapFromGlTexture";

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
                                   GstVideoFrame &frame, GstVideoInfo &videoInfo,
                                   Qt::HANDLE eglDisplay, QFunctionPointer eglImageTargetTexture2D)
{
    qCDebug(qLcGstVideoBuffer) << "mapFromDmaBuffer, glGetError returns" << Qt::hex << glGetError()
                               << ", eglGetError() returns" << eglGetError();

    GstBuffer *buffer = bufferHandle.get();

    Q_ASSERT(gst_is_dmabuf_memory(gst_buffer_peek_memory(buffer, 0)));
    Q_ASSERT(eglDisplay);
    Q_ASSERT(eglImageTargetTexture2D);
    Q_ASSERT(QGuiApplication::platformName() == QLatin1String("eglfs"));
    Q_ASSERT(rhi);

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
    auto glContext = nativeHandles->context;
    if (!glContext) {
        qWarning() << "no GL context";
        return {};
    }

    if (!gst_video_frame_map(&frame, &videoInfo, buffer, GstMapFlags(GST_MAP_READ))) {
        qWarning() << "gst_video_frame_map failed, couldn't map DMA video frame";
        return {};
    }

    constexpr int maxPlanes = 4;
    const int nPlanes = GST_VIDEO_FRAME_N_PLANES(&frame);
    const int nMemoryBlocks = gst_buffer_n_memory(buffer);
    const bool externalOes =
            QVideoTextureHelper::forceGlTextureExternalOesIsSet();
    static const bool singleEGLImage =
            externalOes || qEnvironmentVariableIsSet("QT_GSTREAMER_FORCE_SINGLE_EGLIMAGE");

    qCDebug(qLcGstVideoBuffer) << "nPlanes:" << nPlanes
                               << "nMemoryBlocks:" << nMemoryBlocks
                               << "externalOes:" << externalOes
                               << "singleEGLImage:" << singleEGLImage;
    Q_ASSERT(nPlanes >= 1
             && nPlanes <= maxPlanes
             && (nMemoryBlocks == 1 || nMemoryBlocks == nPlanes));

    GlTextures textures = {};
    textures.owned = true;
    textures.count = singleEGLImage ? 1 : nPlanes;

    QOpenGLFunctions functions(glContext);
    functions.glGenTextures(int(textures.count), textures.names.data());
    qCDebug(qLcGstVideoBuffer) << "called glGenTextures, glGetError returns"
                               << Qt::hex << glGetError();

    std::array<int, maxPlanes> fds{-1, -1, -1, -1};
    for (int i = 0; i < nMemoryBlocks && i < maxPlanes; ++i) {
        fds[i] = gst_dmabuf_memory_get_fd(gst_buffer_peek_memory(buffer, i));
    }
    auto fdForPlane = [&](int plane) {
        if (plane < 0 || plane >= maxPlanes || plane >= nMemoryBlocks)
            return fds[0];
        return (fds[plane] >= 0) ? fds[plane] : fds[0];
    };

    int nEGLImages = singleEGLImage ? 1 : nPlanes;
    for (int plane = 0; plane < nEGLImages; ++plane) {
        constexpr int maxAttrCount = 31;
        std::array<EGLAttrib, maxAttrCount> attr;
        int i = 0;

        gint width = singleEGLImage ? GST_VIDEO_FRAME_WIDTH(&frame)
                                    : GST_VIDEO_FRAME_COMP_WIDTH(&frame, plane);
        gint height = singleEGLImage ? GST_VIDEO_FRAME_HEIGHT(&frame)
                                     : GST_VIDEO_FRAME_COMP_HEIGHT(&frame, plane);
        attr[i++] = EGL_WIDTH;
        attr[i++] = width;
        attr[i++] = EGL_HEIGHT;
        attr[i++] = height;
        attr[i++] = EGL_LINUX_DRM_FOURCC_EXT;
        attr[i++] = fourccFromVideoInfo(&videoInfo, plane, singleEGLImage);

        attr[i++] = EGL_DMA_BUF_PLANE0_FD_EXT;
        attr[i++] = fdForPlane(plane);
        attr[i++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
        attr[i++] = (EGLAttrib)(GST_VIDEO_FRAME_PLANE_OFFSET(&frame, plane));
        attr[i++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
        attr[i++] = (EGLAttrib)(GST_VIDEO_FRAME_PLANE_STRIDE(&frame, plane));

        if (singleEGLImage && nPlanes > 1) {
            attr[i++] = EGL_DMA_BUF_PLANE1_FD_EXT;
            attr[i++] = fdForPlane(1);
            attr[i++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
            attr[i++] = (EGLAttrib)(GST_VIDEO_FRAME_PLANE_OFFSET(&frame, 1));
            attr[i++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
            attr[i++] = (EGLAttrib)(GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 1));
        }

        if (singleEGLImage && nPlanes > 2) {
            attr[i++] = EGL_DMA_BUF_PLANE2_FD_EXT;
            attr[i++] = fdForPlane(2);
            attr[i++] = EGL_DMA_BUF_PLANE2_OFFSET_EXT;
            attr[i++] = (EGLAttrib)(GST_VIDEO_FRAME_PLANE_OFFSET(&frame, 2));
            attr[i++] = EGL_DMA_BUF_PLANE2_PITCH_EXT;
            attr[i++] = (EGLAttrib)(GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 2));
        }

        if (singleEGLImage && nPlanes > 3) {
            attr[i++] = EGL_DMA_BUF_PLANE3_FD_EXT;
            attr[i++] = fdForPlane(3);
            attr[i++] = EGL_DMA_BUF_PLANE3_OFFSET_EXT;
            attr[i++] = (EGLAttrib)(GST_VIDEO_FRAME_PLANE_OFFSET(&frame, 3));
            attr[i++] = EGL_DMA_BUF_PLANE3_PITCH_EXT;
            attr[i++] = (EGLAttrib)(GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 3));
        }

        attr[i++] = EGL_NONE;
        Q_ASSERT(i <= maxAttrCount);

        EGLImage image = eglCreateImage(eglDisplay,
                                        EGL_NO_CONTEXT,
                                        EGL_LINUX_DMA_BUF_EXT,
                                        nullptr,
                                        attr.data());
        if (image == EGL_NO_IMAGE_KHR) {
            qWarning() << "could not create EGL image for plane" << plane
                       << ", eglError"<< Qt::hex << eglGetError();
            continue;
        }
        qCDebug(qLcGstVideoBuffer) << "called eglCreateImage, glGetError returns"
                                   << Qt::hex << glGetError()
                                   << ", eglGetError() returns" << eglGetError();

        #ifdef GL_OES_EGL_image_external
                GLenum target = externalOes ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;
        #else
                GLenum target = GL_TEXTURE_2D;
        #endif
        functions.glBindTexture(target, textures.names[plane]);

        auto EGLImageTargetTexture2D = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglImageTargetTexture2D;
        EGLImageTargetTexture2D(target, image);
        qCDebug(qLcGstVideoBuffer) << "called glEGLImageTargetTexture2DOES, glGetError returns"
                                   << Qt::hex << glGetError()
                                   << ", eglGetError() returns" << eglGetError();

        eglDestroyImage(eglDisplay, image);
    }

    gst_video_frame_unmap(&frame);

    return textures;
}
#endif
#endif

QVideoFrameTexturesUPtr QGstVideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr& /*oldTextures*/)
{
#if QT_CONFIG(gstreamer_gl)
#  if QT_CONFIG(gstreamer_gl_egl) && QT_CONFIG(linux_dmabuf)
    static const bool isEglfsQPA = QGuiApplication::platformName() == QLatin1String("eglfs");
#  endif
    GlTextures textures = {};
    if (m_memoryFormat == QGstCaps::GLTexture)
        textures = mapFromGlTexture(m_buffer, m_frame, m_videoInfo);

#  if QT_CONFIG(gstreamer_gl_egl) && QT_CONFIG(linux_dmabuf)
    else if (m_memoryFormat == QGstCaps::DMABuf && eglDisplay && isEglfsQPA)
        textures = mapFromDmaBuffer(&rhi, m_buffer, m_frame, m_videoInfo, eglDisplay,
                                    eglImageTargetTexture2D);

#  endif
    if (textures.count > 0)
        return std::make_unique<QGstQVideoFrameTextures>(&rhi, QSize{m_videoInfo.width, m_videoInfo.height},
                                                         m_frameFormat.pixelFormat(), textures, m_memoryFormat);
#endif
    return {};
}

QT_END_NAMESPACE
