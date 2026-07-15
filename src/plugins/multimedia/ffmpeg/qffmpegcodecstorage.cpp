// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegcodecstorage_p.h"

#include "qffmpeg_p.h"
#include "qffmpeghwaccel_p.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#include <algorithm>
#include <array>
#include <future>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
}

#ifdef Q_OS_ANDROID
#  include <QtCore/qjniobject.h>
#  include <QtCore/qjniarray.h>
#  include <QtCore/qjnitypes.h>

#  include <QtFFmpegMediaPluginImpl/private/qandroidvideojnitypes_p.h>
#endif

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcCodecStorage, "qt.multimedia.ffmpeg.codecstorage");

namespace QFFmpeg {

namespace ranges = QtMultimediaPrivate::ranges;
using namespace Qt::Literals;

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

void dumpCodecInfo(const Codec &codec)
{
    const auto type = codec.isEncoder()
            ? codec.isDecoder() ? "encoder/decoder:" : "encoder:"
            : "decoder:";

    qCDebug(qLcCodecStorage) << codec.type() << type << codec.name() << "id:" << codec.id()
                             << "capabilities:" << AVCodecCapabilities(codec.capabilities());

    if (codec.type() == AVMEDIA_TYPE_VIDEO) {
        const auto pixelFormats = codec.pixelFormats();
        if (!pixelFormats.empty()) {
            qCDebug(qLcCodecStorage) << "  pixelFormats:";
            for (AVPixelFormat f : pixelFormats) {
                auto desc = av_pix_fmt_desc_get(f);
                qCDebug(qLcCodecStorage)
                        << "    id:" << f << desc->name << "depth:" << desc->comp[0].depth
                        << "flags:" << AVPixelFormatFlags(desc->flags);
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
                    << "hw_config_methods:" << AVHwConfigMethods(config->methods);
        }
    }
}

enum class MFCodecCheckResult {
    supported_mf_codec,
    unsupported_mf_codec,
    not_an_mf_codec,
};

MFCodecCheckResult isValidMFEncoder([[maybe_unused]] const Codec &codec)
{
    if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows) {
        if (!codec.name().endsWith("_mf"_L1))
            return MFCodecCheckResult::not_an_mf_codec;

        AVCodecContextUPtr ctx{ avcodec_alloc_context3(codec.get()) };
        if (!ctx)
            return MFCodecCheckResult::unsupported_mf_codec;

        ctx->width = 1280;
        ctx->height = 720;
        ctx->time_base = { 1, 30 };
        ctx->framerate = { 30, 1 };
        ctx->pix_fmt = AV_PIX_FMT_NV12;

        const int ret = avcodec_open2(ctx.get(), codec.get(), nullptr);
        if (ret == AVERROR(ENOSYS)) {
            qCDebug(qLcCodecStorage) << "MF codec" << codec.name() << "is not available.";
            return MFCodecCheckResult::unsupported_mf_codec;
        }

        if (ret < 0) {
            qCDebug(qLcCodecStorage) << "MF codec" << codec.name()
                                     << "is not supported due to avcodec_open2 failure:" << ret
                                     << QFFmpeg::AVError(ret);
            return MFCodecCheckResult::unsupported_mf_codec;
        }

        return MFCodecCheckResult::supported_mf_codec;
    } else {
        return MFCodecCheckResult::not_an_mf_codec;
    }
}

bool isCodecValid(const Codec &codec, QSpan<const AVHWDeviceType> availableHwDeviceTypes,
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

    if (codec.isEncoder() && isValidMFEncoder(codec) == MFCodecCheckResult::unsupported_mf_codec)
        return false; // Unsupported Media Foundation codec

    if (!findAVPixelFormat(codec, &isHwPixelFormat))
        return true; // Codec does not support any hw pixel formats, so no further checks are needed

    if ((codec.capabilities() & AV_CODEC_CAP_HARDWARE) == 0)
        return true; // Codec does not support hardware processing, so no further checks are needed

    if (codecAvailableOnDevice && codecAvailableOnDevice->count(codec.id()) == 0)
        return false; // Codec is not in platform's allow-list

    auto checkDeviceType = [codec](AVHWDeviceType type) {
        return isAVFormatSupported(codec, pixelFormatForHwDevice(type));
    };

    return ranges::any_of(availableHwDeviceTypes, checkDeviceType);
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

struct CodecStoreSingleton
{
    std::shared_future<std::array<CodecsStorage, 2>> codecStoreFuture;

    static bool isExcludedEncoder(QLatin1String codecName)
    {
        static const std::set<std::string, std::less<>> excludeSet = [] {
            std::set<std::string, std::less<>> s;
            const QByteArray excludeEnv = qgetenv("QT_FFMPEG_EXCLUDE_ENCODERS");
            if (excludeEnv.isEmpty())
                return s;
            const QStringList parts = QString::fromUtf8(excludeEnv).split(u',', Qt::SkipEmptyParts);
            for (const QString &p : parts) {
                const QString t = p.trimmed().toLower();
                if (!t.isEmpty())
                    s.insert(t.toStdString());
            }
            return s;
        }();

        std::string_view codecNameView{ codecName.data(), size_t(codecName.size()) };

        if (excludeSet.count(codecNameView)) {
            qCDebug(qLcCodecStorage)
                    << "Skip encoder" << codecName << "due to QT_FFMPEG_EXCLUDE_ENCODERS";
            return true;
        }
        return false;
    }

    static std::array<CodecsStorage, 2> enumerateCodecs()
    {
        std::array<CodecsStorage, 2> result;
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
                    qCDebug(qLcCodecStorage) << "Skip decoder" << codec.name()
                                             << "due to disabled matching hw acceleration, or "
                                                "dysfunctional codec";
            }

            if (codec.isEncoder()) {
                if (isExcludedEncoder(codec.name()))
                    continue;

                if (isCodecValid(codec, HWAccel::encodingDeviceTypes(), platformHwEncoders))
                    result[Encoders].emplace_back(codec);
                else
                    qCDebug(qLcCodecStorage) << "Skip encoder" << codec.name()
                                             << "due to disabled matching hw acceleration, or "
                                                "dysfunctional codec";
            }
        }

        for (auto &storage : result) {
            storage.shrink_to_fit();

            // we should ensure the original order
            ranges::stable_sort(storage, CodecsComparator{});
        }

        // It print pretty much logs, so let's print it only for special case
        const bool shouldDumpCodecsInfo = qLcCodecStorage().isEnabled(QtDebugMsg)
                && qEnvironmentVariableIsSet("QT_FFMPEG_DEBUG");

        if (shouldDumpCodecsInfo) {
            qCDebug(qLcCodecStorage) << "Advanced FFmpeg codecs info:";
            for (auto &storage : result) {
                for (auto &codec : storage)
                    dumpCodecInfo(codec);
                qCDebug(qLcCodecStorage) << "---------------------------";
            }
        }
        return result;
    }

