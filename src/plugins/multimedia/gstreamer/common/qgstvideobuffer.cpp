// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstvideobuffer_p.h"
#include "qgstreamervideosink_p.h"
#include <private/qvideotexturehelper_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <qguiapplication.h>

#include <gst/video/video.h>
#include <gst/video/video-frame.h>
#include <gst/video/gstvideometa.h>
#include <gst/pbutils/gstpluginsbaseversion.h>

#include "qgstutils_p.h"

#if QT_CONFIG(gstreamer_gl)
#include <rhi/qrhi.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qopengl.h>

#include <gst/gl/gstglconfig.h>
#include <gst/gl/gstglmemory.h>
#include <gst/gl/gstglsyncmeta.h>
#if QT_CONFIG(linux_dmabuf)
#include <gst/allocators/gstdmabuf.h>
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

QT_BEGIN_NAMESPACE

// keep things building without drm_fourcc.h
#define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | \
                                 ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define DRM_FORMAT_RGBA8888     fourcc_code('R', 'A', '2', '4') /* [31:0] R:G:B:A 8:8:8:8 little endian */
#define DRM_FORMAT_RGB888       fourcc_code('R', 'G', '2', '4') /* [23:0] R:G:B little endian */
#define DRM_FORMAT_RG88         fourcc_code('R', 'G', '8', '8') /* [15:0] R:G 8:8 little endian */
#define DRM_FORMAT_ABGR8888     fourcc_code('A', 'B', '2', '4') /* [31:0] A:B:G:R 8:8:8:8 little endian */
#define DRM_FORMAT_BGR888       fourcc_code('B', 'G', '2', '4') /* [23:0] B:G:R little endian */
#define DRM_FORMAT_GR88         fourcc_code('G', 'R', '8', '8') /* [15:0] G:R 8:8 little endian */
#define DRM_FORMAT_R8           fourcc_code('R', '8', ' ', ' ') /* [7:0] R */
#define DRM_FORMAT_R16          fourcc_code('R', '1', '6', ' ') /* [15:0] R little endian */
#define DRM_FORMAT_RGB565       fourcc_code('R', 'G', '1', '6') /* [15:0] R:G:B 5:6:5 little endian */
#define DRM_FORMAT_RG1616       fourcc_code('R', 'G', '3', '2') /* [31:0] R:G 16:16 little endian */
#define DRM_FORMAT_GR1616       fourcc_code('G', 'R', '3', '2') /* [31:0] G:R 16:16 little endian */
#define DRM_FORMAT_BGRA1010102  fourcc_code('B', 'A', '3', '0') /* [31:0] B:G:R:A 10:10:10:2 little endian */

QGstVideoBuffer::QGstVideoBuffer(GstBuffer *buffer, const GstVideoInfo &info, QGstreamerVideoSink *sink,
                                 const QVideoFrameFormat &frameFormat,
                                 QGstCaps::MemoryFormat format)
    : QAbstractVideoBuffer((sink && sink->rhi() && format != QGstCaps::CpuMemory) ?
                            QVideoFrame::RhiTextureHandle : QVideoFrame::NoHandle, sink ? sink->rhi() : nullptr)
    , memoryFormat(format)
    , m_frameFormat(frameFormat)
    , m_rhi(sink ? sink->rhi() : nullptr)
    , m_videoInfo(info)
    , m_buffer(buffer)
{
    gst_buffer_ref(m_buffer);
    if (sink) {
        eglDisplay =  sink->eglDisplay();
        eglImageTargetTexture2D = sink->eglImageTargetTexture2D();
    }
}

QGstVideoBuffer::~QGstVideoBuffer()
{
    unmap();

    gst_buffer_unref(m_buffer);
}


QVideoFrame::MapMode QGstVideoBuffer::mapMode() const
{
    return m_mode;
}

QAbstractVideoBuffer::MapData QGstVideoBuffer::map(QVideoFrame::MapMode mode)
{
    const GstMapFlags flags = GstMapFlags(((mode & QVideoFrame::ReadOnly) ? GST_MAP_READ : 0)
                | ((mode & QVideoFrame::WriteOnly) ? GST_MAP_WRITE : 0));

    MapData mapData;
    if (mode == QVideoFrame::NotMapped || m_mode != QVideoFrame::NotMapped)
        return mapData;

    if (m_videoInfo.finfo->n_planes == 0) {         // Encoded
        if (gst_buffer_map(m_buffer, &m_frame.map[0], flags)) {
            mapData.nPlanes = 1;
            mapData.bytesPerLine[0] = -1;
            mapData.size[0] = m_frame.map[0].size;
            mapData.data[0] = static_cast<uchar *>(m_frame.map[0].data);

            m_mode = mode;
        }
    } else if (gst_video_frame_map(&m_frame, &m_videoInfo, m_buffer, flags)) {
        mapData.nPlanes = GST_VIDEO_FRAME_N_PLANES(&m_frame);

        for (guint i = 0; i < GST_VIDEO_FRAME_N_PLANES(&m_frame); ++i) {
            mapData.bytesPerLine[i] = GST_VIDEO_FRAME_PLANE_STRIDE(&m_frame, i);
            mapData.data[i] = static_cast<uchar *>(GST_VIDEO_FRAME_PLANE_DATA(&m_frame, i));
            mapData.size[i] = mapData.bytesPerLine[i]*GST_VIDEO_FRAME_COMP_HEIGHT(&m_frame, i);
        }

        m_mode = mode;
    }
    return mapData;
}

