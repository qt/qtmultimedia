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

static AVScore calculateTargetFormatScore(const HWAccel *accel, AVPixelFormat sourceFormat,
                                          const AVPixFmtDescriptor *sourceSwFormatDesc,
                                          AVPixelFormat fmt)
{
    if (accel) {
        // accept source format as the best one to ensure zero-copy
        // TODO: maybe, checking of accel->hwFormat() should go first,
        // to be investigated
        if (fmt == sourceFormat)
            return BestAVScore;

        if (accel->hwFormat() == fmt)
            return BestAVScore - 1;

        // The case is suspicious, but probably we should accept it
        if (isHwPixelFormat(fmt))
            return BestAVScore - 2;
    } else {
        if (isHwPixelFormat(fmt))
            return NotSuitableAVScore;

        if (fmt == sourceFormat)
            return BestAVScore;
    }

    return calculateTargetSwFormatScore(sourceSwFormatDesc, fmt);
}

static auto targetFormatScoreCalculator(const HWAccel *accel, AVPixelFormat sourceFormat,
                                        AVPixelFormat sourceSWFormat)
{
    const auto sourceSwFormatDesc = av_pix_fmt_desc_get(sourceSWFormat);
    return [=](AVPixelFormat fmt) {
        return calculateTargetFormatScore(accel, sourceFormat, sourceSwFormatDesc, fmt);
    };
}

AVPixelFormat findTargetSWFormat(AVPixelFormat sourceSWFormat, const HWAccel &accel)
{
    // determine the format used by the encoder.
    // We prefer YUV422 based formats such as NV12 or P010. Selection trues to find the best
    // matching format for the encoder depending on the bit depth of the source format

    const auto sourceFormatDesc = av_pix_fmt_desc_get(sourceSWFormat);
    const auto constraints = accel.constraints();

    if (!constraints || !constraints->valid_sw_formats)
        return sourceSWFormat;

    auto [format, scores] = findBestAVFormat(constraints->valid_sw_formats, [&](AVPixelFormat fmt) {
        return calculateTargetSwFormatScore(sourceFormatDesc, fmt);
    });

    return format;
}

AVPixelFormat findTargetFormat(AVPixelFormat sourceFormat, AVPixelFormat sourceSWFormat,
                               const AVCodec *codec, const HWAccel *accel)
{
    if (!codec->pix_fmts) {
        qWarning() << "Codec pix formats are undefined, it's likely to behave incorrectly";

        // if no accel created, accept only sw format
        return accel || !isHwPixelFormat(sourceFormat) ? sourceFormat : sourceSWFormat;
    }

    auto scoreCalculator = targetFormatScoreCalculator(accel, sourceFormat, sourceSWFormat);
    return findBestAVFormat(codec->pix_fmts, scoreCalculator).first;
}

std::pair<const AVCodec *, std::unique_ptr<HWAccel>> findHwEncoder(AVCodecID codecID,
                                                                   const QSize &sourceSize)
{
    auto matchesSizeConstraints = [&sourceSize](const HWAccel &accel) {
        const auto constraints = accel.constraints();
        if (!constraints)
            return true;

        return sourceSize.width() >= constraints->min_width
                && sourceSize.height() >= constraints->min_height
                && sourceSize.width() <= constraints->max_width
                && sourceSize.height() <= constraints->max_height;
    };

    // 1st - attempt to find hw accelerated encoder
    auto result = HWAccel::findEncoderWithHwAccel(codecID, matchesSizeConstraints);
    Q_ASSERT(!!result.first == !!result.second);

    return result;
}

const AVCodec *findSwEncoder(AVCodecID codecID, AVPixelFormat sourceFormat,
                             AVPixelFormat sourceSWFormat)
{
    auto formatScoreCalculator = targetFormatScoreCalculator(nullptr, sourceFormat, sourceSWFormat);

    return findAVEncoder(codecID, [&formatScoreCalculator](const AVCodec *codec) {
        if (!codec->pix_fmts)
            // codecs without pix_fmts are suspicious
            return MinAVScore;

        return findBestAVFormat(codec->pix_fmts, formatScoreCalculator).second;
    });
}

} // namespace QFFmpeg

QT_END_NAMESPACE
