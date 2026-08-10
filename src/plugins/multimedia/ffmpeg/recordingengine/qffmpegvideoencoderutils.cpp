// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideoencoderutils_p.h"

#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>

#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtCore/qglobalstatic.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qreadwritelock.h>
#include <QtCore/private/qminimalflatset_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegrecordingengineutils_p.h>

#include <map>
#include <mutex>
#include <shared_mutex>

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/pixdesc.h>
}

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcVideoEncoderUtils, "qt.multimedia.ffmpeg.videoencoderutils");

namespace QFFmpeg {

using namespace Qt::Literals;

namespace {

bool is16BitFormat(const AVPixFmtDescriptor *desc)
{
    return desc->comp[0].depth == 16;
}

bool is10BitFormat(const AVPixFmtDescriptor *desc)
{
    return desc->comp[0].depth == 10;
}

bool is8BitFormat(const AVPixFmtDescriptor *desc)
{
    return desc->comp[0].depth == 8;
}

bool is444Format(const AVPixFmtDescriptor *desc)
{
    return desc->log2_chroma_h == 0 && desc->log2_chroma_w == 0;
}

bool is422Format(const AVPixFmtDescriptor *desc)
{
    return desc->log2_chroma_h == 1 && desc->log2_chroma_w == 0;
}

bool is420Format(const AVPixFmtDescriptor *desc)
{
    return desc->log2_chroma_h == 1 && desc->log2_chroma_w == 1;
}

bool isGreyFormat(const AVPixFmtDescriptor *desc)
{
    return desc->nb_components == 1;
}

AVScore scoreTargetSwFormat(const AVPixFmtDescriptor *sourceSwFormatDesc, AVPixelFormat fmt)
{
    // determine the format used by the encoder.
    // We prefer YUV420 based formats such as NV12 or P010. Selection trues to find the best
    // matching format for the encoder depending on the bit depth of the source format

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(fmt);
    if (!desc)
        return NotSuitableAVScore;

    if (desc->flags & AV_PIX_FMT_FLAG_HWACCEL)
        // we really don't want HW accelerated formats here
        return NotSuitableAVScore;

    AVScore score = DefaultAVScore;

    if (desc == sourceSwFormatDesc)
        // prefer exact matches
        score += 10;

    const int sourceBpp = av_get_bits_per_pixel(sourceSwFormatDesc);
    const int bpp = av_get_bits_per_pixel(desc);

    // we want formats with the same bpp
    if (bpp == sourceBpp)
        score += 100;
    else if (bpp < sourceBpp)
        score -= 100 + (sourceBpp - bpp);

    // pessimize 10 and 16 bit formats if the source format is 8 bit
    if (is8BitFormat(sourceSwFormatDesc)) {
        if (is10BitFormat(desc))
            score -= 100;
        else if (is16BitFormat(desc))
            score -= 200;
    }

    // Add a slight preference for 4:2:0 formats.
    if (is420Format(desc))
        score += 2;
    else if (is422Format(desc))
        score += 1;
    else if (is444Format(desc))
        score -= 1;

    if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Android) {
        // Add a slight preference for NV12 on Android
        // as it's supported better than other 4:2:0 formats
        if (fmt == AV_PIX_FMT_NV12)
            score += 1;
    }

    if (isGreyFormat(desc) && !isGreyFormat(sourceSwFormatDesc)) // we don't want greyscale formats
        return AVScore::NotSuitableAVScore;

    if (desc->flags & AV_PIX_FMT_FLAG_BE) // we don't want big endian formats
        score -= 10;
    if (desc->flags & AV_PIX_FMT_FLAG_PAL) // we don't want paletted formats
        score -= 10000;
    if (desc->flags & AV_PIX_FMT_FLAG_RGB) // we don't want RGB formats
        score -= 1000;

