// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideoencoderutils_p.h"

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
    if (accel) {
        const auto hwFormat = accel->hwFormat();

        const auto constraints = accel->constraints();
        if (constraints && hasAVFormat(constraints->valid_hw_formats, hwFormat))
            return hwFormat;

        // Some codecs, e.g. mediacodec, don't expose constraints, let's find the format in
        // codec->pix_fmts
        if (hasAVFormat(codec->pix_fmts, hwFormat))
            return hwFormat;

        return AV_PIX_FMT_NONE;
    }

    if (!codec->pix_fmts) {
        qWarning() << "Codec pix formats are undefined, it's likely to behave incorrectly";

        return sourceSWFormat;
    }

    auto scoreCalculator = targetSwFormatScoreCalculator(sourceFormat);
    return findBestAVFormat(codec->pix_fmts, scoreCalculator).first;
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

} // namespace QFFmpeg

QT_END_NAMESPACE
