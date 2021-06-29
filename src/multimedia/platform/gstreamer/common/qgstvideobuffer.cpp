/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstvideobuffer_p.h"

#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>

#include "qgstutils_p.h"

#if QT_CONFIG(gstreamer_gl)
#include <QtGui/private/qrhi_p.h>
#include <QtGui/private/qrhigles2_p.h>

#include <gst/gl/gstglconfig.h>
#include <gst/gl/gstglmemory.h>
#endif


QT_BEGIN_NAMESPACE

QGstVideoBuffer::QGstVideoBuffer(GstBuffer *buffer, const GstVideoInfo &info, QRhi *rhi, BufferFormat format)
    : QAbstractVideoBuffer(QVideoFrame::NoHandle, rhi)
    , bufferFormat(format)
    , m_videoInfo(info)
    , m_buffer(buffer)
{
    gst_buffer_ref(m_buffer);
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
        mapData.nPlanes = m_frame.info.finfo->n_planes;

        for (guint i = 0; i < m_frame.info.finfo->n_planes; ++i) {
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

quint64 QGstVideoBuffer::textureHandle(int plane) const
{
    if (plane != 0)
        return 0;
#if QT_CONFIG(gstreamer_gl)
    if (bufferFormat == GLTexture) {
        auto *memory = gst_buffer_peek_memory(m_buffer, 0);
        if (gst_is_gl_memory(memory)) {
            GstGLMemory *glmem = GST_GL_MEMORY_CAST(memory);
            // ### Handle multiple planes
            return gst_gl_memory_get_texture_id(glmem);
        }
    } else if (bufferFormat == VideoGLTextureUploadMeta) {
        Q_ASSERT(rhi && rhi->backend() == QRhi::OpenGLES2);

        auto *upload = gst_buffer_get_video_gl_texture_upload_meta(m_buffer);
        if (upload) {
            rhi->makeThreadLocalNativeContextCurrent();
            // ### Handle multiple planes
            guint textures[4];
            gst_video_gl_texture_upload_meta_upload(upload, textures);
            return textures[0];
        } else {
            qWarning() << "Could not use GstVideoGLTextureUploadMeta";
        }
    }
#endif
    return 0;
}

QT_END_NAMESPACE
