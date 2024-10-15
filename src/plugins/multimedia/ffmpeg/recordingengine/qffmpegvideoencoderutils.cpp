// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideoencoderutils_p.h"
#include "qffmpegcodecstorage_p.h"
#include "private/qmultimediautils_p.h"

extern "C" {
#include <libavutil/pixdesc.h>
}

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

static AVScore calculateTargetSwFormatScore(const AVPixFmtDescriptor *sourceSwFormatDesc,
                                            AVPixelFormat fmt)
{
    // determine the format used by the encoder.
    // We prefer YUV422 based formats such as NV12 or P010. Selection trues to find the best
    // matching format for the encoder depending on the bit depth of the source format

    const auto *desc = av_pix_fmt_desc_get(fmt);
    if (!desc)
        return NotSuitableAVScore;

    const int sourceDepth = sourceSwFormatDesc ? sourceSwFormatDesc->comp[0].depth : 0;

    if (desc->flags & AV_PIX_FMT_FLAG_HWACCEL)
        // we really don't want HW accelerated formats here
        return NotSuitableAVScore;

    auto score = DefaultAVScore;

    if (desc == sourceSwFormatDesc)
        // prefer exact matches
        score += 10;
    if (desc->comp[0].depth == sourceDepth)
        score += 100;
    else if (desc->comp[0].depth < sourceDepth)
        score -= 100 + (sourceDepth - desc->comp[0].depth);
    if (desc->log2_chroma_h == 1)
        score += 1;
    if (desc->log2_chroma_w == 1)
        score += 1;
    if (desc->flags & AV_PIX_FMT_FLAG_BE)
        score -= 10;
    if (desc->flags & AV_PIX_FMT_FLAG_PAL)
        // we don't want paletted formats
        score -= 10000;
    if (desc->flags & AV_PIX_FMT_FLAG_RGB)
        // we don't want RGB formats
        score -= 1000;

    // qCDebug(qLcVideoFrameEncoder)
    //        << "checking format" << fmt << Qt::hex << desc->flags << desc->comp[0].depth
    //        << desc->log2_chroma_h << desc->log2_chroma_w << "score:" << score;

    return score;
}

static auto targetSwFormatScoreCalculator(AVPixelFormat sourceFormat)
{
    const auto sourceSwFormatDesc = av_pix_fmt_desc_get(sourceFormat);
    return [=](AVPixelFormat fmt) { return calculateTargetSwFormatScore(sourceSwFormatDesc, fmt); };
}

static bool isHwFormatAcceptedByCodec(AVPixelFormat pixFormat)
{
    switch (pixFormat) {
    case AV_PIX_FMT_MEDIACODEC:
        // Mediacodec doesn't accept AV_PIX_FMT_MEDIACODEC (QTBUG-116836)
        return false;
    default:
        return true;
    }
}

AVPixelFormat findTargetSWFormat(AVPixelFormat sourceSWFormat, const AVCodec *codec,
                                 const HWAccel &accel)
{
    auto scoreCalculator = targetSwFormatScoreCalculator(sourceSWFormat);

    const auto constraints = accel.constraints();
    if (constraints && constraints->valid_sw_formats)
        return findBestAVValue(constraints->valid_sw_formats, scoreCalculator).first;

    // Some codecs, e.g. mediacodec, don't expose constraints, let's find the format in
    // codec->pix_fmts (avcodec_get_supported_config with AV_CODEC_CONFIG_PIX_FORMAT since n7.1)
    const auto pixelFormats = getCodecPixelFormats(codec);
    if (pixelFormats)
        return findBestAVValue(pixelFormats, scoreCalculator).first;

    return AV_PIX_FMT_NONE;
}

