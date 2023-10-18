// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeg_p.h"

#include <qdebug.h>
#include <qloggingcategory.h>
#include <qffmpeghwaccel_p.h> // TODO: probably decompose HWAccel and get rid of the header in the base utils

#include <algorithm>
#include <vector>
#include <array>
#include <optional>

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

namespace {

enum CodecStorageType {
    ENCODERS,
    DECODERS,

    // TODO: maybe split sw/hw codecs

    CODEC_STORAGE_TYPE_COUNT
};

using CodecsStorage = std::vector<const AVCodec *>;

struct CodecsComparator
{
    bool operator()(const AVCodec *a, const AVCodec *b) const { return a->id < b->id; }

    bool operator()(const AVCodec *a, AVCodecID id) const { return a->id < id; }
};

template<typename FlagNames>
QString flagsToString(int flags, const FlagNames &flagNames)
{
    QString result;
    int leftover = flags;
    for (const auto &flagAndName : flagNames)
        if ((flags & flagAndName.first) != 0) {
            leftover &= ~flagAndName.first;
            if (!result.isEmpty())
                result += ", ";
            result += flagAndName.second;
        }

    if (leftover) {
        if (!result.isEmpty())
            result += ", ";
        result += QString::number(leftover, 16);
    }
    return result;
}

void dumpCodecInfo(const AVCodec *codec)
{
    using FlagNames = std::initializer_list<std::pair<int, const char *>>;
    const auto mediaType = codec->type == AVMEDIA_TYPE_VIDEO ? "video"
            : codec->type == AVMEDIA_TYPE_AUDIO              ? "audio"
            : codec->type == AVMEDIA_TYPE_SUBTITLE           ? "subtitle"
                                                             : "other_type";

    const auto type = av_codec_is_encoder(codec)
            ? av_codec_is_decoder(codec) ? "encoder/decoder:" : "encoder:"
            : "decoder:";

    static const FlagNames capabilitiesNames = {
        { AV_CODEC_CAP_DRAW_HORIZ_BAND, "DRAW_HORIZ_BAND" },
        { AV_CODEC_CAP_DR1, "DRAW_HORIZ_DR1" },
        { AV_CODEC_CAP_DELAY, "DELAY" },
        { AV_CODEC_CAP_SMALL_LAST_FRAME, "SMALL_LAST_FRAME" },
        { AV_CODEC_CAP_SUBFRAMES, "SUBFRAMES" },
        { AV_CODEC_CAP_EXPERIMENTAL, "EXPERIMENTAL" },
        { AV_CODEC_CAP_CHANNEL_CONF, "CHANNEL_CONF" },
        { AV_CODEC_CAP_FRAME_THREADS, "FRAME_THREADS" },
        { AV_CODEC_CAP_SLICE_THREADS, "SLICE_THREADS" },
        { AV_CODEC_CAP_PARAM_CHANGE, "PARAM_CHANGE" },
#ifdef AV_CODEC_CAP_OTHER_THREADS
        { AV_CODEC_CAP_OTHER_THREADS, "OTHER_THREADS" },
#endif
        { AV_CODEC_CAP_VARIABLE_FRAME_SIZE, "VARIABLE_FRAME_SIZE" },
        { AV_CODEC_CAP_AVOID_PROBING, "AVOID_PROBING" },
        { AV_CODEC_CAP_HARDWARE, "HARDWARE" },
        { AV_CODEC_CAP_HYBRID, "HYBRID" },
        { AV_CODEC_CAP_ENCODER_REORDERED_OPAQUE, "ENCODER_REORDERED_OPAQUE" },
#ifdef AV_CODEC_CAP_ENCODER_FLUSH
        { AV_CODEC_CAP_ENCODER_FLUSH, "ENCODER_FLUSH" },
#endif
    };

    qCDebug(qLcFFmpegUtils) << mediaType << type << codec->name << "id:" << codec->id
                            << "capabilities:"
                            << flagsToString(codec->capabilities, capabilitiesNames);

    if (codec->pix_fmts) {
        static const FlagNames flagNames = {
            { AV_PIX_FMT_FLAG_BE, "BE" },
            { AV_PIX_FMT_FLAG_PAL, "PAL" },
            { AV_PIX_FMT_FLAG_BITSTREAM, "BITSTREAM" },
            { AV_PIX_FMT_FLAG_HWACCEL, "HWACCEL" },
            { AV_PIX_FMT_FLAG_PLANAR, "PLANAR" },
            { AV_PIX_FMT_FLAG_RGB, "RGB" },
            { AV_PIX_FMT_FLAG_ALPHA, "ALPHA" },
            { AV_PIX_FMT_FLAG_BAYER, "BAYER" },
            { AV_PIX_FMT_FLAG_FLOAT, "FLOAT" },
        };

        qCDebug(qLcFFmpegUtils) << "  pix_fmts:";
        for (auto f = codec->pix_fmts; *f != AV_PIX_FMT_NONE; ++f) {
            auto desc = av_pix_fmt_desc_get(*f);
            qCDebug(qLcFFmpegUtils)
                    << "    id:" << *f << desc->name << "depth:" << desc->comp[0].depth
                    << "flags:" << flagsToString(desc->flags, flagNames);
        }
    } else if (codec->type == AVMEDIA_TYPE_VIDEO) {
        qCDebug(qLcFFmpegUtils) << "  pix_fmts: null";
    }

    if (codec->sample_fmts) {
        qCDebug(qLcFFmpegUtils) << "  sample_fmts:";
        for (auto f = codec->sample_fmts; *f != AV_SAMPLE_FMT_NONE; ++f) {
            const auto name = av_get_sample_fmt_name(*f);
            qCDebug(qLcFFmpegUtils) << "    id:" << *f << (name ? name : "unknown")
                                    << "bytes_per_sample:" << av_get_bytes_per_sample(*f)
                                    << "is_planar:" << av_sample_fmt_is_planar(*f);
        }
    } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
        qCDebug(qLcFFmpegUtils) << "  sample_fmts: null";
    }

