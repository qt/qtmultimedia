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
#include "qffmpeghwaccel_p.h"

extern "C" {
#include <libavutil/pixdesc.h>
}

QT_BEGIN_NAMESPACE

QFFmpegVideoBuffer::QFFmpegVideoBuffer(AVFrame *frame, const QFFmpeg::HWAccel &accel)
    : QAbstractVideoBuffer(accel.rhi() ? QVideoFrame::RhiTextureHandle : QVideoFrame::NoHandle, accel.rhi())
    , frame(frame)
    , hwAccel(accel)
{
    if (frame->hw_frames_ctx) {
        Q_ASSERT(!accel.isNull());
        hwFrame = frame;
        m_pixelFormat = toQtPixelFormat(accel.format(frame));
        return;
    }

    m_pixelFormat = toQtPixelFormat(AVPixelFormat(swFrame->format));
    swFrame = frame;

    convertSWFrame();
}

QFFmpegVideoBuffer::~QFFmpegVideoBuffer()
{
    if (swFrame)
        av_frame_free(&swFrame);
    if (hwFrame)
        av_frame_free(&hwFrame);
}

void QFFmpegVideoBuffer::convertSWFrame()
{
    Q_ASSERT(swFrame);
    bool needsConversion = false;
    auto pixelFormat = toQtPixelFormat(AVPixelFormat(swFrame->format), &needsConversion);
//    qDebug() << "SW frame format:" << pixelFormat << swFrame->format << needsConversion;

    if (pixelFormat != m_pixelFormat) {
        AVPixelFormat newFormat = toAVPixelFormat(m_pixelFormat);
        // convert the format into something we can handle
        SwsContext *c = sws_getContext(swFrame->width, swFrame->height, AVPixelFormat(swFrame->format),
                                       swFrame->width, swFrame->height, newFormat,
                                       SWS_BICUBIC, nullptr, nullptr, nullptr);

        AVFrame *newFrame = av_frame_alloc();
        newFrame->width = swFrame->width;
        newFrame->height = swFrame->height;
        newFrame->format = newFormat;
        av_frame_get_buffer(newFrame, 0);

        sws_scale(c, swFrame->data, swFrame->linesize, 0, swFrame->height, newFrame->data, newFrame->linesize);
        av_frame_free(&swFrame);
        swFrame = newFrame;
        sws_freeContext(c);
    }
}

QVideoFrame::MapMode QFFmpegVideoBuffer::mapMode() const
{
    return m_mode;
}

QAbstractVideoBuffer::MapData QFFmpegVideoBuffer::map(QVideoFrame::MapMode mode)
{
    if (!swFrame) {
        Q_ASSERT(hwFrame && hwFrame->hw_frames_ctx);
        swFrame = av_frame_alloc();
        /* retrieve data from GPU to CPU */
        int ret = av_hwframe_transfer_data(swFrame, hwFrame, 0);
        if (ret < 0) {
            qWarning() << "Error transferring the data to system memory\n";
            return {};
        }
        convertSWFrame();
    }

    m_mode = mode;

    MapData mapData;
    mapData.nPlanes = QVideoTextureHelper::textureDescription(pixelFormat())->nplanes;
    for (int i = 0; i < mapData.nPlanes; ++i) {
        mapData.data[i] = swFrame->data[i];
        mapData.bytesPerLine[i] = swFrame->linesize[i];
        auto *bufferRef = av_frame_get_plane_buffer(swFrame, i);
        mapData.size[i] = bufferRef ? bufferRef->size : 0;
    }
    return mapData;
}

void QFFmpegVideoBuffer::unmap()
{
    // nothing to do here for SW buffers
}

void QFFmpegVideoBuffer::mapTextures()
{
    if (textures[0])
        return;
    qDebug() << ">>>>> mapTextures";
//    QFFmpeg::getRhiTextures(rhi, hwDeviceContext, hwFrame, textures);
}

quint64 QFFmpegVideoBuffer::textureHandle(int plane) const
{
    qDebug() << "retrieving texture for plane" << plane << textures[plane];
    return textures[plane];
}

QVideoFrameFormat::PixelFormat QFFmpegVideoBuffer::pixelFormat() const
{
    return m_pixelFormat;
}

QSize QFFmpegVideoBuffer::size() const
{
    return QSize(frame->width, frame->height);
}

QVideoFrameFormat::PixelFormat QFFmpegVideoBuffer::toQtPixelFormat(AVPixelFormat avPixelFormat, bool *needsConversion)
{
    if (needsConversion)
        *needsConversion = false;

    switch (avPixelFormat) {
    default:
        break;
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
    case AV_PIX_FMT_YUV420P10:
        return QVideoFrameFormat::Format_YUV420P10;
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

    if (needsConversion)
        *needsConversion = true;

    const AVPixFmtDescriptor *descriptor = av_pix_fmt_desc_get(avPixelFormat);

    if (descriptor->flags & AV_PIX_FMT_FLAG_RGB)
        return QVideoFrameFormat::Format_RGBA8888;

    if (descriptor->comp[0].depth > 8)
        return QVideoFrameFormat::Format_P016;
    return QVideoFrameFormat::Format_YUV420P;
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
    case QVideoFrameFormat::Format_YUV420P10:
        return AV_PIX_FMT_YUV420P10;
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
