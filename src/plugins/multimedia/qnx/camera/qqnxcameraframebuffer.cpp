/****************************************************************************
**
** Copyright (C) 2022 The Qt Company
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

#include "qqnxcameraframebuffer_p.h"

#include <limits>

static constexpr QVideoFrameFormat::PixelFormat frameTypeToPixelFormat(camera_frametype_t type)
{
    switch (type) {
    case CAMERA_FRAMETYPE_NV12:
        return QVideoFrameFormat::Format_NV12;
    case CAMERA_FRAMETYPE_RGB8888:
        return QVideoFrameFormat::Format_ARGB8888;
    case CAMERA_FRAMETYPE_GRAY8:
        return QVideoFrameFormat::Format_Y8;
    case CAMERA_FRAMETYPE_CBYCRY:
        return QVideoFrameFormat::Format_UYVY;
    case CAMERA_FRAMETYPE_YCBCR420P:
        return QVideoFrameFormat::Format_YUV420P;
    case CAMERA_FRAMETYPE_YCBYCR:
        return QVideoFrameFormat::Format_YUYV;
    default:
        break;
    }

    return QVideoFrameFormat::Format_Invalid;
}

static constexpr size_t bufferDataSize(const camera_frame_nv12_t &frame)
{
    return frame.uv_offset + frame.uv_stride * frame.height / 2;
}

static constexpr size_t bufferDataSize(const camera_frame_rgb8888_t &frame)
{
    return frame.stride * frame.height;
}

static constexpr size_t bufferDataSize(const camera_frame_gray8_t &frame)
{
    return frame.stride * frame.height;
}

static constexpr size_t bufferDataSize(const camera_frame_cbycry_t &frame)
{
    return frame.bufsize;
}

static constexpr size_t bufferDataSize(const camera_frame_ycbcr420p_t &frame)
{
    return frame.cr_offset + frame.cr_stride * frame.height / 2;
}

static constexpr size_t bufferDataSize(const camera_frame_ycbycr_t &frame)
{
    return frame.stride * frame.height;
}

static constexpr size_t bufferDataSize(const camera_buffer_t *buffer)
{
    switch (buffer->frametype) {
    case CAMERA_FRAMETYPE_NV12:
        return bufferDataSize(buffer->framedesc.nv12);
    case CAMERA_FRAMETYPE_RGB8888:
        return bufferDataSize(buffer->framedesc.rgb8888);
    case CAMERA_FRAMETYPE_GRAY8:
        return bufferDataSize(buffer->framedesc.gray8);
    case CAMERA_FRAMETYPE_CBYCRY:
        return bufferDataSize(buffer->framedesc.cbycry);
    case CAMERA_FRAMETYPE_YCBCR420P:
        return bufferDataSize(buffer->framedesc.ycbcr420p);
    case CAMERA_FRAMETYPE_YCBYCR:
        return bufferDataSize(buffer->framedesc.ycbycr);
    default:
        break;
    }

    return 0;
}

static QAbstractVideoBuffer::MapData mapData(const camera_frame_nv12_t &frame,
        unsigned char *baseAddress)
{

    return {
        .nPlanes = 2,
        .bytesPerLine = {
            frame.stride,
            frame.uv_stride
        },
        .data = {
            baseAddress,
            baseAddress + frame.uv_offset
        },
        .size = {
            frame.stride * frame.height,
            frame.uv_stride * frame.height / 2
        }
    };
}

static QAbstractVideoBuffer::MapData mapData(const camera_frame_rgb8888_t &frame,
        unsigned char *baseAddress)
{
    return {
        .nPlanes = 1,
        .bytesPerLine = {
            frame.stride
        },
        .data = {
            baseAddress
        },
        .size = {
            frame.stride * frame.height,
        }
    };
}

static QAbstractVideoBuffer::MapData mapData(const camera_frame_gray8_t &frame,
        unsigned char *baseAddress)
{
    return {
        .nPlanes = 1,
        .bytesPerLine = {
            frame.stride
        },
        .data = {
            baseAddress
        },
        .size = {
            frame.stride * frame.height
        }
    };
}

static QAbstractVideoBuffer::MapData mapData(const camera_frame_cbycry_t &frame,
        unsigned char *baseAddress)
{
    return {
        .nPlanes = 1,
        .bytesPerLine = {
            frame.stride
        },
        .data = {
            baseAddress
        },
        .size = {
            frame.bufsize,
        }
    };
}

static QAbstractVideoBuffer::MapData mapData(const camera_frame_ycbcr420p_t &frame,
        unsigned char *baseAddress)
{
    return {
        .nPlanes = 3,
        .bytesPerLine = {
            frame.y_stride,
            frame.cb_stride,
            frame.cr_stride,
        },
        .data = {
            baseAddress,
            baseAddress + frame.cb_offset,
            baseAddress + frame.cr_offset,
        },
        .size = {
            frame.y_stride * frame.height,
            frame.cb_stride * frame.height / 2,
            frame.cr_stride * frame.height / 2
        }
    };
}

static QAbstractVideoBuffer::MapData mapData(const camera_frame_ycbycr_t &frame,
        unsigned char *baseAddress)
{
    return {
        .nPlanes = 1,
        .bytesPerLine = {
            frame.stride
        },
        .data = {
            baseAddress
        },
        .size = {
            frame.stride * frame.height
        }
    };
}

static QAbstractVideoBuffer::MapData mapData(const camera_buffer_t *buffer,
        unsigned char *baseAddress)
{
    switch (buffer->frametype) {
    case CAMERA_FRAMETYPE_NV12:
        return mapData(buffer->framedesc.nv12, baseAddress);
    case CAMERA_FRAMETYPE_RGB8888:
        return mapData(buffer->framedesc.rgb8888, baseAddress);
    case CAMERA_FRAMETYPE_GRAY8:
        return mapData(buffer->framedesc.gray8, baseAddress);
    case CAMERA_FRAMETYPE_CBYCRY:
        return mapData(buffer->framedesc.cbycry, baseAddress);
    case CAMERA_FRAMETYPE_YCBCR420P:
        return mapData(buffer->framedesc.ycbcr420p, baseAddress);
    case CAMERA_FRAMETYPE_YCBYCR:
        return mapData(buffer->framedesc.ycbycr, baseAddress);
    default:
        break;
    }

    return {};
}

static constexpr int toInt(uint32_t value)
{
    return static_cast<int>(std::min<uint32_t>(value, std::numeric_limits<int>::max()));
}

template <typename T>
static constexpr QSize frameSize(const T &frame)
{
    return { toInt(frame.width), toInt(frame.height) };
}

static constexpr QSize frameSize(const camera_buffer_t *buffer)
{
    switch (buffer->frametype) {
    case CAMERA_FRAMETYPE_NV12:
        return frameSize(buffer->framedesc.nv12);
    case CAMERA_FRAMETYPE_RGB8888:
        return frameSize(buffer->framedesc.rgb8888);
    case CAMERA_FRAMETYPE_GRAY8:
        return frameSize(buffer->framedesc.gray8);
    case CAMERA_FRAMETYPE_CBYCRY:
        return frameSize(buffer->framedesc.cbycry);
    case CAMERA_FRAMETYPE_YCBCR420P:
        return frameSize(buffer->framedesc.ycbcr420p);
    case CAMERA_FRAMETYPE_YCBYCR:
        return frameSize(buffer->framedesc.ycbycr);
    default:
        break;
    }

    return {};
}

QT_BEGIN_NAMESPACE

QQnxCameraFrameBuffer::QQnxCameraFrameBuffer(const camera_buffer_t *buffer, QRhi *rhi)
    : QAbstractVideoBuffer(rhi ? QVideoFrame::RhiTextureHandle : QVideoFrame::NoHandle, rhi)
    , m_rhi(rhi)
    , m_pixelFormat(::frameTypeToPixelFormat(buffer->frametype))
    , m_dataSize(::bufferDataSize(buffer))
{
    if (m_dataSize <= 0)
        return;

    m_data = std::make_unique<unsigned char[]>(m_dataSize);

    memcpy(m_data.get(), buffer->framebuf, m_dataSize);

    m_mapData = ::mapData(buffer, m_data.get());

    m_frameSize = ::frameSize(buffer);
}

QVideoFrame::MapMode QQnxCameraFrameBuffer::mapMode() const
{
    return QVideoFrame::ReadOnly;
}

QAbstractVideoBuffer::MapData QQnxCameraFrameBuffer::map(QVideoFrame::MapMode)
{
    return m_mapData;
}

void QQnxCameraFrameBuffer::unmap()
{
}

QVideoFrameFormat::PixelFormat QQnxCameraFrameBuffer::pixelFormat() const
{
    return m_pixelFormat;
}

QSize QQnxCameraFrameBuffer::size() const
{
    return m_frameSize;
}

QT_END_NAMESPACE
