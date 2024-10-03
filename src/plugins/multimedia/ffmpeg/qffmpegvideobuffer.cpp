// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideobuffer_p.h"
#include "private/qvideotexturehelper_p.h"
#include "private/qmultimediautils_p.h"
#include "qffmpeghwaccel_p.h"
#include "qloggingcategory.h"

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/hdr_dynamic_metadata.h>
#include <libavutil/mastering_display_metadata.h>
}

QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

static bool isFrameFlipped(const AVFrame& frame) {
    for (int i = 0; i < AV_NUM_DATA_POINTERS && frame.data[i]; ++i) {
        if (frame.linesize[i] < 0)
            return true;
    }

    return false;
}

Q_STATIC_LOGGING_CATEGORY(qLcFFmpegVideoBuffer, "qt.multimedia.ffmpeg.videobuffer");

QFFmpegVideoBuffer::QFFmpegVideoBuffer(AVFrameUPtr frame, AVRational pixelAspectRatio)
    : QHwVideoBuffer(QVideoFrame::NoHandle),
      m_frame(frame.get()),
      m_size(qCalculateFrameSize({ frame->width, frame->height },
                                 { pixelAspectRatio.num, pixelAspectRatio.den }))
{
    if (frame->hw_frames_ctx) {
        m_hwFrame = std::move(frame);
        m_pixelFormat = toQtPixelFormat(HWAccel::format(m_hwFrame.get()));
        return;
    }

    m_swFrame = std::move(frame);
    m_pixelFormat = toQtPixelFormat(AVPixelFormat(m_swFrame->format));

    convertSWFrame();
}

QFFmpegVideoBuffer::~QFFmpegVideoBuffer() = default;

void QFFmpegVideoBuffer::convertSWFrame()
{
    Q_ASSERT(m_swFrame);

    const auto actualAVPixelFormat = AVPixelFormat(m_swFrame->format);
    const auto targetAVPixelFormat = toAVPixelFormat(m_pixelFormat);

    const QSize actualSize(m_swFrame->width, m_swFrame->height);
    if (actualAVPixelFormat != targetAVPixelFormat || isFrameFlipped(*m_swFrame)
        || m_size != actualSize) {
        Q_ASSERT(toQtPixelFormat(targetAVPixelFormat) == m_pixelFormat);
        // convert the format into something we can handle
        SwsContextUPtr scaleContext = createSwsContext(actualSize, actualAVPixelFormat, m_size,
                                                       targetAVPixelFormat, SWS_BICUBIC);

        auto newFrame = makeAVFrame();
        newFrame->width = m_size.width();
        newFrame->height = m_size.height();
        newFrame->format = targetAVPixelFormat;
        av_frame_get_buffer(newFrame.get(), 0);

        sws_scale(scaleContext.get(), m_swFrame->data, m_swFrame->linesize, 0, m_swFrame->height,
                  newFrame->data, newFrame->linesize);
        if (m_frame == m_swFrame.get())
            m_frame = newFrame.get();
        m_swFrame = std::move(newFrame);
    }
}

void QFFmpegVideoBuffer::setTextureConverter(const TextureConverter &converter)
{
    m_textureConverter = converter;
    m_textureConverter.init(m_hwFrame.get());
    m_type = converter.isNull() ? QVideoFrame::NoHandle : QVideoFrame::RhiTextureHandle;
}

QVideoFrameFormat::ColorSpace QFFmpegVideoBuffer::colorSpace() const
{
    return fromAvColorSpace(m_frame->colorspace);
}

QVideoFrameFormat::ColorTransfer QFFmpegVideoBuffer::colorTransfer() const
{
    return fromAvColorTransfer(m_frame->color_trc);
}

QVideoFrameFormat::ColorRange QFFmpegVideoBuffer::colorRange() const
{
    return fromAvColorRange(m_frame->color_range);
}