    if (avcodec_get_hw_config(codec, 0)) {
        static const FlagNames hwConfigMethodNames = {
            { AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX, "HW_DEVICE_CTX" },
            { AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX, "HW_FRAMES_CTX" },
            { AV_CODEC_HW_CONFIG_METHOD_INTERNAL, "INTERNAL" },
            { AV_CODEC_HW_CONFIG_METHOD_AD_HOC, "AD_HOC" }
        };

        qCDebug(qLcFFmpegUtils) << "  hw config:";
        for (int index = 0; auto config = avcodec_get_hw_config(codec, index); ++index) {
            const auto pixFmtForDevice = pixelFormatForHwDevice(config->device_type);
            auto pixFmtDesc = av_pix_fmt_desc_get(config->pix_fmt);
            auto pixFmtForDeviceDesc = av_pix_fmt_desc_get(pixFmtForDevice);
            qCDebug(qLcFFmpegUtils)
                    << "    device_type:" << config->device_type << "pix_fmt:" << config->pix_fmt
                    << (pixFmtDesc ? pixFmtDesc->name : "unknown")
                    << "pixelFormatForHwDevice:" << pixelFormatForHwDevice(config->device_type)
                    << (pixFmtForDeviceDesc ? pixFmtForDeviceDesc->name : "unknown")
                    << "hw_config_methods:" << flagsToString(config->methods, hwConfigMethodNames);
        }
    }
}

bool isCodecValid(const AVCodec *codec, const std::vector<AVHWDeviceType> &availableHwDeviceTypes)
{
    if (codec->type != AVMEDIA_TYPE_VIDEO)
        return true;

    const auto pixFmts = codec->pix_fmts;

    if (!pixFmts)
        return true; // To be investigated. This happens for RAW_VIDEO, that is supposed to be OK,
                     // and with v4l2m2m codecs, that is suspicious.

    if (findAVFormat(pixFmts, &isHwPixelFormat) == AV_PIX_FMT_NONE)
        return true;

    if ((codec->capabilities & AV_CODEC_CAP_HARDWARE) == 0)
        return true;

    auto checkDeviceType = [pixFmts](AVHWDeviceType type) {
        return hasAVFormat(pixFmts, pixelFormatForHwDevice(type));
    };

    return std::any_of(availableHwDeviceTypes.begin(), availableHwDeviceTypes.end(),
                       checkDeviceType);
}

const CodecsStorage &codecsStorage(CodecStorageType codecsType)
{
    static const auto &storages = []() {
        std::array<CodecsStorage, CODEC_STORAGE_TYPE_COUNT> result;
        void *opaque = nullptr;

        while (auto codec = av_codec_iterate(&opaque)) {
            // TODO: to be investigated
            // FFmpeg functions avcodec_find_decoder/avcodec_find_encoder
            // find experimental codecs in the last order,
            // now we don't consider them at all since they are supposed to
            // be not stable, maybe we shouldn't.
            if (codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL) {
                qCDebug(qLcFFmpegUtils) << "Skip experimental codec" << codec->name;
                continue;
            }

            if (av_codec_is_decoder(codec)) {
                if (isCodecValid(codec, HWAccel::decodingDeviceTypes()))
                    result[DECODERS].emplace_back(codec);
                else
                    qCDebug(qLcFFmpegUtils) << "Skip decoder" << codec->name
                                            << "due to disabled matching hw acceleration";
            }

            if (av_codec_is_encoder(codec)) {
                if (isCodecValid(codec, HWAccel::encodingDeviceTypes()))
                    result[ENCODERS].emplace_back(codec);
                else
                    qCDebug(qLcFFmpegUtils) << "Skip encoder" << codec->name
                                            << "due to disabled matching hw acceleration";
            }
        }

        for (auto &storage : result) {
            storage.shrink_to_fit();

            // we should ensure the original order
            std::stable_sort(storage.begin(), storage.end(), CodecsComparator{});
        }

        // It print pretty much logs, so let's print it only for special case
        const bool shouldDumpCodecsInfo = qLcFFmpegUtils().isEnabled(QtDebugMsg)
                && qEnvironmentVariableIsSet("QT_FFMPEG_DEBUG");

        if (shouldDumpCodecsInfo) {
            qCDebug(qLcFFmpegUtils) << "Advanced ffmpeg codecs info:";
            for (auto &storage : result) {
                std::for_each(storage.begin(), storage.end(), &dumpCodecInfo);
                qCDebug(qLcFFmpegUtils) << "---------------------------";
            }
        }

        return result;
    }();

    return storages[codecsType];
}