void QGstVideoBuffer::unmap()
{
    if (m_mode != QVideoFrame::NotMapped) {
        if (m_videoInfo.finfo->n_planes == 0)
            gst_buffer_unmap(m_buffer, &m_frame.map[0]);
        else
            gst_video_frame_unmap(&m_frame);
    }
    m_mode = QVideoFrame::NotMapped;
}

#if QT_CONFIG(gstreamer_gl) && QT_CONFIG(linux_dmabuf)
static int
fourccFromVideoInfo(const GstVideoInfo * info, int plane)
{
    GstVideoFormat format = GST_VIDEO_INFO_FORMAT (info);
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    const gint rgba_fourcc = DRM_FORMAT_ABGR8888;
    const gint rgb_fourcc = DRM_FORMAT_BGR888;
    const gint rg_fourcc = DRM_FORMAT_GR88;
#else
    const gint rgba_fourcc = DRM_FORMAT_RGBA8888;
    const gint rgb_fourcc = DRM_FORMAT_RGB888;
    const gint rg_fourcc = DRM_FORMAT_RG88;
#endif

    GST_DEBUG ("Getting DRM fourcc for %s plane %i",
              gst_video_format_to_string (format), plane);

    switch (format) {
    case GST_VIDEO_FORMAT_RGB16:
    case GST_VIDEO_FORMAT_BGR16:
        return DRM_FORMAT_RGB565;

    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
        return rgb_fourcc;

    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_BGRA:
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ABGR:
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_AYUV:
#if GST_CHECK_PLUGINS_BASE_VERSION(1,16,0)
    case GST_VIDEO_FORMAT_VUYA:
#endif
        return rgba_fourcc;

    case GST_VIDEO_FORMAT_GRAY8:
        return DRM_FORMAT_R8;

    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_UYVY:
    case GST_VIDEO_FORMAT_GRAY16_LE:
    case GST_VIDEO_FORMAT_GRAY16_BE:
        return rg_fourcc;

    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV21:
        return plane == 0 ? DRM_FORMAT_R8 : rg_fourcc;

    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
    case GST_VIDEO_FORMAT_Y41B:
    case GST_VIDEO_FORMAT_Y42B:
    case GST_VIDEO_FORMAT_Y444:
        return DRM_FORMAT_R8;

#if GST_CHECK_PLUGINS_BASE_VERSION(1,16,0)
    case GST_VIDEO_FORMAT_BGR10A2_LE:
        return DRM_FORMAT_BGRA1010102;
#endif

//    case GST_VIDEO_FORMAT_RGB10A2_LE:
//        return DRM_FORMAT_RGBA1010102;

    case GST_VIDEO_FORMAT_P010_10LE:
//    case GST_VIDEO_FORMAT_P012_LE:
//    case GST_VIDEO_FORMAT_P016_LE:
        return plane == 0 ? DRM_FORMAT_R16 : DRM_FORMAT_GR1616;

    case GST_VIDEO_FORMAT_P010_10BE:
//    case GST_VIDEO_FORMAT_P012_BE:
//    case GST_VIDEO_FORMAT_P016_BE:
        return plane == 0 ? DRM_FORMAT_R16 : DRM_FORMAT_RG1616;

    default:
        GST_ERROR ("Unsupported format for DMABuf.");
        return -1;
    }
}
#endif

#if QT_CONFIG(gstreamer_gl)
struct GlTextures
{
    uint count = 0;
    bool owned = false;
    std::array<guint32, QVideoTextureHelper::TextureDescription::maxPlanes> names;
};

class QGstQVideoFrameTextures : public QVideoFrameTextures
{
public:
    QGstQVideoFrameTextures(QRhi *rhi, QSize size, QVideoFrameFormat::PixelFormat format, GlTextures &textures)
        : m_rhi(rhi)
        , m_glTextures(textures)
    {
        auto desc = QVideoTextureHelper::textureDescription(format);
        for (uint i = 0; i < textures.count; ++i) {
            QSize planeSize(desc->widthForPlane(size.width(), int(i)),
                            desc->heightForPlane(size.height(), int(i)));
            m_textures[i].reset(rhi->newTexture(desc->textureFormat[i], planeSize, 1, {}));
            m_textures[i]->createFrom({textures.names[i], 0});
        }
    }

