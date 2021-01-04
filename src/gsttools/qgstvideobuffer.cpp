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

QT_BEGIN_NAMESPACE

QGstVideoBuffer::QGstVideoBuffer(GstBuffer *buffer, const GstVideoInfo &info)
    : QAbstractVideoBuffer(NoHandle)
    , m_videoInfo(info)
    , m_buffer(buffer)
{
    gst_buffer_ref(m_buffer);
}

QGstVideoBuffer::QGstVideoBuffer(GstBuffer *buffer, const GstVideoInfo &info,
                QGstVideoBuffer::HandleType handleType,
                const QVariant &handle)
    : QAbstractVideoBuffer(handleType)
    , m_videoInfo(info)
    , m_buffer(buffer)
    , m_handle(handle)
{
    gst_buffer_ref(m_buffer);
}

QGstVideoBuffer::~QGstVideoBuffer()
{
    unmap();

    gst_buffer_unref(m_buffer);
}


QAbstractVideoBuffer::MapMode QGstVideoBuffer::mapMode() const
{
    return m_mode;
}

QAbstractVideoBuffer::MapData QGstVideoBuffer::map(MapMode mode)
{
    const GstMapFlags flags = GstMapFlags(((mode & ReadOnly) ? GST_MAP_READ : 0)
                | ((mode & WriteOnly) ? GST_MAP_WRITE : 0));

    MapData mapData;
    if (mode == NotMapped || m_mode != NotMapped)
        return mapData;

    if (m_videoInfo.finfo->n_planes == 0) {         // Encoded
        if (gst_buffer_map(m_buffer, &m_frame.map[0], flags)) {
            mapData.nBytes = m_frame.map[0].size;
            mapData.nPlanes = 1;
            mapData.bytesPerLine[0] = -1;
            mapData.data[0] = static_cast<uchar *>(m_frame.map[0].data);

            m_mode = mode;
        }
    } else if (gst_video_frame_map(&m_frame, &m_videoInfo, m_buffer, flags)) {
        mapData.nBytes = m_frame.info.size;
        mapData.nPlanes = m_frame.info.finfo->n_planes;

        for (guint i = 0; i < m_frame.info.finfo->n_planes; ++i) {
            mapData.bytesPerLine[i] = m_frame.info.stride[i];
            mapData.data[i] = static_cast<uchar *>(m_frame.data[i]);
        }

        m_mode = mode;
    }
    return mapData;
}

void QGstVideoBuffer::unmap()
{
    if (m_mode != NotMapped) {
        if (m_videoInfo.finfo->n_planes == 0)
            gst_buffer_unmap(m_buffer, &m_frame.map[0]);
        else
            gst_video_frame_unmap(&m_frame);
    }
    m_mode = NotMapped;
}

QT_END_NAMESPACE
