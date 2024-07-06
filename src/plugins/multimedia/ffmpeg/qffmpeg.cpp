// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeg_p.h"

#include <qdebug.h>
#include <qloggingcategory.h>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>

#ifdef Q_OS_DARWIN
#include <libavutil/hwcontext_videotoolbox.h>
#endif
}

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcFFmpegUtils, "qt.multimedia.ffmpeg.utils");

namespace QFFmpeg {

bool isAVFormatSupported(const AVCodec *codec, PixelOrSampleFormat format)
{
    if (codec->type == AVMEDIA_TYPE_VIDEO) {
        auto checkFormat = [format](AVPixelFormat f) { return f == format; };
        return findAVPixelFormat(codec, checkFormat) != AV_PIX_FMT_NONE;
    }

    if (codec->type == AVMEDIA_TYPE_AUDIO)
        return hasAVValue(codec->sample_fmts, AVSampleFormat(format));

    return false;
}

bool isHwPixelFormat(AVPixelFormat format)
{
    const auto desc = av_pix_fmt_desc_get(format);
    return desc && (desc->flags & AV_PIX_FMT_FLAG_HWACCEL) != 0;
}

bool isAVCodecExperimental(const AVCodec *codec)
{
    return (codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL) != 0;
}

void applyExperimentalCodecOptions(const AVCodec *codec, AVDictionary** opts)
{
    if (isAVCodecExperimental(codec)) {
        qCWarning(qLcFFmpegUtils) << "Applying the option 'strict -2' for the experimental codec"
                                  << codec->name << ". it's unlikely to work properly";
        av_dict_set(opts, "strict", "-2", 0);
    }
}

AVPixelFormat pixelFormatForHwDevice(AVHWDeviceType deviceType)
{
    switch (deviceType) {
    case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
        return AV_PIX_FMT_VIDEOTOOLBOX;
    case AV_HWDEVICE_TYPE_VAAPI:
        return AV_PIX_FMT_VAAPI;
    case AV_HWDEVICE_TYPE_MEDIACODEC:
        return AV_PIX_FMT_MEDIACODEC;
    case AV_HWDEVICE_TYPE_CUDA:
        return AV_PIX_FMT_CUDA;
    case AV_HWDEVICE_TYPE_VDPAU:
        return AV_PIX_FMT_VDPAU;
    case AV_HWDEVICE_TYPE_OPENCL:
        return AV_PIX_FMT_OPENCL;
    case AV_HWDEVICE_TYPE_QSV:
        return AV_PIX_FMT_QSV;
    case AV_HWDEVICE_TYPE_D3D11VA:
        return AV_PIX_FMT_D3D11;
#if QT_FFMPEG_HAS_D3D12VA
    case AV_HWDEVICE_TYPE_D3D12VA:
        return AV_PIX_FMT_D3D12;
#endif
    case AV_HWDEVICE_TYPE_DXVA2:
        return AV_PIX_FMT_DXVA2_VLD;
    case AV_HWDEVICE_TYPE_DRM:
        return AV_PIX_FMT_DRM_PRIME;
#if QT_FFMPEG_HAS_VULKAN
    case AV_HWDEVICE_TYPE_VULKAN:
        return AV_PIX_FMT_VULKAN;
#endif
    default:
        return AV_PIX_FMT_NONE;
    }
}

AVPacketSideData *addStreamSideData(AVStream *stream, AVPacketSideData sideData)
{
    QScopeGuard freeData([&sideData]() { av_free(sideData.data); });
#if QT_FFMPEG_STREAM_SIDE_DATA_DEPRECATED
    AVPacketSideData *result = av_packet_side_data_add(
                                          &stream->codecpar->coded_side_data,
                                          &stream->codecpar->nb_coded_side_data,
                                          sideData.type,
                                          sideData.data,
                                          sideData.size,
                                          0);
    if (result) {
        // If the result is not null, the ownership is taken by AVStream,
        // otherwise the data must be deleted.
        freeData.dismiss();
        return result;
    }
#else
    Q_UNUSED(stream);
    // TODO: implement for older FFmpeg versions
    qWarning() << "Adding stream side data is not supported for FFmpeg < 6.1";
#endif

    return nullptr;
}

const AVPacketSideData *streamSideData(const AVStream *stream, AVPacketSideDataType type)
{
    Q_ASSERT(stream);

#if QT_FFMPEG_STREAM_SIDE_DATA_DEPRECATED
    return av_packet_side_data_get(stream->codecpar->coded_side_data,
                                   stream->codecpar->nb_coded_side_data, type);
#else
    auto checkType = [type](const auto &item) { return item.type == type; };
    const auto end = stream->side_data + stream->nb_side_data;
    const auto found = std::find_if(stream->side_data, end, checkType);
    return found == end ? nullptr : found;
#endif
}

SwrContextUPtr createResampleContext(const AVAudioFormat &inputFormat,
                                     const AVAudioFormat &outputFormat)
{
    SwrContext *resampler = nullptr;
#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    resampler = swr_alloc_set_opts(nullptr,
                                   outputFormat.channelLayoutMask,
                                   outputFormat.sampleFormat,
                                   outputFormat.sampleRate,
                                   inputFormat.channelLayoutMask,
                                   inputFormat.sampleFormat,
                                   inputFormat.sampleRate,
                                   0,
                                   nullptr);
#else

#if QT_FFMPEG_SWR_CONST_CH_LAYOUT
    using AVChannelLayoutPrm = const AVChannelLayout*;
#else
    using AVChannelLayoutPrm = AVChannelLayout*;
#endif

