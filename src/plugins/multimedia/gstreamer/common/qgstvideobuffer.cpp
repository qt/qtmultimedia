// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstvideobuffer_p.h"

#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtCore/qloggingcategory.h>

#include <common/qgstutils_p.h>

#include <gst/video/video.h>
#include <gst/video/video-frame.h>
#include <gst/pbutils/gstpluginsbaseversion.h>


#if QT_CONFIG(gstreamer_gl)
#  include <QtCore/qscopeguard.h>

#  include <QtGui/rhi/qrhi.h>
#  include <QtGui/qopenglcontext.h>
#  include <QtGui/qopenglfunctions.h>
#  include <QtGui/qopengl.h>

#  include <gst/gl/gstglconfig.h>
#  include <gst/gl/gstglmemory.h>
#  include <gst/gl/gstglsyncmeta.h>
#endif

// Vivante phys-mem buffers carry no GstGL/dmabuf memory, just a physical
// address (gst_phys_memory_get_phys_addr()); unlike the paths above, this
// doesn't depend on QT_CONFIG(gstreamer_gl). The GL/EGL work happens inside
// QtMultimediaPrivate::importVivanteTexture (qvivantetextureimporter_p.h).
#if QT_CONFIG(gpu_vivante)
#  include <QtMultimedia/private/qvivantetextureimporter_p.h>
#  include <QtGui/rhi/qrhi.h>
#  include <gst/allocators/gstphysmemory.h>
#  include <optional>
#endif

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcGstVideoBuffer, "qt.multimedia.gstreamer.videobuffer")