    ~QGstQVideoFrameTextures()
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


static GlTextures mapFromGlTexture(GstBuffer *buffer, GstVideoFrame &frame, GstVideoInfo &videoInfo)
{
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

#if GST_GL_HAVE_PLATFORM_EGL && QT_CONFIG(linux_dmabuf)
static GlTextures mapFromDmaBuffer(QRhi *rhi, GstBuffer *buffer, GstVideoFrame &frame,
                                   GstVideoInfo &videoInfo, Qt::HANDLE eglDisplay,
                                   QFunctionPointer eglImageTargetTexture2D)
{
    Q_ASSERT(gst_is_dmabuf_memory(gst_buffer_peek_memory(buffer, 0)));
    Q_ASSERT(eglDisplay);
    Q_ASSERT(eglImageTargetTexture2D);

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
    auto glContext = nativeHandles->context;
    if (!glContext) {
        qWarning() << "no GL context";
        return {};
    }

    if (!gst_video_frame_map(&frame, &videoInfo, buffer, GstMapFlags(GST_MAP_READ))) {
        qDebug() << "Couldn't map DMA video frame";
        return {};
    }

    GlTextures textures = {};
    textures.owned = true;
    textures.count = GST_VIDEO_FRAME_N_PLANES(&frame);
    //        int width = GST_VIDEO_FRAME_WIDTH(&frame);
    //        int height = GST_VIDEO_FRAME_HEIGHT(&frame);
    Q_ASSERT(GST_VIDEO_FRAME_N_PLANES(&frame) == gst_buffer_n_memory(buffer));

    QOpenGLFunctions functions(glContext);
    functions.glGenTextures(int(textures.count), textures.names.data());

    //        qDebug() << Qt::hex << "glGenTextures: glerror" << glGetError() << "egl error" << eglGetError();
    //        qDebug() << "converting DMA buffer nPlanes=" << nPlanes << m_textures[0] << m_textures[1] << m_textures[2];

    for (int i = 0; i < int(textures.count); ++i) {
        auto offset = GST_VIDEO_FRAME_PLANE_OFFSET(&frame, i);
        auto stride = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, i);
        int planeWidth = GST_VIDEO_FRAME_COMP_WIDTH(&frame, i);
        int planeHeight = GST_VIDEO_FRAME_COMP_HEIGHT(&frame, i);
        auto mem = gst_buffer_peek_memory(buffer, i);
        int fd = gst_dmabuf_memory_get_fd(mem);

        //            qDebug() << "    plane" << i << "size" << width << height << "stride" << stride << "offset" << offset << "fd=" << fd;
        // ### do we need to open/close the fd?
        // ### can we convert several planes at once?
        // Get the correct DRM_FORMATs from the texture format in the description
        EGLAttrib const attribute_list[] = {
            EGL_WIDTH, planeWidth,
            EGL_HEIGHT, planeHeight,
            EGL_LINUX_DRM_FOURCC_EXT, fourccFromVideoInfo(&videoInfo, i),
            EGL_DMA_BUF_PLANE0_FD_EXT, fd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLAttrib)offset,
            EGL_DMA_BUF_PLANE0_PITCH_EXT, stride,
            EGL_NONE
        };
        EGLImage image = eglCreateImage(eglDisplay,
                                        EGL_NO_CONTEXT,
                                        EGL_LINUX_DMA_BUF_EXT,
                                        nullptr,
                                        attribute_list);
        if (image == EGL_NO_IMAGE_KHR) {
            qWarning() << "could not create EGL image for plane" << i << Qt::hex << eglGetError();
        }
        //            qDebug() << Qt::hex << "eglCreateImage: glerror" << glGetError() << "egl error" << eglGetError();
        functions.glBindTexture(GL_TEXTURE_2D, textures.names[i]);
        //            qDebug() << Qt::hex << "bind texture: glerror" << glGetError() << "egl error" << eglGetError();
        auto EGLImageTargetTexture2D = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglImageTargetTexture2D;
        EGLImageTargetTexture2D(GL_TEXTURE_2D, image);
        //            qDebug() << Qt::hex << "glerror" << glGetError() << "egl error" << eglGetError();
        eglDestroyImage(eglDisplay, image);
    }
    gst_video_frame_unmap(&frame);

    return textures;
}
#endif
#endif

std::unique_ptr<QVideoFrameTextures> QGstVideoBuffer::mapTextures(QRhi *rhi)
{
    if (!rhi)
        return {};

#if QT_CONFIG(gstreamer_gl)
    GlTextures textures = {};
    if (memoryFormat == QGstCaps::GLTexture) {
        textures = mapFromGlTexture(m_buffer, m_frame, m_videoInfo);
    }
#if GST_GL_HAVE_PLATFORM_EGL && QT_CONFIG(linux_dmabuf)
    else if (memoryFormat == QGstCaps::DMABuf) {
        textures = mapFromDmaBuffer(m_rhi, m_buffer, m_frame, m_videoInfo, eglDisplay, eglImageTargetTexture2D);
    }
#endif
    if (textures.count > 0)
        return std::make_unique<QGstQVideoFrameTextures>(rhi, QSize{m_videoInfo.width, m_videoInfo.height},
                                                         m_frameFormat.pixelFormat(), textures);
#endif
    return {};
}

QT_END_NAMESPACE