AVPixelFormat findTargetFormat(AVPixelFormat sourceFormat, AVPixelFormat sourceSWFormat,
                               const AVCodec *codec, const HWAccel *accel)
{
    Q_UNUSED(sourceFormat);

    if (accel) {
        const auto hwFormat = accel->hwFormat();

        // TODO: handle codec->capabilities & AV_CODEC_CAP_HARDWARE here
        if (!isHwFormatAcceptedByCodec(hwFormat))
            return findTargetSWFormat(sourceSWFormat, codec, *accel);

        const auto constraints = accel->constraints();
        if (constraints && hasAVValue(constraints->valid_hw_formats, hwFormat))
            return hwFormat;

        // Some codecs, don't expose constraints,
        // let's find the format in codec->pix_fmts (avcodec_get_supported_config with
        // AV_CODEC_CONFIG_PIX_FORMAT since n7.1) and hw_config
        if (isAVFormatSupported(codec, hwFormat))
            return hwFormat;
    }

    const auto pixelFormats = getCodecPixelFormats(codec);
    if (!pixelFormats) {
        qWarning() << "Codec pix formats are undefined, it's likely to behave incorrectly";

        return sourceSWFormat;
    }

    auto swScoreCalculator = targetSwFormatScoreCalculator(sourceSWFormat);
    return findBestAVValue(pixelFormats, swScoreCalculator).first;
}

std::pair<const AVCodec *, HWAccelUPtr> findHwEncoder(AVCodecID codecID, const QSize &resolution)
{
    auto matchesSizeConstraints = [&resolution](const HWAccel &accel) {
        return accel.matchesSizeContraints(resolution);
    };

    // 1st - attempt to find hw accelerated encoder
    auto result = HWAccel::findEncoderWithHwAccel(codecID, matchesSizeConstraints);
    Q_ASSERT(!!result.first == !!result.second);

    return result;
}

AVScore findSWFormatScores(const AVCodec* codec, AVPixelFormat sourceSWFormat)
{
    const auto pixelFormats = getCodecPixelFormats(codec);
    if (!pixelFormats)
        // codecs without pixel formats are suspicious
        return MinAVScore;

    auto formatScoreCalculator = targetSwFormatScoreCalculator(sourceSWFormat);
    return findBestAVValue(pixelFormats, formatScoreCalculator).second;
}

const AVCodec *findSwEncoder(AVCodecID codecID, AVPixelFormat sourceSWFormat)
{
    return findAVEncoder(codecID, [sourceSWFormat](const AVCodec *codec) {
        return findSWFormatScores(codec, sourceSWFormat);
    });
}

AVRational adjustFrameRate(const AVRational *supportedRates, qreal requestedRate)
{
    auto calcScore = [requestedRate](const AVRational &rate) {
        // relative comparison
        return qMin(requestedRate * rate.den, qreal(rate.num))
                / qMax(requestedRate * rate.den, qreal(rate.num));
    };

    const auto result = findBestAVValue(supportedRates, calcScore).first;
    if (result.num && result.den)
        return result;

    const auto [num, den] = qRealToFraction(requestedRate);
    return { num, den };
}

AVRational adjustFrameTimeBase(const AVRational *supportedRates, AVRational frameRate)
{
    // TODO: user-specified frame rate might be required.
    if (supportedRates) {
        auto hasFrameRate = [&]() {
            for (auto rate = supportedRates; rate->num && rate->den; ++rate)
                if (rate->den == frameRate.den && rate->num == frameRate.num)
                    return true;

            return false;
        };

        Q_ASSERT(hasFrameRate());

        return { frameRate.den, frameRate.num };
    }

    constexpr int TimeScaleFactor = 1000; // Allows not to follow fixed rate
    return { frameRate.den, frameRate.num * TimeScaleFactor };
}

QSize adjustVideoResolution(const AVCodec *codec, QSize requestedResolution)
{
#ifdef Q_OS_WINDOWS
    // TODO: investigate, there might be more encoders not supporting odd resolution
    if (strcmp(codec->name, "h264_mf") == 0) {
        auto makeEven = [](int size) { return size & ~1; };
        return QSize(makeEven(requestedResolution.width()), makeEven(requestedResolution.height()));
    }
#else
    Q_UNUSED(codec);
#endif
    return requestedResolution;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
