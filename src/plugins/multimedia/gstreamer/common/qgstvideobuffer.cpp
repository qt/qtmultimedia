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

    std::vector<guint32> textures;

    for (uint i = 0; i < GST_VIDEO_FRAME_N_PLANES(&frame); ++i)
        textures.push_back(*(guint32 *)frame.data[i]);

    return textures;
}

#endif

QVideoFrameTexturesUPtr QGstVideoBuffer::mapTextures([[maybe_unused]] QRhi &rhi)
{
    switch (m_memoryFormat) {
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