    return score;
}

auto targetSwFormatScoreCalculator(AVPixelFormat sourceFormat)
{
    const auto sourceSwFormatDesc = av_pix_fmt_desc_get(sourceFormat);
    return [=](AVPixelFormat fmt) {
        return scoreTargetSwFormat(sourceSwFormatDesc, fmt);
    };
}

bool isHwFormatAcceptedByCodec(AVPixelFormat pixFormat)
{
    switch (pixFormat) {
    case AV_PIX_FMT_MEDIACODEC:
        // Mediacodec doesn't accept AV_PIX_FMT_MEDIACODEC (QTBUG-116836)
        return false;
    default:
        return true;
    }
}

// The sw formats commonly accepted by mpeg2, mpeg4, h264, h265, vp8, vp9 and av1 encoders.
// Probing is only a fallback for codecs that tell us nothing, so the list is deliberately short;
// scoreTargetSwFormat() takes care of ranking whatever we find.
static constexpr AVPixelFormat probeCandidates[] = {
    AV_PIX_FMT_YUV420P,     AV_PIX_FMT_NV12,        AV_PIX_FMT_NV21,        AV_PIX_FMT_YUVJ420P,
    AV_PIX_FMT_YUV422P,     AV_PIX_FMT_YUV444P,     AV_PIX_FMT_YUVA420P,    AV_PIX_FMT_P010LE,
    AV_PIX_FMT_YUV420P10LE, AV_PIX_FMT_YUV422P10LE, AV_PIX_FMT_YUV444P10LE, AV_PIX_FMT_GRAY8,
};

class ProbedPixelFormatCache
{
public:
    std::vector<AVPixelFormat> pixelFormats(const Codec &codec, QSize resolution,
                                            const HWAccel *accel);

private:
    std::optional<std::vector<AVPixelFormat>> lookup(const AVCodec *codec, QSize resolution,
                                                     AVPixelFormat hwFormat);

    struct ProbeRecord
    {
        QSize resolution;
        AVPixelFormat hwFormat = AV_PIX_FMT_NONE;
        AVPixelFormat format = AV_PIX_FMT_NONE;
        bool supported = false;
    };

    QReadWriteLock m_lock;
    std::multimap<const AVCodec *, ProbeRecord> m_cache;
};

std::vector<AVPixelFormat>
ProbedPixelFormatCache::pixelFormats(const Codec &codec, QSize resolution, const HWAccel *accel)
{
    const AVPixelFormat hwFormat = accel ? accel->hwFormat() : AV_PIX_FMT_NONE;

    {
        std::shared_lock locker(m_lock);
        if (std::optional cached = lookup(codec.get(), resolution, hwFormat))
            return *cached;
    }

    std::unique_lock locker(m_lock);
    if (std::optional cached = lookup(codec.get(), resolution, hwFormat))
        return *cached;

    const std::vector<AVPixelFormat> supported = probePixelFormats(codec, resolution, accel);
    qCDebug(qLcVideoEncoderUtils) << "Probed pixel formats for" << codec.name() << "at"
                                  << resolution << "hwFormat" << hwFormat << ":" << supported;

    using namespace QtMultimediaPrivate;
    for (AVPixelFormat format : probeCandidates)
        m_cache.emplace(codec.get(),
                        ProbeRecord{
                                resolution,
                                hwFormat,
                                format,
                                ranges::contains(supported, format),
                        });

    return supported;
}

std::optional<std::vector<AVPixelFormat>>
ProbedPixelFormatCache::lookup(const AVCodec *codec, QSize resolution, AVPixelFormat hwFormat)
{
    using namespace QtMultimediaPrivate;
    namespace ranges = QtMultimediaPrivate::ranges;

    auto range_pair = m_cache.equal_range(codec);
    ranges::subrange<std::multimap<const AVCodec *, ProbeRecord>::iterator> range{
        range_pair.first,
        range_pair.second,
    };

    auto matches = [&](const auto &entry) {
        return entry.second.resolution == resolution && entry.second.hwFormat == hwFormat;
    };

    if (!ranges::any_of(range, matches))
        return std::nullopt;

    return views::filter(range, [&](const auto &entry) {
        return matches(entry) && entry.second.supported;
    }) | views::transform([](const auto &entry) {
        return entry.second.format;
    }) | ranges::to<std::vector>();
}

Q_GLOBAL_STATIC(ProbedPixelFormatCache, probedPixelFormatCache)

bool isProbingDisabled()
{
    static const bool disabled =
            qEnvironmentVariableIntValue("QT_FFMPEG_DISABLE_ENCODER_PROBE") != 0;
    return disabled;
}

AVBufferUPtr createProbeFramesContext(const HWAccel &accel, AVPixelFormat swFormat,
                                      QSize resolution)
{
    AVBufferUPtr framesContext{ av_hwframe_ctx_alloc(accel.hwDeviceContextAsBuffer()) };
    if (!framesContext)
        return nullptr;

    auto *context = reinterpret_cast<AVHWFramesContext *>(framesContext->data);
    context->format = accel.hwFormat();
    context->sw_format = swFormat;
    context->width = resolution.width();
    context->height = resolution.height();

    if (av_hwframe_ctx_init(framesContext.get()) < 0)
        return nullptr;

    return framesContext;
}

// Some hw encoders (e.g. nvenc, qsv) advertise the sw formats they can consume directly in
// codec.pixelFormats(), on top of their own hw pixel format. Others (e.g. vaapi) only ever
// declare their own hw pixel format there and rely entirely on the hw accel's frame
// constraints (valid_sw_formats) to describe the sw formats they accept.
bool codecDeclaresSwFormats(const Codec &codec, QSize resolution, const HWAccel *accel)
{
    const std::vector<AVPixelFormat> formats = encoderPixelFormats(codec, resolution, accel);
    return QtMultimediaPrivate::ranges::any_of(formats, [](AVPixelFormat fmt) {
        return !isHwPixelFormat(fmt);
    });
}

inline constexpr auto filterSuitablePixelFormats =
        QtMultimediaPrivate::views::filter([](const ScoredPixelFormat &scoredFmt) {
    return scoredFmt.score > AVScore::NotSuitableAVScore;
});

} // namespace