const char *preferredHwCodecNameSuffix(bool isEncoder, AVHWDeviceType deviceType)
{
    switch (deviceType) {
    case AV_HWDEVICE_TYPE_VAAPI:
        return "_vaapi";
    case AV_HWDEVICE_TYPE_MEDIACODEC:
        return "_mediacodec";
    case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
        return "_videotoolbox";
    case AV_HWDEVICE_TYPE_D3D11VA:
    case AV_HWDEVICE_TYPE_DXVA2:
        return "_mf";
    case AV_HWDEVICE_TYPE_CUDA:
    case AV_HWDEVICE_TYPE_VDPAU:
        return isEncoder ? "_nvenc" : "_cuvid";
    default:
        return nullptr;
    }
}

template<typename CodecScoreGetter>
const AVCodec *findAVCodec(CodecStorageType codecsType, AVCodecID codecId,
                           const CodecScoreGetter &scoreGetter)
{
    const auto &storage = codecsStorage(codecsType);
    auto it = std::lower_bound(storage.begin(), storage.end(), codecId, CodecsComparator{});

    const AVCodec *result = nullptr;
    AVScore resultScore = NotSuitableAVScore;

    for (; it != storage.end() && (*it)->id == codecId && resultScore != BestAVScore; ++it) {
        const auto score = scoreGetter(*it);

        if (score > resultScore) {
            resultScore = score;
            result = *it;
        }
    }

    return result;
}

AVScore hwCodecNameScores(const AVCodec *codec, AVHWDeviceType deviceType)
{
    if (auto suffix = preferredHwCodecNameSuffix(av_codec_is_encoder(codec), deviceType)) {
        const auto substr = strstr(codec->name, suffix);
        if (substr && !substr[strlen(suffix)])
            return BestAVScore;

        return DefaultAVScore;
    }

    return BestAVScore;
}

const AVCodec *findAVCodec(CodecStorageType codecsType, AVCodecID codecId,
                           const std::optional<AVHWDeviceType> &deviceType,
                           const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(codecsType, codecId, [&](const AVCodec *codec) {
        if (format && !isAVFormatSupported(codec, *format))
            return NotSuitableAVScore;

        if (!deviceType)
            return BestAVScore; // find any codec with the id

        if (*deviceType == AV_HWDEVICE_TYPE_NONE
            && findAVFormat(codec->pix_fmts, &isSwPixelFormat) != AV_PIX_FMT_NONE)
            return BestAVScore;

        if (*deviceType != AV_HWDEVICE_TYPE_NONE) {
            for (int index = 0; auto config = avcodec_get_hw_config(codec, index); ++index) {
                if (config->device_type != deviceType)
                    continue;

                if (format && config->pix_fmt != AV_PIX_FMT_NONE && config->pix_fmt != *format)
                    continue;

                return hwCodecNameScores(codec, *deviceType);
            }

            // The situation happens mostly with encoders
            // Probably, it's ffmpeg bug: avcodec_get_hw_config returns null even though
            // hw acceleration is supported
            if (hasAVFormat(codec->pix_fmts, pixelFormatForHwDevice(*deviceType)))
                return hwCodecNameScores(codec, *deviceType);
        }

        return NotSuitableAVScore;
    });
}

} // namespace

const AVCodec *findAVDecoder(AVCodecID codecId, const std::optional<AVHWDeviceType> &deviceType,
                             const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(DECODERS, codecId, deviceType, format);
}

const AVCodec *findAVEncoder(AVCodecID codecId, const std::optional<AVHWDeviceType> &deviceType,
                             const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(ENCODERS, codecId, deviceType, format);
}

const AVCodec *findAVEncoder(AVCodecID codecId,
                             const std::function<AVScore(const AVCodec *)> &scoresGetter)
{
    return findAVCodec(ENCODERS, codecId, scoresGetter);
}

bool isAVFormatSupported(const AVCodec *codec, PixelOrSampleFormat format)
{
    if (codec->type == AVMEDIA_TYPE_VIDEO)
        return hasAVFormat(codec->pix_fmts, AVPixelFormat(format));

    if (codec->type == AVMEDIA_TYPE_AUDIO)
        return hasAVFormat(codec->sample_fmts, AVSampleFormat(format));

    return false;
}

bool isHwPixelFormat(AVPixelFormat format)
{
    const auto desc = av_pix_fmt_desc_get(format);
    return desc && (desc->flags & AV_PIX_FMT_FLAG_HWACCEL) != 0;
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

QT_END_NAMESPACE