    swr_alloc_set_opts2(&resampler,
                        const_cast<AVChannelLayoutPrm>(&outputFormat.channelLayout),
                        outputFormat.sampleFormat,
                        outputFormat.sampleRate,
                        const_cast<AVChannelLayoutPrm>(&inputFormat.channelLayout),
                        inputFormat.sampleFormat,
                        inputFormat.sampleRate,
                        0,
                        nullptr);
#endif

    swr_init(resampler);
    return SwrContextUPtr(resampler);
}

QVideoFrameFormat::ColorTransfer fromAvColorTransfer(AVColorTransferCharacteristic colorTrc) {
    switch (colorTrc) {
    case AVCOL_TRC_BT709:
    // The following three cases have transfer characteristics identical to BT709
    case AVCOL_TRC_BT1361_ECG:
    case AVCOL_TRC_BT2020_10:
    case AVCOL_TRC_BT2020_12:
    case AVCOL_TRC_SMPTE240M: // almost identical to bt709
        return QVideoFrameFormat::ColorTransfer_BT709;
    case AVCOL_TRC_GAMMA22:
    case AVCOL_TRC_SMPTE428: // No idea, let's hope for the best...
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

AVColorTransferCharacteristic toAvColorTransfer(QVideoFrameFormat::ColorTransfer colorTrc)
{
    switch (colorTrc) {
    case QVideoFrameFormat::ColorTransfer_BT709:
        return AVCOL_TRC_BT709;
    case QVideoFrameFormat::ColorTransfer_BT601:
        return AVCOL_TRC_BT709; // which one is the best?
    case QVideoFrameFormat::ColorTransfer_Linear:
        return AVCOL_TRC_SMPTE2084;
    case QVideoFrameFormat::ColorTransfer_Gamma22:
        return AVCOL_TRC_GAMMA22;
    case QVideoFrameFormat::ColorTransfer_Gamma28:
        return AVCOL_TRC_GAMMA28;
    case QVideoFrameFormat::ColorTransfer_ST2084:
        return AVCOL_TRC_SMPTE2084;
    case QVideoFrameFormat::ColorTransfer_STD_B67:
        return AVCOL_TRC_ARIB_STD_B67;
    default:
        return AVCOL_TRC_UNSPECIFIED;
    }
}

QVideoFrameFormat::ColorSpace fromAvColorSpace(AVColorSpace colorSpace)
{
    switch (colorSpace) {
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

AVColorSpace toAvColorSpace(QVideoFrameFormat::ColorSpace colorSpace)
{
    switch (colorSpace) {
    case QVideoFrameFormat::ColorSpace_BT601:
        return AVCOL_SPC_BT470BG;
    case QVideoFrameFormat::ColorSpace_BT709:
        return AVCOL_SPC_BT709;
    case QVideoFrameFormat::ColorSpace_AdobeRgb:
        return AVCOL_SPC_RGB;
    case QVideoFrameFormat::ColorSpace_BT2020:
        return AVCOL_SPC_BT2020_CL;
    default:
        return AVCOL_SPC_UNSPECIFIED;
    }
}

QVideoFrameFormat::ColorRange fromAvColorRange(AVColorRange colorRange)
{
    switch (colorRange) {
    case AVCOL_RANGE_MPEG:
        return QVideoFrameFormat::ColorRange_Video;
    case AVCOL_RANGE_JPEG:
        return QVideoFrameFormat::ColorRange_Full;
    default:
        return QVideoFrameFormat::ColorRange_Unknown;
    }
}

AVColorRange toAvColorRange(QVideoFrameFormat::ColorRange colorRange)
{
    switch (colorRange) {
    case QVideoFrameFormat::ColorRange_Video:
        return AVCOL_RANGE_MPEG;
    case QVideoFrameFormat::ColorRange_Full:
        return AVCOL_RANGE_JPEG;
    default:
        return AVCOL_RANGE_UNSPECIFIED;
    }
}

AVHWDeviceContext* avFrameDeviceContext(const AVFrame* frame) {
    if (!frame)
        return {};
    if (!frame->hw_frames_ctx)
        return {};

    const auto *frameCtx = reinterpret_cast<AVHWFramesContext *>(frame->hw_frames_ctx->data);
    if (!frameCtx)
        return {};

    return frameCtx->device_ctx;
}

#ifdef Q_OS_DARWIN
bool isCVFormatSupported(uint32_t cvFormat)
{
    return av_map_videotoolbox_format_to_pixfmt(cvFormat) != AV_PIX_FMT_NONE;
}

std::string cvFormatToString(uint32_t cvFormat)
{
    auto formatDescIt = std::make_reverse_iterator(reinterpret_cast<const char *>(&cvFormat));
    return std::string(formatDescIt - 4, formatDescIt);
}

#endif

} // namespace QFFmpeg

QDebug operator<<(QDebug dbg, const AVRational &value)
{
    dbg << value.num << "/" << value.den;
    return dbg;
}

#if !QT_FFMPEG_OLD_CHANNEL_LAYOUT
QDebug operator<<(QDebug dbg, const AVChannelLayout &layout)
{
    dbg << '[';
    dbg << "nb_channels:" << layout.nb_channels;
    dbg << ", order:" << layout.order;

    if (layout.order == AV_CHANNEL_ORDER_NATIVE || layout.order == AV_CHANNEL_ORDER_AMBISONIC)
        dbg << ", mask:" << Qt::bin << layout.u.mask << Qt::dec;
    else if (layout.order == AV_CHANNEL_ORDER_CUSTOM && layout.u.map)
        dbg << ", id: " << layout.u.map->id;

    dbg << ']';

    return dbg;
}
#endif

QT_END_NAMESPACE
