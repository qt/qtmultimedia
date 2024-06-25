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
#include <unordered_set>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>

#ifdef Q_OS_DARWIN
#include <libavutil/hwcontext_videotoolbox.h>
#endif
}

#ifdef Q_OS_ANDROID
#include <QtCore/qjniobject.h>
#include <QtCore/qjniarray.h>
#include <QtCore/qjnitypes.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef Q_OS_ANDROID
Q_DECLARE_JNI_CLASS(QtVideoDeviceManager,
                    "org/qtproject/qt/android/multimedia/QtVideoDeviceManager");

# ifndef QT_DECLARE_JNI_CLASS_STANDARD_TYPES
Q_DECLARE_JNI_CLASS(String, "java/lang/String");
# endif
#endif // Q_OS_ANDROID

Q_STATIC_LOGGING_CATEGORY(qLcFFmpegUtils, "qt.multimedia.ffmpeg.utils");

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
    bool operator()(const AVCodec *a, const AVCodec *b) const
    {
        return a->id < b->id
                || (a->id == b->id && isAVCodecExperimental(a) < isAVCodecExperimental(b));
    }

    bool operator()(const AVCodec *codec, AVCodecID id) const { return codec->id < id; }
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

bool isCodecValid(const AVCodec *codec, const std::vector<AVHWDeviceType> &availableHwDeviceTypes,
                  const std::optional<std::unordered_set<AVCodecID>> &codecAvailableOnDevice)
{
    if (codec->type != AVMEDIA_TYPE_VIDEO)
        return true;

    if (!codec->pix_fmts) {
#if defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
        // Disable V4L2 M2M codecs for encoding for now,
        // TODO: Investigate on how to get them working
        if (std::strstr(codec->name, "_v4l2m2m") && av_codec_is_encoder(codec))
            return false;

        // MediaCodec in Android is used for hardware-accelerated media processing. That is why
        // before marking it as valid, we need to make sure if it is available on current device.
        if (std::strstr(codec->name, "_mediacodec")
            && (codec->capabilities & AV_CODEC_CAP_HARDWARE)
            && codecAvailableOnDevice && codecAvailableOnDevice->count(codec->id) == 0)
            return false;
#endif

        return true; // To be investigated. This happens for RAW_VIDEO, that is supposed to be OK,
        // and with v4l2m2m codecs, that is suspicious.
    }

    if (findAVPixelFormat(codec, &isHwPixelFormat) == AV_PIX_FMT_NONE)
        return true;

    if ((codec->capabilities & AV_CODEC_CAP_HARDWARE) == 0)
        return true;

    auto checkDeviceType = [codec](AVHWDeviceType type) {
        return isAVFormatSupported(codec, pixelFormatForHwDevice(type));
    };

    if (codecAvailableOnDevice && codecAvailableOnDevice->count(codec->id) == 0)
        return false;

    return std::any_of(availableHwDeviceTypes.begin(), availableHwDeviceTypes.end(),
                       checkDeviceType);
}

std::optional<std::unordered_set<AVCodecID>> availableHWCodecs(const CodecStorageType type)
{
#ifdef Q_OS_ANDROID
    using namespace Qt::StringLiterals;
    using namespace QtJniTypes;
    std::unordered_set<AVCodecID> availabeCodecs;

    auto getCodecId = [] (const QString& codecName) {
        if (codecName == "3gpp"_L1) return AV_CODEC_ID_H263;
        if (codecName == "avc"_L1) return AV_CODEC_ID_H264;
        if (codecName == "hevc"_L1) return AV_CODEC_ID_HEVC;
        if (codecName == "mp4v-es"_L1) return AV_CODEC_ID_MPEG4;
        if (codecName == "x-vnd.on2.vp8"_L1) return AV_CODEC_ID_VP8;
        if (codecName == "x-vnd.on2.vp9"_L1) return AV_CODEC_ID_VP9;
        return AV_CODEC_ID_NONE;
    };

    const QJniArray jniCodecs = QtVideoDeviceManager::callStaticMethod<String[]>(
                    type == ENCODERS ? "getHWVideoEncoders" : "getHWVideoDecoders");

    for (const auto &codec : jniCodecs)
        availabeCodecs.insert(getCodecId(codec.toString()));
    return availabeCodecs;
#else
    Q_UNUSED(type);
    return {};
#endif
}

