/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qffmpegvideobuffer_p.h"
#include "private/qvideotexturehelper_p.h"

QT_BEGIN_NAMESPACE

QFFmpegVideoBuffer::QFFmpegVideoBuffer(AVFrame *frame)
    : QAbstractVideoBuffer(QVideoFrame::NoHandle, nullptr)
    , m_frame(frame)
{

}

QFFmpegVideoBuffer::~QFFmpegVideoBuffer()
{
    av_frame_free(&m_frame);
}

QVideoFrame::MapMode QFFmpegVideoBuffer::mapMode() const
{
    return m_mode;
}

QAbstractVideoBuffer::MapData QFFmpegVideoBuffer::map(QVideoFrame::MapMode mode)
{
    m_mode = mode;

    MapData mapData;
    mapData.nPlanes = QVideoTextureHelper::textureDescription(pixelFormat())->nplanes;
    for (int i = 0; i < mapData.nPlanes; ++i) {
        mapData.data[i] = m_frame->data[i];
        mapData.bytesPerLine[i] = m_frame->linesize[i];
        auto *bufferRef = av_frame_get_plane_buffer(m_frame, i);
        mapData.size[i] = bufferRef ? bufferRef->size : 0;
    }
    return mapData;
}

void QFFmpegVideoBuffer::unmap()
{
    // nothing to do here for SW buffers
}

QVideoFrameFormat::PixelFormat QFFmpegVideoBuffer::pixelFormat() const
{
    return toQtPixelFormat(AVPixelFormat(m_frame->format));
}

QSize QFFmpegVideoBuffer::size() const
{
    return QSize(m_frame->width, m_frame->height);
}

QVideoFrameFormat::PixelFormat QFFmpegVideoBuffer::toQtPixelFormat(AVPixelFormat avPixelFormat)
{
    switch (avPixelFormat) {
    default:
        return QVideoFrameFormat::Format_Invalid;
    case AV_PIX_FMT_ARGB:
        return QVideoFrameFormat::Format_ARGB8888;
    case AV_PIX_FMT_0RGB:
        return QVideoFrameFormat::Format_XRGB8888;
    case AV_PIX_FMT_BGRA:
        return QVideoFrameFormat::Format_BGRA8888;
    case AV_PIX_FMT_BGR0:
        return QVideoFrameFormat::Format_BGRX8888;
    case AV_PIX_FMT_ABGR:
        return QVideoFrameFormat::Format_ABGR8888;
    case AV_PIX_FMT_0BGR:
        return QVideoFrameFormat::Format_XBGR8888;
    case AV_PIX_FMT_RGBA:
        return QVideoFrameFormat::Format_RGBA8888;
    case AV_PIX_FMT_RGB0:
        return QVideoFrameFormat::Format_RGBX8888;

    case AV_PIX_FMT_YUV422P:
        return QVideoFrameFormat::Format_YUV422P;
    case AV_PIX_FMT_YUV420P:
        return QVideoFrameFormat::Format_YUV420P;
    case AV_PIX_FMT_UYVY422:
        return QVideoFrameFormat::Format_UYVY;
    case AV_PIX_FMT_YUYV422:
        return QVideoFrameFormat::Format_YUYV;
    case AV_PIX_FMT_NV12:
        return QVideoFrameFormat::Format_NV12;
    case AV_PIX_FMT_NV21:
        return QVideoFrameFormat::Format_NV21;
    case AV_PIX_FMT_GRAY8:
        return QVideoFrameFormat::Format_Y8;
    case AV_PIX_FMT_GRAY16:
        return QVideoFrameFormat::Format_Y16;

    case AV_PIX_FMT_P010:
        return QVideoFrameFormat::Format_P010;
    case AV_PIX_FMT_P016:
        return QVideoFrameFormat::Format_P016;
    }
}

AVPixelFormat QFFmpegVideoBuffer::toAVPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat)
{
    switch (pixelFormat) {
    default:
    case QVideoFrameFormat::Format_Invalid:
    case QVideoFrameFormat::Format_AYUV:
    case QVideoFrameFormat::Format_AYUV_Premultiplied:
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_IMC1:
    case QVideoFrameFormat::Format_IMC2:
    case QVideoFrameFormat::Format_IMC3:
    case QVideoFrameFormat::Format_IMC4:
        return AV_PIX_FMT_NONE;
    case QVideoFrameFormat::Format_ARGB8888:
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
        return AV_PIX_FMT_ARGB;
    case QVideoFrameFormat::Format_XRGB8888:
        return AV_PIX_FMT_0RGB;
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
        return AV_PIX_FMT_BGRA;
    case QVideoFrameFormat::Format_BGRX8888:
        return AV_PIX_FMT_BGR0;
    case QVideoFrameFormat::Format_ABGR8888:
        return AV_PIX_FMT_ABGR;
    case QVideoFrameFormat::Format_XBGR8888:
        return AV_PIX_FMT_0BGR;
    case QVideoFrameFormat::Format_RGBA8888:
        return AV_PIX_FMT_RGBA;
    case QVideoFrameFormat::Format_RGBX8888:
        return AV_PIX_FMT_RGB0;

    case QVideoFrameFormat::Format_YUV422P:
        return AV_PIX_FMT_YUV422P;
    case QVideoFrameFormat::Format_YUV420P:
        return AV_PIX_FMT_YUV420P;
    case QVideoFrameFormat::Format_UYVY:
        return AV_PIX_FMT_UYVY422;
    case QVideoFrameFormat::Format_YUYV:
        return AV_PIX_FMT_YUYV422;
    case QVideoFrameFormat::Format_NV12:
        return AV_PIX_FMT_NV12;
    case QVideoFrameFormat::Format_NV21:
        return AV_PIX_FMT_NV21;
    case QVideoFrameFormat::Format_Y8:
        return AV_PIX_FMT_GRAY8;
    case QVideoFrameFormat::Format_Y16:
        return AV_PIX_FMT_GRAY16;

    case QVideoFrameFormat::Format_P010:
        return AV_PIX_FMT_P010;
    case QVideoFrameFormat::Format_P016:
        return AV_PIX_FMT_P016;
    }
}

QT_END_NAMESPACE