std::vector<AVPixelFormat> probePixelFormats(const Codec &codec, QSize resolution,
                                             const HWAccel *accel)
{
    std::vector<AVPixelFormat> result;

    for (AVPixelFormat format : probeCandidates) {
        AVCodecContextUPtr context{ avcodec_alloc_context3(codec.get()) };
        if (!context) {
            qCWarning(qLcVideoEncoderUtils)
                    << "Could not allocate codec context to probe" << codec.name();
            break;
        }

        QSpan<const AVRational> supportedFrameRates = codec.frameRates();
        const AVRational frameRate =
                supportedFrameRates.empty() ? AVRational{ 25, 1 } : supportedFrameRates.front();

        context->width = resolution.width();
        context->height = resolution.height();
        context->time_base = { frameRate.den, frameRate.num };
        context->framerate = frameRate;

        AVBufferUPtr framesContext;
        if (accel) {
            framesContext = createProbeFramesContext(*accel, format, resolution);
            if (!framesContext)
                continue;
            context->hw_frames_ctx = av_buffer_ref(framesContext.get());
            context->pix_fmt = accel->hwFormat();
        } else {
            context->pix_fmt = format;
        }

        AVDictionaryHolder opts;
        applyExperimentalCodecOptions(codec, opts);

        if (avcodec_open2(context.get(), codec.get(), opts) >= 0)
            result.push_back(format);
    }

    return result;
}

std::vector<AVPixelFormat> encoderPixelFormats(const Codec &codec, QSize resolution,
                                               const HWAccel *accel)
{
    // An empty result from FFmpeg means 'unknown', not 'nothing is supported', so fall back to
    // asking the encoder itself rather than guessing.
    const QSpan<const AVPixelFormat> declared = codec.pixelFormats();
    if (!declared.empty() || isProbingDisabled())
        return std::vector<AVPixelFormat>(declared.begin(), declared.end());

    return probedPixelFormatCache->pixelFormats(codec, resolution, accel);
}

AVScore scoreTargetSwFormat(AVPixelFormat source, AVPixelFormat target)
{
    const auto *desc = av_pix_fmt_desc_get(source);
    return desc ? scoreTargetSwFormat(desc, target) : AVScore::NotSuitableAVScore;
}

