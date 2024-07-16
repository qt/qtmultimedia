// Copyright (C) 2022 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxcameraframebuffer_p.h"

#include <limits>

template <typename T>
static constexpr int toInt(T value)
{
    if constexpr (sizeof(T) >= sizeof(int)) {
        if (std::is_signed_v<T>) {
            return static_cast<int>(std::clamp<T>(value,
                        std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
        } else {
            return static_cast<int>(std::min<T>(value, std::numeric_limits<int>::max()));
        }
    } else {
        return static_cast<int>(value);
    }
}

template <typename T>
static constexpr QSize frameSize(const T &frame)
{
    return { toInt(frame.width), toInt(frame.height) };
}

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
        .planeCount = 2,
        .bytesPerLine = {
            toInt(frame.stride),
            toInt(frame.uv_stride)
        },
        .data = {
            baseAddress,
            baseAddress + frame.uv_offset
        },
        .dataSize = {
            toInt(frame.stride * frame.height),
            toInt(frame.uv_stride * frame.height / 2)
        }
    };
}

static QAbstractVideoBuffer::MapData mapData(const camera_frame_rgb8888_t &frame,
        unsigned char *baseAddress)
{
    return {
        .planeCount = 1,
        .bytesPerLine = {
            toInt(frame.stride)
        },
        .data = {
            baseAddress
        },
        .dataSize = {
            toInt(frame.stride * frame.height),
        }
    };
}

static QAbstractVideoBuffer::MapData mapData(const camera_frame_gray8_t &frame,
        unsigned char *baseAddress)
{
    return {
        .planeCount = 1,
        .bytesPerLine = {
            toInt(frame.stride)
        },
        .data = {
            baseAddress
        },
        .dataSize = {
            toInt(frame.stride * frame.height)
        }
    };
}

static QAbstractVideoBuffer::MapData mapData(const camera_frame_cbycry_t &frame,
        unsigned char *baseAddress)
{
    return {
        .planeCount = 1,
        .bytesPerLine = {
            toInt(frame.stride)
        },
        .data = {
            baseAddress
        },
        .dataSize = {
            toInt(frame.bufsize),
        }
    };
}

static QAbstractVideoBuffer::MapData mapData(const camera_frame_ycbcr420p_t &frame,
        unsigned char *baseAddress)
{
    return {
        .planeCount = 3,
        .bytesPerLine = {
            toInt(frame.y_stride),
            frame.cb_stride,
            frame.cr_stride,
        },
        .data = {
            baseAddress,
            baseAddress + frame.cb_offset,
            baseAddress + frame.cr_offset,
        },
        .dataSize = {
            toInt(frame.y_stride * frame.height),
            toInt(frame.cb_stride * frame.height / 2),
            toInt(frame.cr_stride * frame.height / 2)
        }
    };
}

static QAbstractVideoBuffer::MapData mapData(const camera_frame_ycbycr_t &frame,
        unsigned char *baseAddress)
{
    return {
        .planeCount = 1,
        .bytesPerLine = {
            toInt(frame.stride)
        },
        .data = {
            baseAddress
        },
        .dataSize = {
            toInt(frame.stride * frame.height)
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
    : QHwVideoBuffer(rhi ? QVideoFrame::RhiTextureHandle : QVideoFrame::NoHandle, rhi),
      m_rhi(rhi),
      m_pixelFormat(::frameTypeToPixelFormat(buffer->frametype)),
      m_dataSize(::bufferDataSize(buffer))
{
    if (m_dataSize <= 0)
        return;

    m_data = std::make_unique<unsigned char[]>(m_dataSize);

    memcpy(m_data.get(), buffer->framebuf, m_dataSize);

    m_mapData = ::mapData(buffer, m_data.get());

    m_frameSize = ::frameSize(buffer);
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
