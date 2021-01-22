/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qgstvideobuffer_p.h"

QT_BEGIN_NAMESPACE

#if GST_CHECK_VERSION(1,0,0)
QGstVideoBuffer::QGstVideoBuffer(GstBuffer *buffer, const GstVideoInfo &info)
    : QAbstractPlanarVideoBuffer(NoHandle)
    , m_videoInfo(info)
#else
QGstVideoBuffer::QGstVideoBuffer(GstBuffer *buffer, int bytesPerLine)
    : QAbstractVideoBuffer(NoHandle)
    , m_bytesPerLine(bytesPerLine)
#endif
    , m_buffer(buffer)
{
    gst_buffer_ref(m_buffer);
}

#if GST_CHECK_VERSION(1,0,0)
QGstVideoBuffer::QGstVideoBuffer(GstBuffer *buffer, const GstVideoInfo &info,
                QGstVideoBuffer::HandleType handleType,
                const QVariant &handle)
    : QAbstractPlanarVideoBuffer(handleType)
    , m_videoInfo(info)
#else
QGstVideoBuffer::QGstVideoBuffer(GstBuffer *buffer, int bytesPerLine,
                QGstVideoBuffer::HandleType handleType,
                const QVariant &handle)
    : QAbstractVideoBuffer(handleType)
    , m_bytesPerLine(bytesPerLine)
#endif
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

#if GST_CHECK_VERSION(1,0,0)

int QGstVideoBuffer::map(MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4])
{
    const GstMapFlags flags = GstMapFlags(((mode & ReadOnly) ? GST_MAP_READ : 0)
                | ((mode & WriteOnly) ? GST_MAP_WRITE : 0));

    if (mode == NotMapped || m_mode != NotMapped) {
        return 0;
    } else if (m_videoInfo.finfo->n_planes == 0) {         // Encoded
        if (gst_buffer_map(m_buffer, &m_frame.map[0], flags)) {
            if (numBytes)
                *numBytes = m_frame.map[0].size;
            bytesPerLine[0] = -1;
            data[0] = static_cast<uchar *>(m_frame.map[0].data);

            m_mode = mode;

            return 1;
        }
    } else if (gst_video_frame_map(&m_frame, &m_videoInfo, m_buffer, flags)) {
        if (numBytes)
            *numBytes = m_frame.info.size;

        for (guint i = 0; i < m_frame.info.finfo->n_planes; ++i) {
            bytesPerLine[i] = m_frame.info.stride[i];
            data[i] = static_cast<uchar *>(m_frame.data[i]);
        }

        m_mode = mode;

        return m_frame.info.finfo->n_planes;
    }
    return 0;
}

#else

uchar *QGstVideoBuffer::map(MapMode mode, int *numBytes, int *bytesPerLine)
{
    if (mode != NotMapped && m_mode == NotMapped) {
        if (numBytes)
            *numBytes = m_buffer->size;
        if (bytesPerLine)
            *bytesPerLine = m_bytesPerLine;

        m_mode = mode;

        return m_buffer->data;
    } else {
        return 0;
    }
}

#endif

void QGstVideoBuffer::unmap()
{
#if GST_CHECK_VERSION(1,0,0)
    if (m_mode != NotMapped) {
        if (m_videoInfo.finfo->n_planes == 0)
            gst_buffer_unmap(m_buffer, &m_frame.map[0]);
        else
            gst_video_frame_unmap(&m_frame);
    }
#endif
    m_mode = NotMapped;
}

QT_END_NAMESPACE