float QFFmpegVideoBuffer::maxNits()
{
    float maxNits = -1;
    for (int i = 0; i < m_frame->nb_side_data; ++i) {
        AVFrameSideData *sd = m_frame->side_data[i];
        // TODO: Longer term we might want to also support HDR10+ dynamic metadata
        if (sd->type == AV_FRAME_DATA_MASTERING_DISPLAY_METADATA) {
            auto *data = reinterpret_cast<AVMasteringDisplayMetadata *>(sd->data);
            auto maybeLum = QFFmpeg::mul(qreal(10'000.), data->max_luminance);
            if (maybeLum)
                maxNits = float(maybeLum.value());
        }
    }
    return maxNits;
}

QAbstractVideoBuffer::MapData QFFmpegVideoBuffer::map(QVideoFrame::MapMode mode)
{
    if (!m_swFrame) {
        Q_ASSERT(m_hwFrame && m_hwFrame->hw_frames_ctx);
        m_swFrame = makeAVFrame();
        /* retrieve data from GPU to CPU */
        int ret = av_hwframe_transfer_data(m_swFrame.get(), m_hwFrame.get(), 0);
        if (ret < 0) {
            qWarning() << "Error transferring the data to system memory:" << ret;
            return {};
        }
        convertSWFrame();
    }

    m_mode = mode;

    MapData mapData;
    auto *desc = QVideoTextureHelper::textureDescription(pixelFormat());
    mapData.planeCount = desc->nplanes;
    for (int i = 0; i < mapData.planeCount; ++i) {
        Q_ASSERT(m_swFrame->linesize[i] >= 0);

        mapData.data[i] = m_swFrame->data[i];
        mapData.bytesPerLine[i] = m_swFrame->linesize[i];
        mapData.dataSize[i] = mapData.bytesPerLine[i]*desc->heightForPlane(m_swFrame->height, i);
    }

    if ((mode & QVideoFrame::WriteOnly) != 0 && m_hwFrame) {
        m_type = QVideoFrame::NoHandle;
        m_hwFrame.reset();
        if (m_textures) {
            qCDebug(qLcFFmpegVideoBuffer)
                    << "Mapping of FFmpeg video buffer with write mode when "
                       "textures have been created. Visual artifacts might "
                       "happen if the frame is still in the rendering pipeline";
            m_textures.reset();
        }
    }

    return mapData;
}

void QFFmpegVideoBuffer::unmap()
{
    // nothing to do here for SW buffers.
    // Set NotMapped mode to ensure map/unmap/mapMode consisteny.
    m_mode = QVideoFrame::NotMapped;
}

std::unique_ptr<QVideoFrameTextures> QFFmpegVideoBuffer::mapTextures(QRhi *)
{
    if (m_textures)
        return {};
    if (!m_hwFrame)
        return {};
    if (m_textureConverter.isNull()) {
        m_textures = nullptr;
        return {};
    }

    m_textures.reset(m_textureConverter.getTextures(m_hwFrame.get()));
    if (!m_textures) {
        static thread_local int lastFormat = 0;
        if (std::exchange(lastFormat, m_hwFrame->format) != m_hwFrame->format) // prevent logging spam
            qWarning() << "    failed to get textures for frame; format:" << m_hwFrame->format;
    }
    return {};
}

quint64 QFFmpegVideoBuffer::textureHandle(QRhi *rhi, int plane) const
{
    return m_textures ? m_textures->textureHandle(rhi, plane) : 0;
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
        return AV_PIX_FMT_ARGB;
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case QVideoFrameFormat::Format_XRGB8888:
        return AV_PIX_FMT_0RGB;
    case QVideoFrameFormat::Format_BGRA8888:
        return AV_PIX_FMT_BGRA;
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
        return AV_PIX_FMT_BGR0;
    case QVideoFrameFormat::Format_ABGR8888:
        return AV_PIX_FMT_ABGR;
    case QVideoFrameFormat::Format_XBGR8888:
        return AV_PIX_FMT_0BGR;
    case QVideoFrameFormat::Format_RGBA8888:
        return AV_PIX_FMT_RGBA;
    // to be added in 6.8:
    // case QVideoFrameFormat::Format_RGBA8888_Premultiplied:
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