QGstVideoBuffer::QGstVideoBuffer(QGstBufferHandle buffer, const QGstVideoInfo &videoInfo,
                                 const QVideoFrameFormat &frameFormat)
    : m_memoryFormat(buffer.isValid() ? qMemoryFormatFromGstBuffer(buffer.get())
                                      : QGstCaps::CpuMemory),
      m_frameFormat(frameFormat),
      m_videoInfo(videoInfo),
      m_buffer(std::move(buffer))
{
    if constexpr (QT_CONFIG(linux_dmabuf))
        Q_ASSERT(m_memoryFormat != QGstCaps::DMABuf
                 && "DMABuf buffers should use QGstDmaBufVideoBuffer");
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

#if QT_CONFIG(gpu_vivante)
// Maps a GstVideoFormat to the GL_VIV_direct_texture format token that tells
// glTexDirectVIVMap how to interpret/convert the underlying (possibly
// planar) memory. Returns std::nullopt for anything else, which triggers the
// CPU-copy fallback path.
std::optional<GLenum> qGstVivFormatTokenFromGstVideoFormat(GstVideoFormat format)
{
    switch (format) {
    case GST_VIDEO_FORMAT_I420:
        return GL_VIV_I420;
    case GST_VIDEO_FORMAT_YV12:
        return GL_VIV_YV12;
    case GST_VIDEO_FORMAT_NV12:
        return GL_VIV_NV12;
    case GST_VIDEO_FORMAT_NV21:
        return GL_VIV_NV21;
    case GST_VIDEO_FORMAT_YUY2:
        return GL_VIV_YUY2;
    case GST_VIDEO_FORMAT_UYVY:
        return GL_VIV_UYVY;
    case GST_VIDEO_FORMAT_BGRA:
    case GST_VIDEO_FORMAT_BGRx:
        return GL_BGRA_EXT;
    default:
        return std::nullopt;
    }
}

// The decoder's buffers are usually padded (stride and plane offsets aligned to
// hardware requirements, see GstVideoMeta). glTexDirectVIVMap only takes a
// width/height and derives the plane layout from it, so it has to be given the
// padded dimensions: width from the luma stride, height from the distance
// between the luma and the first chroma plane.
std::optional<QSize> qGstVivantePaddedFrameSize(GstBuffer *buffer, const GstVideoInfo &videoInfo)
{
    const GstVideoMeta *meta = gst_buffer_get_video_meta(buffer);
    const int stride = meta ? meta->stride[0] : GST_VIDEO_INFO_PLANE_STRIDE(&videoInfo, 0);
    const int pixelStride = GST_VIDEO_INFO_COMP_PSTRIDE(&videoInfo, 0);
    const int width = GST_VIDEO_INFO_WIDTH(&videoInfo);
    const int height = GST_VIDEO_INFO_HEIGHT(&videoInfo);

    int paddedHeight = height;
    if (GST_VIDEO_INFO_N_PLANES(&videoInfo) > 1 && stride > 0) {
        const gsize lumaOffset = meta ? meta->offset[0] : GST_VIDEO_INFO_PLANE_OFFSET(&videoInfo, 0);
        const gsize chromaOffset =
                meta ? meta->offset[1] : GST_VIDEO_INFO_PLANE_OFFSET(&videoInfo, 1);
        paddedHeight = int((chromaOffset - lumaOffset) / stride);
    }
    const int paddedWidth = pixelStride > 0 ? stride / pixelStride : width;

    if (paddedWidth < width || paddedHeight < height) {
        qCWarning(qLcGstVideoBuffer) << "Unexpected Vivante buffer layout: stride" << stride
                                     << "padded size" << paddedWidth << "x" << paddedHeight
                                     << "for a" << width << "x" << height << "frame";
        return std::nullopt;
    }
    return QSize(paddedWidth, paddedHeight);
}
#endif // QT_CONFIG(gpu_vivante)

#if QT_CONFIG(gstreamer_gl)

class QGstQVideoFrameTextures : public QVideoFrameTextures
{
public:
    QGstQVideoFrameTextures(QRhi *rhi, QSize size, QVideoFrameFormat::PixelFormat format,
                            std::vector<guint32> textures)
        : m_rhi(rhi), m_glTextures(std::move(textures))
    {
        QRhiTexture::Flags textureFlags = {};
        if (QVideoTextureHelper::forceGlTextureExternalOesIsSet() && m_rhi
            && rhi->backend() == QRhi::OpenGLES2)
            textureFlags = {QRhiTexture::ExternalOES};

        auto desc = QVideoTextureHelper::textureDescription(format);
        for (uint i = 0; i < textures.size(); ++i) {
            QSize planeSize = desc->rhiPlaneSize(size, i, m_rhi);
            QRhiTexture::Format format = desc->rhiTextureFormat(i, m_rhi);
            m_textures[i].reset(rhi->newTexture(format, planeSize, 1, textureFlags));
            m_textures[i]->createFrom({ textures[i], 0 });
        }
    }

    QRhiTexture *texture(uint plane) const override
    {
        return plane < m_glTextures.size() ? m_textures[plane].get() : nullptr;
    }

private:
    QRhi *m_rhi = nullptr;
    std::vector<guint32> m_glTextures;
    std::unique_ptr<QRhiTexture> m_textures[QVideoTextureHelper::TextureDescription::maxPlanes];
};

static std::vector<guint32> mapFromGlTexture(const QGstBufferHandle &bufferHandle,
                                             GstVideoFrame &frame, GstVideoInfo &videoInfo)
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

    const auto unmapGuard = qScopeGuard([&] {
        gst_video_frame_unmap(&frame);
    });

    // Set and wait on a GPU fence before accessing the texture.
    {
        auto *sync_meta = gst_buffer_get_gl_sync_meta(buffer);
        QGstBufferHandle sync_buffer;
        if (!sync_meta) {
            sync_buffer.reset(gst_buffer_new(), QGstBufferHandle::HasRef);
            sync_meta = gst_buffer_add_gl_sync_meta(mem->context, sync_buffer.get());
        }
        gst_gl_sync_meta_set_sync_point(sync_meta, mem->context);
        gst_gl_sync_meta_wait(sync_meta, mem->context);
    }

    std::vector<guint32> textures;

    for (uint i = 0; i < GST_VIDEO_FRAME_N_PLANES(&frame); ++i)
        textures.push_back(*(guint32 *)frame.data[i]);

    return textures;
}

#endif

