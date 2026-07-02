// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideoencoderutils_p.h"

#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/private/qminimalflatset_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegrecordingengineutils_p.h>

extern "C" {
#include <libavutil/pixdesc.h>
}

QT_BEGIN_NAMESPACE

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

} // namespace

std::optional<AVPixelFormat> findTargetSWFormat(AVPixelFormat sourceSWFormat, const Codec &codec,
                                                const HWAccel &accel,
                                                const AVPixelFormatSet &prohibitedFormats)
{
    using namespace QtMultimediaPrivate;

    auto scoreTargetSwFormat = targetSwFormatScoreCalculator(sourceSWFormat);

    const auto constraints = accel.constraints();
    if (constraints && constraints->valid_sw_formats) {

        const auto validSWFormatsForHWAccel =
                makeSpan(constraints->valid_sw_formats) | ranges::to<QMinimalFlatSet>();

        const auto codecPixelFormats = codec.pixelFormats();
        auto validCodecPixelFormats = views::filter(codecPixelFormats, [&](AVPixelFormat fmt) {
            if (!validSWFormatsForHWAccel.contains(fmt))
                return false;

            return !prohibitedFormats.count(fmt);
        });

        if constexpr (false) {
            qDebug() << "validSWFormats" << (validSWFormatsForHWAccel | ranges::to<std::vector>())
                     << "scoredPixelFormats"
                     << (validCodecPixelFormats | views::transform([&](auto arg) {
                return std::pair(arg, scoreTargetSwFormat(arg));
            }) | ranges::to<std::vector>());
        }

        std::optional bestPixelFormat =
                findBestAVValue(validCodecPixelFormats, scoreTargetSwFormat);
        if (bestPixelFormat)
            return bestPixelFormat;
    }

    // Some codecs, e.g. mediacodec, don't expose constraints, let's find the format in
    // codec->pix_fmts (avcodec_get_supported_config with AV_CODEC_CONFIG_PIX_FORMAT since n7.1)
    const auto codecPixelFormats = codec.pixelFormats();
    auto pixelFormats = views::filter(codecPixelFormats, [&](AVPixelFormat fmt) {
        return !prohibitedFormats.count(fmt);
    });

    return findBestAVValue(pixelFormats, scoreTargetSwFormat);
}

std::optional<AVPixelFormat> findTargetFormat(AVPixelFormat sourceSWFormat, const Codec &codec,
                                              const HWAccel *accel,
                                              const AVPixelFormatSet &prohibitedFormats)
{
    using namespace QtMultimediaPrivate;

    if (accel) {
        const auto hwFormat = accel->hwFormat();

        // TODO: handle codec->capabilities & AV_CODEC_CAP_HARDWARE here
        if (!isHwFormatAcceptedByCodec(hwFormat) || prohibitedFormats.count(hwFormat))
            return findTargetSWFormat(sourceSWFormat, codec, *accel, prohibitedFormats);

        const auto constraints = accel->constraints();
        if (constraints && ranges::contains(makeSpan(constraints->valid_hw_formats), hwFormat))
            return hwFormat;

        // Some codecs, don't expose constraints,
        // let's find the format in codec->pix_fmts (avcodec_get_supported_config with
        // AV_CODEC_CONFIG_PIX_FORMAT since n7.1) and hw_config
        if (isAVFormatSupported(codec, hwFormat))
            return hwFormat;
    }

    const auto pixelFormats = codec.pixelFormats();
    if (pixelFormats.empty()) {
        qWarning() << "Codec pix formats are undefined, it's likely to behave incorrectly";

        return sourceSWFormat;
    }

    auto candidatePixelFormats = views::filter(pixelFormats, [&](AVPixelFormat fmt) {
        return !prohibitedFormats.count(fmt);
    });

    auto swScoreCalculator = targetSwFormatScoreCalculator(sourceSWFormat);
    return findBestAVValue(candidatePixelFormats, swScoreCalculator);
}

AVScore findSWFormatScores(const Codec &codec, AVPixelFormat sourceSWFormat)
{
    const auto pixelFormats = codec.pixelFormats();
    if (pixelFormats.empty())
        // codecs without pixel formats are suspicious
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