std::optional<AVPixelFormat> findTargetSWFormat(AVPixelFormat sourceSWFormat, const Codec &codec,
                                                const HWAccel &accel, QSize resolution)
{
    using namespace QtMultimediaPrivate;

    auto scoreTargetSwFormat = targetSwFormatScoreCalculator(sourceSWFormat);

    const auto constraints = accel.constraints();
    if (constraints && constraints->valid_sw_formats) {

        const auto validSWFormatsForHWAccel =
                makeSpan(constraints->valid_sw_formats) | ranges::to<QMinimalFlatSet>();

        auto bestPixelFormat = [&]() -> std::optional<AVPixelFormat> {
            if (codecDeclaresSwFormats(codec, resolution, &accel)) {
                // If the codec declares sw formats, we can find the best one among the intersection
                // of the codec's sw formats and the valid sw formats for the hw accel.
                const std::vector<AVPixelFormat> codecPixelFormats =
                        encoderPixelFormats(codec, resolution, &accel);
                auto formats = views::filter(codecPixelFormats, [&](AVPixelFormat fmt) {
                    return validSWFormatsForHWAccel.contains(fmt);
                });

                return findBestAVValue(formats, scoreTargetSwFormat);
            } else {
                return findBestAVValue(validSWFormatsForHWAccel, scoreTargetSwFormat);
            }
        }();

        if (bestPixelFormat)
            return bestPixelFormat;
    }

    // Some codecs, e.g. mediacodec, don't expose constraints, let's find the format in
    // codec->pix_fmts (avcodec_get_supported_config with AV_CODEC_CONFIG_PIX_FORMAT since n7.1),
    // or, if the codec declares nothing at all, in the probed formats.
    const std::vector<AVPixelFormat> codecPixelFormats =
            encoderPixelFormats(codec, resolution, &accel);
    return findBestAVValue(codecPixelFormats, scoreTargetSwFormat);
}

std::vector<ScoredPixelFormat> findAndScoreTargetSWFormats(AVPixelFormat sourceSWFormat,
                                                           const Codec &codec, const HWAccel &accel,
                                                           QSize resolution)
{
    if constexpr (false) {
        qDebug() << "findAndScoreTargetSWFormats" << codec.name() << codec.id();
    }

    using namespace QtMultimediaPrivate;

    auto scoreTargetSwFormat = targetSwFormatScoreCalculator(sourceSWFormat);

    auto score = views::transform([&](AVPixelFormat arg) {
        return ScoredPixelFormat{ arg, scoreTargetSwFormat(arg) };
    });

    std::vector<ScoredPixelFormat> scoredPixelFormats = [&] {
        const auto constraints = accel.constraints();
        if (constraints && constraints->valid_sw_formats) {
            const auto validSWFormatsForHWAccel =
                    makeSpan(constraints->valid_sw_formats) | ranges::to<QMinimalFlatSet>();

            if (codecDeclaresSwFormats(codec, resolution, &accel)) {
                // If the codec declares sw formats, we can find the best one among the intersection
                // of the codec's sw formats and the valid sw formats for the hw accel.
                const std::vector<AVPixelFormat> codecPixelFormats =
                        encoderPixelFormats(codec, resolution, &accel);
                auto validCodecPixelFormats =
                        views::filter(codecPixelFormats, [&](AVPixelFormat fmt) {
                    return validSWFormatsForHWAccel.contains(fmt);
                });
                return validCodecPixelFormats | score | filterSuitablePixelFormats
                        | ranges::to<std::vector>();
            } else {
                // The codec only declares its own hw pixel format (e.g. vaapi); the sw formats it
                // actually accepts come solely from the hw accel's frame constraints.
                return validSWFormatsForHWAccel | score | filterSuitablePixelFormats
                        | ranges::to<std::vector>();
            }
        } else {
            // Some codecs, e.g. mediacodec, don't expose constraints, let's find the format in
            // codec->pix_fmts (avcodec_get_supported_config with AV_CODEC_CONFIG_PIX_FORMAT since
            // n7.1), or, if the codec declares nothing at all, in the probed formats.

            const std::vector<AVPixelFormat> codecPixelFormats =
                    encoderPixelFormats(codec, resolution, &accel);
            return codecPixelFormats | score | filterSuitablePixelFormats
                    | ranges::to<std::vector>();
        }
    }();

    ranges::sort(scoredPixelFormats, [](const ScoredPixelFormat &a, const ScoredPixelFormat &b) {
        return a.score > b.score;
    });

    return scoredPixelFormats;
}