const CodecsStorage &codecsStorage(CodecStorageType codecsType)
{
    static const auto &storages = []() {
        std::array<CodecsStorage, CODEC_STORAGE_TYPE_COUNT> result;
        void *opaque = nullptr;
        const auto platformHwEncoders = availableHWCodecs(ENCODERS);
        const auto platformHwDecoders = availableHWCodecs(DECODERS);

        while (auto codec = av_codec_iterate(&opaque)) {
            // TODO: to be investigated
            // FFmpeg functions avcodec_find_decoder/avcodec_find_encoder
            // find experimental codecs in the last order,
            // now we don't consider them at all since they are supposed to
            // be not stable, maybe we shouldn't.
            // Currently, it's possible to turn them on for testing purposes.

            static const auto experimentalCodecsEnabled =
                    qEnvironmentVariableIntValue("QT_ENABLE_EXPERIMENTAL_CODECS");

            if (!experimentalCodecsEnabled && isAVCodecExperimental(codec)) {
                qCDebug(qLcFFmpegUtils) << "Skip experimental codec" << codec->name;
                continue;
            }

            if (av_codec_is_decoder(codec)) {
                if (isCodecValid(codec, HWAccel::decodingDeviceTypes(), platformHwDecoders))
                    result[DECODERS].emplace_back(codec);
                else
                    qCDebug(qLcFFmpegUtils)
                            << "Skip decoder" << codec->name
                            << "due to disabled matching hw acceleration, or dysfunctional codec";
            }

            if (av_codec_is_encoder(codec)) {
                if (isCodecValid(codec, HWAccel::encodingDeviceTypes(), platformHwEncoders))
                    result[ENCODERS].emplace_back(codec);
                else
                    qCDebug(qLcFFmpegUtils)
                            << "Skip encoder" << codec->name
                            << "due to disabled matching hw acceleration, or dysfunctional codec";
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
            qCDebug(qLcFFmpegUtils) << "Advanced FFmpeg codecs info:";
            for (auto &storage : result) {
                std::for_each(storage.begin(), storage.end(), &dumpCodecInfo);
                qCDebug(qLcFFmpegUtils) << "---------------------------";
            }
        }

        return result;
    }();

    return storages[codecsType];
}

template <typename CodecScoreGetter, typename CodecOpener>
bool findAndOpenCodec(CodecStorageType codecsType, AVCodecID codecId,
                      const CodecScoreGetter &scoreGetter, const CodecOpener &opener)
{
    Q_ASSERT(opener);
    const auto &storage = codecsStorage(codecsType);
    auto it = std::lower_bound(storage.begin(), storage.end(), codecId, CodecsComparator{});

    using CodecToScore = std::pair<const AVCodec *, AVScore>;
    std::vector<CodecToScore> codecsToScores;

    for (; it != storage.end() && (*it)->id == codecId; ++it) {
        const AVScore score = scoreGetter ? scoreGetter(*it) : DefaultAVScore;
        if (score != NotSuitableAVScore)
            codecsToScores.emplace_back(*it, score);
    }

    if (scoreGetter) {
        std::stable_sort(
                codecsToScores.begin(), codecsToScores.end(),
                [](const CodecToScore &a, const CodecToScore &b) { return a.second > b.second; });
    }

    auto open = [&opener](const CodecToScore &codecToScore) { return opener(codecToScore.first); };

    return std::any_of(codecsToScores.begin(), codecsToScores.end(), open);
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

const AVCodec *findAVCodec(CodecStorageType codecsType, AVCodecID codecId,
                           const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(codecsType, codecId, [&](const AVCodec *codec) {
        if (format && !isAVFormatSupported(codec, *format))
            return NotSuitableAVScore;

        return BestAVScore;
    });
}

} // namespace

const AVCodec *findAVDecoder(AVCodecID codecId, const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(DECODERS, codecId, format);
}

const AVCodec *findAVEncoder(AVCodecID codecId, const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(ENCODERS, codecId, format);
}

const AVCodec *findAVEncoder(AVCodecID codecId,
                             const std::function<AVScore(const AVCodec *)> &scoresGetter)
{
    return findAVCodec(ENCODERS, codecId, scoresGetter);
}

bool findAndOpenDecoder(AVCodecID codecId,
                        const std::function<AVScore(const AVCodec *)> &scoresGetter,
                        const std::function<bool(const AVCodec *)> &codecOpener)
{
    return findAndOpenCodec(DECODERS, codecId, scoresGetter, codecOpener);
}

bool findAndOpenEncoder(AVCodecID codecId,
                        const std::function<AVScore(const AVCodec *)> &scoresGetter,
                        const std::function<bool(const AVCodec *)> &codecOpener)
{
    return findAndOpenCodec(ENCODERS, codecId, scoresGetter, codecOpener);
}

bool isAVFormatSupported(const AVCodec *codec, PixelOrSampleFormat format)
{
    if (codec->type == AVMEDIA_TYPE_VIDEO) {
        auto checkFormat = [format](AVPixelFormat f) { return f == format; };
        return findAVPixelFormat(codec, checkFormat) != AV_PIX_FMT_NONE;
    }

    if (codec->type == AVMEDIA_TYPE_AUDIO)
        return hasAVFormat(codec->sample_fmts, AVSampleFormat(format));

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

QT_END_NAMESPACE
