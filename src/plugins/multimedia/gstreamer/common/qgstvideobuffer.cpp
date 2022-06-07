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
#include <QtGui/private/qrhi_p.h>
#include <QtGui/private/qrhigles2_p.h>
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
    if (m_syncBuffer)
        gst_buffer_unref(m_syncBuffer);

    if (m_ownTextures && glContext) {
        int planes = 0;
        for (planes = 0; planes < 3; ++planes) {
            if (m_textures[planes] == 0)
                break;
        }
#if QT_CONFIG(gstreamer_gl)
        if (m_rhi) {
            m_rhi->makeThreadLocalNativeContextCurrent();
            QOpenGLFunctions functions(glContext);
            functions.glDeleteTextures(planes, m_textures);
        }
#endif
    }
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

void QGstVideoBuffer::mapTextures()
{
    if (!m_rhi)
        return;

#if QT_CONFIG(gstreamer_gl)
    if (memoryFormat == QGstCaps::GLTexture) {
        auto *mem = GST_GL_BASE_MEMORY_CAST(gst_buffer_peek_memory(m_buffer, 0));
        Q_ASSERT(mem);
        if (!gst_video_frame_map(&m_frame, &m_videoInfo, m_buffer, GstMapFlags(GST_MAP_READ|GST_MAP_GL))) {
            qWarning() << "Could not map GL textures";
        } else {
            auto *sync_meta = gst_buffer_get_gl_sync_meta(m_buffer);

            if (!sync_meta) {
                m_syncBuffer = gst_buffer_new();
                sync_meta = gst_buffer_add_gl_sync_meta(mem->context, m_syncBuffer);
            }
            gst_gl_sync_meta_set_sync_point (sync_meta, mem->context);
            gst_gl_sync_meta_wait (sync_meta, mem->context);

            int nPlanes = m_frame.info.finfo->n_planes;
            for (int i = 0; i < nPlanes; ++i) {
                m_textures[i] = *(guint32 *)m_frame.data[i];
            }
            gst_video_frame_unmap(&m_frame);
        }
    }
#if GST_GL_HAVE_PLATFORM_EGL && QT_CONFIG(linux_dmabuf)
    else if (memoryFormat == QGstCaps::DMABuf) {
        if (m_textures[0])
            return;
        Q_ASSERT(gst_is_dmabuf_memory(gst_buffer_peek_memory(m_buffer, 0)));
        Q_ASSERT(eglDisplay);
        Q_ASSERT(eglImageTargetTexture2D);

        auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(m_rhi->nativeHandles());
        glContext = nativeHandles->context;
        if (!glContext) {
            qWarning() << "no GL context";
            return;
        }

        if (!gst_video_frame_map(&m_frame, &m_videoInfo, m_buffer, GstMapFlags(GST_MAP_READ))) {
            qDebug() << "Couldn't map DMA video frame";
            return;
        }

        int nPlanes = GST_VIDEO_FRAME_N_PLANES(&m_frame);
//        int width = GST_VIDEO_FRAME_WIDTH(&m_frame);
//        int height = GST_VIDEO_FRAME_HEIGHT(&m_frame);
        Q_ASSERT(GST_VIDEO_FRAME_N_PLANES(&m_frame) == gst_buffer_n_memory(m_buffer));

        QOpenGLFunctions functions(glContext);
        functions.glGenTextures(nPlanes, m_textures);
        m_ownTextures = true;

//        qDebug() << Qt::hex << "glGenTextures: glerror" << glGetError() << "egl error" << eglGetError();
//        qDebug() << "converting DMA buffer nPlanes=" << nPlanes << m_textures[0] << m_textures[1] << m_textures[2];

        for (int i = 0; i < nPlanes; ++i) {
            auto offset = GST_VIDEO_FRAME_PLANE_OFFSET(&m_frame, i);
            auto stride = GST_VIDEO_FRAME_PLANE_STRIDE(&m_frame, i);
            int planeWidth = GST_VIDEO_FRAME_COMP_WIDTH(&m_frame, i);
            int planeHeight = GST_VIDEO_FRAME_COMP_HEIGHT(&m_frame, i);
            auto mem = gst_buffer_peek_memory(m_buffer, i);
            int fd = gst_dmabuf_memory_get_fd(mem);

//            qDebug() << "    plane" << i << "size" << width << height << "stride" << stride << "offset" << offset << "fd=" << fd;
            // ### do we need to open/close the fd?
            // ### can we convert several planes at once?
            // Get the correct DRM_FORMATs from the texture format in the description
            EGLAttrib const attribute_list[] = {
                EGL_WIDTH, planeWidth,
                EGL_HEIGHT, planeHeight,
                EGL_LINUX_DRM_FOURCC_EXT, fourccFromVideoInfo(&m_videoInfo, i),
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
            functions.glBindTexture(GL_TEXTURE_2D, m_textures[i]);
//            qDebug() << Qt::hex << "bind texture: glerror" << glGetError() << "egl error" << eglGetError();
            auto EGLImageTargetTexture2D = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglImageTargetTexture2D;
            EGLImageTargetTexture2D(GL_TEXTURE_2D, image);
//            qDebug() << Qt::hex << "glerror" << glGetError() << "egl error" << eglGetError();
            eglDestroyImage(eglDisplay, image);
        }
        gst_video_frame_unmap(&m_frame);
    }
#endif
#endif
    m_texturesUploaded = true;
}

std::unique_ptr<QRhiTexture> QGstVideoBuffer::texture(int plane) const
{
    auto desc = QVideoTextureHelper::textureDescription(m_frameFormat.pixelFormat());
    if (!m_rhi || !desc || plane >= desc->nplanes)
        return {};
    QSize size(desc->widthForPlane(m_videoInfo.width, plane), desc->heightForPlane(m_videoInfo.height, plane));
    std::unique_ptr<QRhiTexture> tex(m_rhi->newTexture(desc->textureFormat[plane], size, 1, {}));
    if (tex) {
        if (!tex->createFrom({m_textures[plane], 0}))
            return {};
    }
    return tex;
}

QT_END_NAMESPACE