    CodecStoreSingleton()
    {
        // enumerate codecs asynchronously, so that enumeration is done on a separate thread
        // without COM initialization, as otherwise avcodec_open2 will fail and ffmpeg will
        // warn that "COM must not be in STA mode"
        constexpr auto launchPolicy =
                QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows
                ? std::launch::async
                : std::launch::deferred;

        codecStoreFuture = std::async(launchPolicy, [] {
            return enumerateCodecs();
        }).share();
    }
};

Q_APPLICATION_STATIC(CodecStoreSingleton, codecStoreSingleton)

const CodecsStorage &codecsStorage(CodecStorageType codecsType)
{
    return codecStoreSingleton->codecStoreFuture.get()[codecsType];
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

    for (; it != storage.end() && it->id() == codecId; ++it) {
        const AVScore score = scoreGetter ? scoreGetter(*it) : DefaultAVScore;
        if (score != NotSuitableAVScore)
            codecsToScores.emplace_back(*it, score);
    }

    if (scoreGetter) {
        ranges::stable_sort(codecsToScores, [](const CodecToScore &a, const CodecToScore &b) {
            return a.second > b.second;
        });

        if (qLcCodecStorage().isEnabled(QtDebugMsg))
            for (const auto &[codec, score] : codecsToScores)
                qCDebug(qLcCodecStorage)
                        << "findAndOpenCodec(): candidate:" << codec.name() << "score:" << score;
    }

    return ranges::any_of(codecsToScores, [&](const CodecToScore &codecToScore) {
        return opener(codecToScore.first);
    });
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