#if QT_CONFIG(gpu_vivante)
QtMultimediaPrivate::QVivanteVideoBuffer *QGstVideoBuffer::vivanteVideoBuffer()
{
    if (m_vivanteVideoBuffer)
        return m_vivanteVideoBuffer.get();

    GstBuffer *buffer = m_buffer.get();
    GstMemory *mem = gst_buffer_peek_memory(buffer, 0);
    if (!mem || !gst_is_phys_memory(mem)) {
        qCWarning(qLcGstVideoBuffer) << "Vivante buffer is not phys-memory backed";
        return nullptr;
    }

    const GstVideoInfo &gstVideoInfo = m_videoInfo.gstVideoInfo;
    const GstVideoFormat gstFormat = GST_VIDEO_INFO_FORMAT(&gstVideoInfo);
    const std::optional<GLenum> vivFormatToken = qGstVivFormatTokenFromGstVideoFormat(gstFormat);
    if (!vivFormatToken) {
        qCDebug(qLcGstVideoBuffer) << "Unsupported format for Vivante direct-texture import:"
                                   << gst_video_format_to_string(gstFormat)
                                   << "; falling back to CPU path";
        return nullptr;
    }

    const auto physicalAddress = quint32(gst_phys_memory_get_phys_addr(mem));
    if (!physicalAddress) {
        qCWarning(qLcGstVideoBuffer) << "Vivante buffer has no physical address";
        return nullptr;
    }

    // glTexDirectVIVMap needs a valid CPU-virtual address of the frame in
    // addition to the physical one, so the frame is mapped here. The mapped
    // frame is handed on as the QVivanteVideoBuffer's keep-alive argument,
    // which ends up in the texture handles. This keeps both the mapping and
    // the GstBuffer alive until the last texture using them is deleted.
    struct MappedFrame
    {
        GstVideoFrame frame{};
        ~MappedFrame() { gst_video_frame_unmap(&frame); }
    };
    auto mapped = std::make_shared<MappedFrame>();
    if (!gst_video_frame_map(&mapped->frame, &gstVideoInfo, buffer, GST_MAP_READ)) {
        qCWarning(qLcGstVideoBuffer) << "Failed to map Vivante phys-mem buffer";
        return nullptr;
    }
    const GstVideoFrame &frame = mapped->frame;

    const std::optional<QSize> paddedSize = qGstVivantePaddedFrameSize(buffer, gstVideoInfo);
    if (!paddedSize)
        return nullptr;
    if (*paddedSize != QSize(GST_VIDEO_FRAME_WIDTH(&frame), GST_VIDEO_FRAME_HEIGHT(&frame))) {
        qCDebug(qLcGstVideoBuffer) << "Vivante buffer is padded to" << *paddedSize << "for a"
                                   << GST_VIDEO_FRAME_WIDTH(&frame) << "x"
                                   << GST_VIDEO_FRAME_HEIGHT(&frame) << "frame";
    }

    void *logicalAddr = GST_VIDEO_FRAME_PLANE_DATA(&frame, 0);
    const quint32 planePhysicalAddress =
            physicalAddress + quint32(GST_VIDEO_FRAME_PLANE_OFFSET(&frame, 0));

    m_vivanteVideoBuffer = std::make_unique<QtMultimediaPrivate::QVivanteVideoBuffer>(
            m_frameFormat.pixelFormat(), *paddedSize, planePhysicalAddress,
            logicalAddr, *vivFormatToken, std::move(mapped));
    return m_vivanteVideoBuffer.get();
}

#endif // QT_CONFIG(gpu_vivante)

QVideoFrameTexturesUPtr QGstVideoBuffer::mapTextures([[maybe_unused]] QRhi &rhi)
{
    switch (m_memoryFormat) {
#if QT_CONFIG(gpu_vivante)
    case QGstCaps::VivantePhysMemory:
        if (QtMultimediaPrivate::QVivanteVideoBuffer *vivante = vivanteVideoBuffer())
            return vivante->mapTextures(rhi);
        return {};
#endif
    case QGstCaps::GLTexture:
#if QT_CONFIG(gstreamer_gl)
    {
        auto textures = mapFromGlTexture(m_buffer, m_frame, m_videoInfo.gstVideoInfo);
        if (textures.size())
            return std::make_unique<QGstQVideoFrameTextures>(
                    &rhi, QSize{ m_videoInfo.gstVideoInfo.width, m_videoInfo.gstVideoInfo.height },
                    m_frameFormat.pixelFormat(), std::move(textures));
        [[fallthrough]];
    }
#endif
    default:
        return {};
    }
}

QT_END_NAMESPACE
