// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegaudioencoderutils_p.h"
#include "qalgorithms.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

AVSampleFormat adjustSampleFormat(QSpan<const AVSampleFormat> supportedFormats, AVSampleFormat requested)
{
    auto calcScore = [requested](AVSampleFormat format) {
        if (format == requested)
            return BestAVScore;
        if (format == av_get_planar_sample_fmt(requested))
            return BestAVScore - 1;

        const int bps = av_get_bytes_per_sample(format);
        const int bpsRequested = av_get_bytes_per_sample(requested);
        // choose the closest one with higher bps
        if (bps >= bpsRequested)
            return DefaultAVScore - (bps - bpsRequested);

        // choose the closest one with lower bps, considering a priority penalty
        return DefaultAVScore - (bpsRequested - bps) - 1000000;
    };

    const auto result = findBestAVValue(supportedFormats, calcScore);
    return result.value_or(requested);
}

int adjustSampleRate(QSpan<const int> supportedRates, int requested)
{
    auto calcScore = [requested](int rate) {
        if (requested == rate)
            return BestAVScore;

        // choose the closest one with higher rate
        if (rate >= requested)
            return DefaultAVScore - (rate - requested);

        // choose the closest one with lower rate, considering a priority penalty
        return DefaultAVScore - (requested - rate) - 1000000;
    };

    const auto result = findBestAVValue(supportedRates, calcScore);
    return result.value_or(requested);
}

static AVScore calculateScoreByChannelsCount(int supportedChannelsNumber,
                                             int requestedChannelsNumber)
{
    // choose the closest one with higher channels number
    if (supportedChannelsNumber >= requestedChannelsNumber)
        return requestedChannelsNumber - supportedChannelsNumber;

    // choose the closest one with lower channels number, considering a priority penalty
    return supportedChannelsNumber - requestedChannelsNumber - 10000;
}

static AVScore calculateScoreByChannelsMask(int supportedChannelsNumber, uint64_t supportedMask,
                                            int requestedChannelsNumber, uint64_t requestedMask)
{
    if ((supportedMask & requestedMask) == requestedMask)
        return BestAVScore - qPopulationCount(supportedMask & ~requestedMask);

    return calculateScoreByChannelsCount(supportedChannelsNumber, requestedChannelsNumber);
}

ChannelLayoutT adjustChannelLayout(QSpan<const ChannelLayoutT> supportedLayouts,
                                   const ChannelLayoutT &requested)
{
#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
    auto calcScore = [&requested](const ChannelLayoutT &layout) {
        if (layout == requested)
            return BestAVScore;

        // The only realistic case for now:
        // layout.order == requested.order == AV_CHANNEL_ORDER_NATIVE
        // Let's consider other orders to make safe code

        if (layout.order == AV_CHANNEL_ORDER_CUSTOM || requested.order == AV_CHANNEL_ORDER_CUSTOM)
            return calculateScoreByChannelsCount(layout.nb_channels, requested.nb_channels) - 1000;

        const auto offset = layout.order == requested.order ? 1 : 100;

        return calculateScoreByChannelsMask(layout.nb_channels, layout.u.mask,
                                            requested.nb_channels, requested.u.mask)
                - offset;
    };

    const auto result = findBestAVValue(supportedLayouts, calcScore);
    return result.value_or(requested);
#else
    auto calcScore = [requested](ChannelLayoutT mask) {
        return calculateScoreByChannelsMask(qPopulationCount(mask), mask,
                                            qPopulationCount(requested), requested);
    };

    const auto result = findBestAVValue(supportedLayouts, calcScore);
    return result.value_or(requested);
#endif
}

} // namespace QFFmpeg

QT_END_NAMESPACE
