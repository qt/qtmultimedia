// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideoencoderutils_p.h"
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
        return findBestAVFormat(constraints->valid_sw_formats, scoreCalculator).first;

    // Some codecs, e.g. mediacodec, don't expose constraints, let's find the format in
    // codec->pix_fmts
    if (codec->pix_fmts)
        return findBestAVFormat(codec->pix_fmts, scoreCalculator).first;

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
        if (constraints && hasAVFormat(constraints->valid_hw_formats, hwFormat))
            return hwFormat;

        // Some codecs, don't expose constraints, let's find the format in codec->pix_fmts
        if (hasAVFormat(codec->pix_fmts, hwFormat))
            return hwFormat;
    }

    if (!codec->pix_fmts) {
        qWarning() << "Codec pix formats are undefined, it's likely to behave incorrectly";

        return sourceSWFormat;
    }

    auto swScoreCalculator = targetSwFormatScoreCalculator(sourceSWFormat);
    return findBestAVFormat(codec->pix_fmts, swScoreCalculator).first;
}

std::pair<const AVCodec *, std::unique_ptr<HWAccel>> findHwEncoder(AVCodecID codecID,
                                                                   const QSize &resolution)
{
    auto matchesSizeConstraints = [&resolution](const HWAccel &accel) {
        const auto constraints = accel.constraints();
        if (!constraints)
            return true;

        return resolution.width() >= constraints->min_width
                && resolution.height() >= constraints->min_height
                && resolution.width() <= constraints->max_width
                && resolution.height() <= constraints->max_height;
    };

    // 1st - attempt to find hw accelerated encoder
    auto result = HWAccel::findEncoderWithHwAccel(codecID, matchesSizeConstraints);
    Q_ASSERT(!!result.first == !!result.second);

    return result;
}

const AVCodec *findSwEncoder(AVCodecID codecID, AVPixelFormat sourceSWFormat)
{
    auto formatScoreCalculator = targetSwFormatScoreCalculator(sourceSWFormat);

    return findAVEncoder(codecID, [&formatScoreCalculator](const AVCodec *codec) {
        if (!codec->pix_fmts)
            // codecs without pix_fmts are suspicious
            return MinAVScore;

        return findBestAVFormat(codec->pix_fmts, formatScoreCalculator).second;
    });
}

AVRational adjustFrameRate(const AVRational *supportedRates, qreal requestedRate)
{
    qreal diff = std::numeric_limits<qreal>::max();

    auto getDiff = [requestedRate](qreal currentRate) {
        return qMax(requestedRate, currentRate) / qMin(requestedRate, currentRate);

        // Using just a liniar delta is also possible, but
        // relative comparison should work better
        // return qAbs(currentRate - requestedRate);
    };

    if (supportedRates) {
        const AVRational *result = nullptr;
        for (auto rate = supportedRates; rate->num && rate->den; ++rate) {
            const qreal currentDiff = getDiff(qreal(rate->num) / rate->den);

            if (currentDiff < diff) {
                diff = currentDiff;
                result = supportedRates;
            }
        }

        if (result)
            return *result;
    }

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
