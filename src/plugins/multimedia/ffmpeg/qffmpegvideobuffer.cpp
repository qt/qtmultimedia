// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideobuffer_p.h"
#include "private/qvideotexturehelper_p.h"
#include "private/qmultimediautils_p.h"
#include "qffmpeghwaccel_p.h"

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/hdr_dynamic_metadata.h>
#include <libavutil/mastering_display_metadata.h>
}

static bool isFrameFlipped(const AVFrame& frame) {
    for (int i = 0; i < AV_NUM_DATA_POINTERS && frame.data[i]; ++i) {
        if (frame.linesize[i] < 0)
            return true;
    }

    return false;
}

QT_BEGIN_NAMESPACE

QFFmpegVideoBuffer::QFFmpegVideoBuffer(AVFrameUPtr frame, AVRational pixelAspectRatio)
    : QAbstractVideoBuffer(QVideoFrame::NoHandle),
      frame(frame.get()),
      m_size(qCalculateFrameSize({ frame->width, frame->height },
                                 { pixelAspectRatio.num, pixelAspectRatio.den }))
{
    if (frame->hw_frames_ctx) {
        hwFrame = std::move(frame);
        m_pixelFormat = toQtPixelFormat(QFFmpeg::HWAccel::format(hwFrame.get()));
        return;
    }

    swFrame = std::move(frame);
    m_pixelFormat = toQtPixelFormat(AVPixelFormat(swFrame->format));

    convertSWFrame();
}

QFFmpegVideoBuffer::~QFFmpegVideoBuffer() = default;

void QFFmpegVideoBuffer::convertSWFrame()
{
    Q_ASSERT(swFrame);

    const auto actualAVPixelFormat = AVPixelFormat(swFrame->format);
    const auto targetAVPixelFormat = toAVPixelFormat(m_pixelFormat);

    if (actualAVPixelFormat != targetAVPixelFormat || isFrameFlipped(*swFrame)
        || m_size != QSize(swFrame->width, swFrame->height)) {
        Q_ASSERT(toQtPixelFormat(targetAVPixelFormat) == m_pixelFormat);
        // convert the format into something we can handle
        SwsContext *c = sws_getContext(swFrame->width, swFrame->height, actualAVPixelFormat,
                                       m_size.width(), m_size.height(), targetAVPixelFormat,
                                       SWS_BICUBIC, nullptr, nullptr, nullptr);

        auto newFrame = QFFmpeg::makeAVFrame();
        newFrame->width = m_size.width();
        newFrame->height = m_size.height();
        newFrame->format = targetAVPixelFormat;
        av_frame_get_buffer(newFrame.get(), 0);

        sws_scale(c, swFrame->data, swFrame->linesize, 0, swFrame->height, newFrame->data, newFrame->linesize);
        if (frame == swFrame.get())
            frame = newFrame.get();
        swFrame = std::move(newFrame);
        sws_freeContext(c);
    }
}

void QFFmpegVideoBuffer::setTextureConverter(const QFFmpeg::TextureConverter &converter)
{
    textureConverter = converter;
    textureConverter.init(hwFrame.get());
    m_type = converter.isNull() ? QVideoFrame::NoHandle : QVideoFrame::RhiTextureHandle;
}

QVideoFrameFormat::ColorSpace QFFmpegVideoBuffer::colorSpace() const
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
        return QVideoFrameFormat::ColorSpace_Undefined;
    case AVCOL_SPC_RGB:
        return QVideoFrameFormat::ColorSpace_AdobeRgb;
    case AVCOL_SPC_BT709:
        return QVideoFrameFormat::ColorSpace_BT709;
    case AVCOL_SPC_BT470BG: // BT601
    case AVCOL_SPC_SMPTE170M: // Also BT601
        return QVideoFrameFormat::ColorSpace_BT601;
    case AVCOL_SPC_BT2020_NCL: // Non constant luminence
    case AVCOL_SPC_BT2020_CL: // Constant luminence
        return QVideoFrameFormat::ColorSpace_BT2020;
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

