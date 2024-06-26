// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegcodecstorage_p.h"

#include "qffmpeg_p.h"
#include "qffmpeghwaccel_p.h"

#include <qdebug.h>
#include <qloggingcategory.h>

#include <algorithm>
#include <vector>
#include <array>

#include <unordered_set>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
}

#ifdef Q_OS_ANDROID
#  include <QtCore/qjniobject.h>
#  include <QtCore/qjniarray.h>
#  include <QtCore/qjnitypes.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef Q_OS_ANDROID
Q_DECLARE_JNI_CLASS(QtVideoDeviceManager,
                    "org/qtproject/qt/android/multimedia/QtVideoDeviceManager");

#  ifndef QT_DECLARE_JNI_CLASS_STANDARD_TYPES
Q_DECLARE_JNI_CLASS(String, "java/lang/String");
#  endif
#endif // Q_OS_ANDROID

Q_STATIC_LOGGING_CATEGORY(qLcCodecStorage, "qt.multimedia.ffmpeg.codecstorage");

namespace QFFmpeg {

namespace {

enum CodecStorageType {
    Encoders,
    Decoders,

    // TODO: maybe split sw/hw codecs

    CodecStorageTypeCount
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

template <typename FlagNames>
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

    qCDebug(qLcCodecStorage) << mediaType << type << codec->name << "id:" << codec->id
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

        qCDebug(qLcCodecStorage) << "  pix_fmts:";
        for (auto f = codec->pix_fmts; *f != AV_PIX_FMT_NONE; ++f) {
            auto desc = av_pix_fmt_desc_get(*f);
            qCDebug(qLcCodecStorage)
                    << "    id:" << *f << desc->name << "depth:" << desc->comp[0].depth
                    << "flags:" << flagsToString(desc->flags, flagNames);
        }
    } else if (codec->type == AVMEDIA_TYPE_VIDEO) {
        qCDebug(qLcCodecStorage) << "  pix_fmts: null";
    }

    if (codec->sample_fmts) {
        qCDebug(qLcCodecStorage) << "  sample_fmts:";
        for (auto f = codec->sample_fmts; *f != AV_SAMPLE_FMT_NONE; ++f) {
            const auto name = av_get_sample_fmt_name(*f);
            qCDebug(qLcCodecStorage) << "    id:" << *f << (name ? name : "unknown")
                                     << "bytes_per_sample:" << av_get_bytes_per_sample(*f)
                                     << "is_planar:" << av_sample_fmt_is_planar(*f);
        }
    } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
        qCDebug(qLcCodecStorage) << "  sample_fmts: null";
    }

    if (avcodec_get_hw_config(codec, 0)) {
        static const FlagNames hwConfigMethodNames = {
            { AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX, "HW_DEVICE_CTX" },
            { AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX, "HW_FRAMES_CTX" },
            { AV_CODEC_HW_CONFIG_METHOD_INTERNAL, "INTERNAL" },
            { AV_CODEC_HW_CONFIG_METHOD_AD_HOC, "AD_HOC" }
        };

        qCDebug(qLcCodecStorage) << "  hw config:";
        for (int index = 0; auto config = avcodec_get_hw_config(codec, index); ++index) {
            const auto pixFmtForDevice = pixelFormatForHwDevice(config->device_type);
            auto pixFmtDesc = av_pix_fmt_desc_get(config->pix_fmt);
            auto pixFmtForDeviceDesc = av_pix_fmt_desc_get(pixFmtForDevice);
            qCDebug(qLcCodecStorage)
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
        if (std::strstr(codec->name, "_mediacodec") && (codec->capabilities & AV_CODEC_CAP_HARDWARE)
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

    auto getCodecId = [](const QString &codecName) {
        if (codecName == "3gpp"_L1)
            return AV_CODEC_ID_H263;
        if (codecName == "avc"_L1)
            return AV_CODEC_ID_H264;
        if (codecName == "hevc"_L1)
            return AV_CODEC_ID_HEVC;
        if (codecName == "mp4v-es"_L1)
            return AV_CODEC_ID_MPEG4;
        if (codecName == "x-vnd.on2.vp8"_L1)
            return AV_CODEC_ID_VP8;
        if (codecName == "x-vnd.on2.vp9"_L1)
            return AV_CODEC_ID_VP9;
        return AV_CODEC_ID_NONE;
    };

    const QJniArray jniCodecs = QtVideoDeviceManager::callStaticMethod<String[]>(
            type == Encoders ? "getHWVideoEncoders" : "getHWVideoDecoders");

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
        std::array<CodecsStorage, CodecStorageTypeCount> result;
        void *opaque = nullptr;
        const auto platformHwEncoders = availableHWCodecs(Encoders);
        const auto platformHwDecoders = availableHWCodecs(Decoders);

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
                qCDebug(qLcCodecStorage) << "Skip experimental codec" << codec->name;
                continue;
            }

            if (av_codec_is_decoder(codec)) {
                if (isCodecValid(codec, HWAccel::decodingDeviceTypes(), platformHwDecoders))
                    result[Decoders].emplace_back(codec);
                else
                    qCDebug(qLcCodecStorage)
                            << "Skip decoder" << codec->name
                            << "due to disabled matching hw acceleration, or dysfunctional codec";
            }

            if (av_codec_is_encoder(codec)) {
                if (isCodecValid(codec, HWAccel::encodingDeviceTypes(), platformHwEncoders))
                    result[Encoders].emplace_back(codec);
                else
                    qCDebug(qLcCodecStorage)
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
        const bool shouldDumpCodecsInfo = qLcCodecStorage().isEnabled(QtDebugMsg)
                && qEnvironmentVariableIsSet("QT_FFMPEG_DEBUG");

        if (shouldDumpCodecsInfo) {
            qCDebug(qLcCodecStorage) << "Advanced FFmpeg codecs info:";
            for (auto &storage : result) {
                std::for_each(storage.begin(), storage.end(), &dumpCodecInfo);
                qCDebug(qLcCodecStorage) << "---------------------------";
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

template <typename CodecScoreGetter>
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
    // TODO: remove deviceType and use only isAVFormatSupported to check the format

    return findAVCodec(codecsType, codecId, [&](const AVCodec *codec) {
        if (format && !isAVFormatSupported(codec, *format))
            return NotSuitableAVScore;

        return BestAVScore;
    });
}

} // namespace

const AVCodec *findAVDecoder(AVCodecID codecId, const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(Decoders, codecId, format);
}

const AVCodec *findAVEncoder(AVCodecID codecId, const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(Encoders, codecId, format);
}

const AVCodec *findAVEncoder(AVCodecID codecId,
                             const std::function<AVScore(const AVCodec *)> &scoresGetter)
{
    return findAVCodec(Encoders, codecId, scoresGetter);
}

bool findAndOpenAVDecoder(AVCodecID codecId,
                          const std::function<AVScore(const AVCodec *)> &scoresGetter,
                          const std::function<bool(const AVCodec *)> &codecOpener)
{
    return findAndOpenCodec(Decoders, codecId, scoresGetter, codecOpener);
}

bool findAndOpenAVEncoder(AVCodecID codecId,
                          const std::function<AVScore(const AVCodec *)> &scoresGetter,
                          const std::function<bool(const AVCodec *)> &codecOpener)
{
    return findAndOpenCodec(Encoders, codecId, scoresGetter, codecOpener);
}

} // namespace QFFmpeg

QT_END_NAMESPACE
