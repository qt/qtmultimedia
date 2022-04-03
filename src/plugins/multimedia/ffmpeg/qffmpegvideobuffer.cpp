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
#include <libavutil/hdr_dynamic_metadata.h>
#include <libavutil/mastering_display_metadata.h>
}

QT_BEGIN_NAMESPACE

QFFmpegVideoBuffer::QFFmpegVideoBuffer(AVFrame *frame)
    : QAbstractVideoBuffer(QVideoFrame::NoHandle)
    , frame(frame)
{
    if (frame->hw_frames_ctx) {
        hwFrame = frame;
        m_pixelFormat = toQtPixelFormat(QFFmpeg::HWAccel::format(frame));
        return;
    }

    swFrame = frame;
    m_pixelFormat = toQtPixelFormat(AVPixelFormat(swFrame->format));

    convertSWFrame();
}

QFFmpegVideoBuffer::~QFFmpegVideoBuffer()
{
    delete textures;
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

void QFFmpegVideoBuffer::setTextureConverter(const QFFmpeg::TextureConverter &converter)
{
    textureConverter = converter;
    textureConverter.init(hwFrame);
    m_type = converter.isNull() ? QVideoFrame::NoHandle : QVideoFrame::RhiTextureHandle;
}

QVideoFrameFormat::YCbCrColorSpace QFFmpegVideoBuffer::colorSpace() const
{
    switch (frame->colorspace) {
    default:
    case AVCOL_SPC_UNSPECIFIED:
    case AVCOL_SPC_RESERVED:
    case AVCOL_SPC_FCC:
    case AVCOL_SPC_SMPTE240M:
    case AVCOL_SPC_YCGCO:
    case AVCOL_SPC_SMPTE2085:
    case AVCOL_SPC_CHROMA_DERIVED_NCL:
    case AVCOL_SPC_CHROMA_DERIVED_CL:
    case AVCOL_SPC_ICTCP: // BT.2100 ICtCp
        return QVideoFrameFormat::YCbCr_Undefined;
    case AVCOL_SPC_RGB:
        return QVideoFrameFormat::YCbCr_AdobeRgb;
    case AVCOL_SPC_BT709:
        return QVideoFrameFormat::YCbCr_BT709;
    case AVCOL_SPC_BT470BG: // BT601
    case AVCOL_SPC_SMPTE170M: // Also BT601
        return QVideoFrameFormat::YCbCr_BT601;
    case AVCOL_SPC_BT2020_NCL: // Non constant luminence
    case AVCOL_SPC_BT2020_CL: // Constant luminence
        return QVideoFrameFormat::YCbCr_BT2020;
    }
}

QVideoFrameFormat::ColorTransfer QFFmpegVideoBuffer::colorTransfer() const
{
    switch (frame->color_trc) {
    case AVCOL_TRC_BT709:
    // The following three cases have transfer characteristics identical to BT709
    case AVCOL_TRC_BT1361_ECG:
    case AVCOL_TRC_BT2020_10:
    case AVCOL_TRC_BT2020_12:
    case AVCOL_TRC_SMPTE240M: // almost identical to bt709
        return QVideoFrameFormat::ColorTransfer_BT709;
    case AVCOL_TRC_GAMMA22:
    case AVCOL_TRC_SMPTE428 : // No idea, let's hope for the best...
    case AVCOL_TRC_IEC61966_2_1: // sRGB, close enough to 2.2...
    case AVCOL_TRC_IEC61966_2_4: // not quite, but probably close enough
        return QVideoFrameFormat::ColorTransfer_Gamma22;
    case AVCOL_TRC_GAMMA28:
        return QVideoFrameFormat::ColorTransfer_Gamma28;
    case AVCOL_TRC_SMPTE170M:
        return QVideoFrameFormat::ColorTransfer_BT601;
    case AVCOL_TRC_LINEAR:
        return QVideoFrameFormat::ColorTransfer_Linear;
    case AVCOL_TRC_SMPTE2084:
        return QVideoFrameFormat::ColorTransfer_ST2084;
    case AVCOL_TRC_ARIB_STD_B67:
        return QVideoFrameFormat::ColorTransfer_STD_B67;
    default:
        break;
    }
    return QVideoFrameFormat::ColorTransfer_Unknown;
}

QVideoFrameFormat::ColorRange QFFmpegVideoBuffer::colorRange() const
{
    switch (frame->color_range) {
    case AVCOL_RANGE_MPEG:
        return QVideoFrameFormat::ColorRange_Video;
    case AVCOL_RANGE_JPEG:
        return QVideoFrameFormat::ColorRange_Full;
    default:
        return QVideoFrameFormat::ColorRange_Unknown;
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

//    qDebug() << "MAP:";
    MapData mapData;
    auto *desc = QVideoTextureHelper::textureDescription(pixelFormat());
    mapData.nPlanes = desc->nplanes;
    for (int i = 0; i < mapData.nPlanes; ++i) {
        mapData.data[i] = swFrame->data[i];
        mapData.bytesPerLine[i] = swFrame->linesize[i];
        mapData.size[i] = mapData.bytesPerLine[i]*desc->heightForPlane(swFrame->height, i);
//        qDebug() << "    " << i << mapData.data[i] << mapData.size[i];
    }
    return mapData;
}

void QFFmpegVideoBuffer::unmap()
{
    // nothing to do here for SW buffers
}

void QFFmpegVideoBuffer::mapTextures()
{
    if (textures || !hwFrame)
        return;
//    qDebug() << ">>>>> mapTextures";
    textures = textureConverter.getTextures(hwFrame);
    if (!textures)
        qWarning() << "    failed to get textures for frame" << textureConverter.isNull();
}

quint64 QFFmpegVideoBuffer::textureHandle(int plane) const
{
    return textures ? textures->texture(plane) : 0;
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
    case QVideoFrameFormat::Format_Jpeg:
        // We're using the data from the converted QImage here, which is in BGRA.
        return AV_PIX_FMT_BGRA;
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