float QFFmpegVideoBuffer::maxNits()
{
    float maxNits = -1;
    for (int i = 0; i < frame->nb_side_data; ++i) {
        AVFrameSideData *sd = frame->side_data[i];
        // TODO: Longer term we might want to also support HDR10+ dynamic metadata
        if (sd->type == AV_FRAME_DATA_MASTERING_DISPLAY_METADATA) {
            auto *data = reinterpret_cast<AVMasteringDisplayMetadata *>(sd->data);
            auto maybeLum = QFFmpeg::mul(10'000., data->max_luminance);
            if (maybeLum)
                maxNits = float(maybeLum.value());
        }
    }
    return maxNits;
}

QVideoFrame::MapMode QFFmpegVideoBuffer::mapMode() const
{
    return m_mode;
}

QAbstractVideoBuffer::MapData QFFmpegVideoBuffer::map(QVideoFrame::MapMode mode)
{
    if (!swFrame) {
        Q_ASSERT(hwFrame && hwFrame->hw_frames_ctx);
        swFrame = QFFmpeg::makeAVFrame();
        /* retrieve data from GPU to CPU */
        int ret = av_hwframe_transfer_data(swFrame.get(), hwFrame.get(), 0);
        if (ret < 0) {
            qWarning() << "Error transferring the data to system memory:" << ret;
            return {};
        }
        convertSWFrame();
    }

    m_mode = mode;

    MapData mapData;
    auto *desc = QVideoTextureHelper::textureDescription(pixelFormat());
    mapData.nPlanes = desc->nplanes;
    for (int i = 0; i < mapData.nPlanes; ++i) {
        Q_ASSERT(swFrame->linesize[i] >= 0);

        mapData.data[i] = swFrame->data[i];
        mapData.bytesPerLine[i] = swFrame->linesize[i];
        mapData.size[i] = mapData.bytesPerLine[i]*desc->heightForPlane(swFrame->height, i);
    }
    return mapData;
}

void QFFmpegVideoBuffer::unmap()
{
    // nothing to do here for SW buffers
}

std::unique_ptr<QVideoFrameTextures> QFFmpegVideoBuffer::mapTextures(QRhi *)
{
    if (textures)
        return {};
    if (!hwFrame)
        return {};
    if (textureConverter.isNull()) {
        textures = nullptr;
        return {};
    }

    textures.reset(textureConverter.getTextures(hwFrame.get()));
    if (!textures) {
        static thread_local int lastFormat = 0;
        if (std::exchange(lastFormat, hwFrame->format) != hwFrame->format) // prevent logging spam
            qWarning() << "    failed to get textures for frame; format:" << hwFrame->format;
    }
    return {};
}

quint64 QFFmpegVideoBuffer::textureHandle(int plane) const
{
    return textures ? textures->textureHandle(plane) : 0;
}

QVideoFrameFormat::PixelFormat QFFmpegVideoBuffer::pixelFormat() const
{
    return m_pixelFormat;
}

QSize QFFmpegVideoBuffer::size() const
{
    return m_size;
}

QVideoFrameFormat::PixelFormat QFFmpegVideoBuffer::toQtPixelFormat(AVPixelFormat avPixelFormat, bool *needsConversion)
{
    if (needsConversion)
        *needsConversion = false;

    switch (avPixelFormat) {
    default:
        break;
    case AV_PIX_FMT_NONE:
        Q_ASSERT(!"Invalid avPixelFormat!");
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
    case AV_PIX_FMT_MEDIACODEC:
        return QVideoFrameFormat::Format_SamplerExternalOES;
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

    case QVideoFrameFormat::Format_SamplerExternalOES:
        return AV_PIX_FMT_MEDIACODEC;
    }
}

QT_END_NAMESPACE