std::optional<AVPixelFormat> findTargetFormat(AVPixelFormat sourceSWFormat, const Codec &codec,
                                              const HWAccel *accel, QSize resolution)
{
    using namespace QtMultimediaPrivate;

    if (accel) {
        const auto hwFormat = accel->hwFormat();

        // TODO: handle codec->capabilities & AV_CODEC_CAP_HARDWARE here
        if (!isHwFormatAcceptedByCodec(hwFormat))
            return findTargetSWFormat(sourceSWFormat, codec, *accel, resolution);

        const auto constraints = accel->constraints();
        if (constraints && ranges::contains(makeSpan(constraints->valid_hw_formats), hwFormat))
            return hwFormat;

        // Some codecs, don't expose constraints,
        // let's find the format in codec->pix_fmts (avcodec_get_supported_config with
        // AV_CODEC_CONFIG_PIX_FORMAT since n7.1) and hw_config
        if (isAVFormatSupported(codec, hwFormat))
            return hwFormat;
    }

    const auto pixelFormats = encoderPixelFormats(codec, resolution, nullptr);
    if (pixelFormats.empty()) {
        qCWarning(qLcVideoEncoderUtils)
                << "Codec pix formats are undefined and probing" << codec.name()
                << "found none either, it's likely to behave incorrectly";

        return sourceSWFormat;
    }

    auto swScoreCalculator = targetSwFormatScoreCalculator(sourceSWFormat);
    return findBestAVValue(pixelFormats, swScoreCalculator);
}

AVScore findSWFormatScores(const Codec &codec, AVPixelFormat sourceSWFormat, QSize resolution)
{
    const auto pixelFormats = encoderPixelFormats(codec, resolution, nullptr);
    if (pixelFormats.empty())
        // codecs that neither declare pixel formats nor accept any of the probed ones
        // are suspicious
        return MinAVScore;

    auto formatScoreCalculator = targetSwFormatScoreCalculator(sourceSWFormat);
    std::optional bestFormatWithScore =
            findBestAVValueWithScore(pixelFormats, formatScoreCalculator);
    if (bestFormatWithScore)
        return bestFormatWithScore->score;
    else
        return MinAVScore;
}

AVRational adjustFrameRate(QSpan<const AVRational> supportedRates, qreal settingsRate,
                           qreal sourceRate)
{
    qreal preferredRate = 0.;
    if (settingsRate > 0)
        preferredRate = settingsRate;
    else if (sourceRate > 0)
        preferredRate = sourceRate;
    else if (supportedRates.empty())
        preferredRate = 0.;
    else
        preferredRate = qreal(DefaultVideoFrameRate);

    auto calcScore = [preferredRate](const AVRational &rate) {
        // relative comparison
        return qMin(preferredRate * rate.den, qreal(rate.num))
                / qMax(preferredRate * rate.den, qreal(rate.num));
    };

    const auto result = findBestAVValue(supportedRates, calcScore);
    if (result && result->num && result->den)
        return *result;

    const auto [num, den] = qRealToFraction(preferredRate);
    return { num, den };
}

AVRational adjustFrameTimeBase(QSpan<const AVRational> supportedRates, AVRational frameRate,
                               bool isFixedRate)
{
    // TODO: user-specified frame rate might be required.
    if (!supportedRates.empty()) {
        auto hasFrameRate = [&]() {
            for (AVRational rate : supportedRates)
                if (rate.den == frameRate.den && rate.num == frameRate.num)
                    return true;

            return false;
        };

        Q_ASSERT(hasFrameRate());

        return { frameRate.den, frameRate.num };
    }

    if (isFixedRate)
        return { frameRate.den, frameRate.num };

    constexpr int TimeScaleFactor = 1000; // Allows not to follow fixed rate
    return { frameRate.den, frameRate.num * TimeScaleFactor };
}

QSize adjustVideoResolution(const Codec &codec, QSize requestedResolution)
{
    if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows) {
        // TODO: investigate, there might be more encoders not supporting odd resolution
        if (codec.name() == "h264_mf"_L1) {
            auto makeEven = [](int size) { return size & ~1; };
            return QSize(makeEven(requestedResolution.width()), makeEven(requestedResolution.height()));
        }
    }
    return requestedResolution;
}

SwsFlags getScaleConversionType(const QSize &sourceSize, const QSize &targetSize)
{
    SwsFlags conversionType = SWS_FAST_BILINEAR;

    if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Android) {
        // On Android, use SWS_BICUBIC for upscaling if least one dimension is upscaled
        // to avoid a crash caused by ff_hcscale_fast_c with SWS_FAST_BILINEAR.
        if (targetSize.width() > sourceSize.width() || targetSize.height() > sourceSize.height())
            conversionType = SWS_BICUBIC;
    }

    return conversionType;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
