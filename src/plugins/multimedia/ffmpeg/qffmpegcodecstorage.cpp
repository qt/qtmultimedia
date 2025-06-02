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
                    "org/qtproject/qt/android/multimedia/QtVideoDeviceManager")
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

using CodecsStorage = std::vector<Codec>;

struct CodecsComparator
{
    bool operator()(const Codec &a, const Codec &b) const
    {
        return a.id() < b.id() || (a.id() == b.id() && a.isExperimental() < b.isExperimental());
    }

    bool operator()(const Codec &codec, AVCodecID id) const { return codec.id() < id; }
    bool operator()(AVCodecID id, const Codec &codec) const { return id < codec.id(); }
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
                result += u", ";
            result += QLatin1StringView(flagAndName.second);
        }

    if (leftover) {
        if (!result.isEmpty())
            result += u", ";
        result += QString::number(leftover, 16);
    }
    return result;
}

void dumpCodecInfo(const Codec &codec)
{
    using FlagNames = std::initializer_list<std::pair<int, const char *>>;
    const auto mediaType = codec.type() == AVMEDIA_TYPE_VIDEO ? "video"
            : codec.type() == AVMEDIA_TYPE_AUDIO              ? "audio"
            : codec.type() == AVMEDIA_TYPE_SUBTITLE           ? "subtitle"
                                                             : "other_type";

    const auto type = codec.isEncoder()
            ? codec.isDecoder() ? "encoder/decoder:" : "encoder:"
            : "decoder:";

    static const FlagNames capabilitiesNames = {
        { AV_CODEC_CAP_DRAW_HORIZ_BAND, "DRAW_HORIZ_BAND" },
        { AV_CODEC_CAP_DR1, "DRAW_HORIZ_DR1" },
        { AV_CODEC_CAP_DELAY, "DELAY" },
        { AV_CODEC_CAP_SMALL_LAST_FRAME, "SMALL_LAST_FRAME" },
#ifdef AV_CODEC_CAP_SUBFRAMES
        { AV_CODEC_CAP_SUBFRAMES, "SUBFRAMES" },
#endif
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

    qCDebug(qLcCodecStorage) << mediaType << type << codec.name() << "id:" << codec.id()
                             << "capabilities:"
                             << flagsToString(codec.capabilities(), capabilitiesNames);

    if (codec.type() == AVMEDIA_TYPE_VIDEO) {
        const auto pixelFormats = codec.pixelFormats();
        if (!pixelFormats.empty()) {
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

            qCDebug(qLcCodecStorage) << "  pixelFormats:";
            for (AVPixelFormat f : pixelFormats) {
                auto desc = av_pix_fmt_desc_get(f);
                qCDebug(qLcCodecStorage)
                        << "    id:" << f << desc->name << "depth:" << desc->comp[0].depth
                        << "flags:" << flagsToString(desc->flags, flagNames);
            }
        } else {
            qCDebug(qLcCodecStorage) << "  pixelFormats: null";
        }
    } else if (codec.type() == AVMEDIA_TYPE_AUDIO) {
        const auto sampleFormats = codec.sampleFormats();
        if (!sampleFormats.empty()) {
            qCDebug(qLcCodecStorage) << "  sampleFormats:";
            for (auto f : sampleFormats) {
                const auto name = av_get_sample_fmt_name(f);
                qCDebug(qLcCodecStorage) << "    id:" << f << (name ? name : "unknown")
                                         << "bytes_per_sample:" << av_get_bytes_per_sample(f)
                                         << "is_planar:" << av_sample_fmt_is_planar(f);
            }
        } else {
            qCDebug(qLcCodecStorage) << "  sampleFormats: null";
        }
    }

    const std::vector<const AVCodecHWConfig*> hwConfigs = codec.hwConfigs();
    if (!hwConfigs.empty()) {
        static const FlagNames hwConfigMethodNames = {
            { AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX, "HW_DEVICE_CTX" },
            { AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX, "HW_FRAMES_CTX" },
            { AV_CODEC_HW_CONFIG_METHOD_INTERNAL, "INTERNAL" },
            { AV_CODEC_HW_CONFIG_METHOD_AD_HOC, "AD_HOC" }
        };

        qCDebug(qLcCodecStorage) << "  hw config:";
        for (const AVCodecHWConfig* config : hwConfigs) {
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

bool isCodecValid(const Codec &codec, const std::vector<AVHWDeviceType> &availableHwDeviceTypes,
                  const std::optional<std::unordered_set<AVCodecID>> &codecAvailableOnDevice)
{
    if (codec.type() != AVMEDIA_TYPE_VIDEO)
        return true;

    const auto pixelFormats = codec.pixelFormats();
    if (pixelFormats.empty()) {
#if defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
        //  Disable V4L2 M2M codecs for encoding for now,
        //  TODO: Investigate on how to get them working
        if (codec.name().contains(QLatin1StringView{ "_v4l2m2m" }) && codec.isEncoder())
            return false;

        // MediaCodec in Android is used for hardware-accelerated media processing. That is why
        // before marking it as valid, we need to make sure if it is available on current device.
        if (codec.name().contains(QLatin1StringView{ "_mediacodec" })
            && (codec.capabilities() & AV_CODEC_CAP_HARDWARE)
            && codecAvailableOnDevice && codecAvailableOnDevice->count(codec.id()) == 0)
            return false;
#endif

        return true; // When the codec reports no pixel formats, format support is unknown.
    }

    if (!findAVPixelFormat(codec, &isHwPixelFormat))
        return true; // Codec does not support any hw pixel formats, so no further checks are needed

    if ((codec.capabilities() & AV_CODEC_CAP_HARDWARE) == 0)
        return true; // Codec does not support hardware processing, so no further checks are needed

    if (codecAvailableOnDevice && codecAvailableOnDevice->count(codec.id()) == 0)
        return false; // Codec is not in platform's allow-list

    auto checkDeviceType = [codec](AVHWDeviceType type) {
        return isAVFormatSupported(codec, pixelFormatForHwDevice(type));
    };

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
        const auto platformHwEncoders = availableHWCodecs(Encoders);
        const auto platformHwDecoders = availableHWCodecs(Decoders);

        for (const Codec codec : CodecEnumerator()) {
            // TODO: to be investigated
            // FFmpeg functions avcodec_find_decoder/avcodec_find_encoder
            // find experimental codecs in the last order,
            // now we don't consider them at all since they are supposed to
            // be not stable, maybe we shouldn't.
            // Currently, it's possible to turn them on for testing purposes.

            static const auto experimentalCodecsEnabled =
                    qEnvironmentVariableIntValue("QT_ENABLE_EXPERIMENTAL_CODECS");

            if (!experimentalCodecsEnabled && codec.isExperimental()) {
                qCDebug(qLcCodecStorage) << "Skip experimental codec" << codec.name();
                continue;
            }

            if (codec.isDecoder()) {
                if (isCodecValid(codec, HWAccel::decodingDeviceTypes(), platformHwDecoders))
                    result[Decoders].emplace_back(codec);
                else
                    qCDebug(qLcCodecStorage)
                            << "Skip decoder" << codec.name()
                            << "due to disabled matching hw acceleration, or dysfunctional codec";
            }

            if (codec.isEncoder()) {
                if (isCodecValid(codec, HWAccel::encodingDeviceTypes(), platformHwEncoders))
                    result[Encoders].emplace_back(codec);
                else
                    qCDebug(qLcCodecStorage)
                            << "Skip encoder" << codec.name()
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

    using CodecToScore = std::pair<Codec, AVScore>;
    std::vector<CodecToScore> codecsToScores;

    for (; it != storage.end() && it->id()  == codecId; ++it) {
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

std::optional<Codec> findAVCodec(CodecStorageType codecsType, AVCodecID codecId,
                                 const std::optional<PixelOrSampleFormat> &format)
{
    const CodecsStorage& storage = codecsStorage(codecsType);

    // Storage is sorted, so we can quickly narrow down the search to codecs with the specific id.
    auto begin = std::lower_bound(storage.begin(), storage.end(), codecId, CodecsComparator{});
    auto end = std::upper_bound(begin, storage.end(), codecId, CodecsComparator{});

    // Within the narrowed down range, look for a codec that supports the format.
    // If no format is specified, return the first one.
    auto codecIt = std::find_if(begin, end, [&format](const Codec &codec) {
        return !format || isAVFormatSupported(codec, *format);
    });

    if (codecIt != end)
        return *codecIt;

    return {};
}

} // namespace

std::optional<Codec> findAVDecoder(AVCodecID codecId,
                                   const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(Decoders, codecId, format);
}

std::optional<Codec> findAVEncoder(AVCodecID codecId, const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(Encoders, codecId, format);
}

bool findAndOpenAVDecoder(AVCodecID codecId,
                          const std::function<AVScore(const Codec &)> &scoresGetter,
                          const std::function<bool(const Codec &)> &codecOpener)
{
    return findAndOpenCodec(Decoders, codecId, scoresGetter, codecOpener);
}

bool findAndOpenAVEncoder(AVCodecID codecId,
                          const std::function<AVScore(const Codec &)> &scoresGetter,
                          const std::function<bool(const Codec &)> &codecOpener)
{
    return findAndOpenCodec(Encoders, codecId, scoresGetter, codecOpener);
}

} // namespace QFFmpeg

QT_END_NAMESPACE
