// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegaudioencoderutils_p.h"
#include "qalgorithms.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

AVSampleFormat adjustSampleFormat(const AVSampleFormat *supportedFormats, AVSampleFormat requested)
{
    auto calcScore = [requested](AVSampleFormat format) {
        return format == requested                              ? BestAVScore
                : format == av_get_planar_sample_fmt(requested) ? BestAVScore - 1
                                                                : 0;
    };

    const auto result = findBestAVValue(supportedFormats, calcScore).first;
    return result == AV_SAMPLE_FMT_NONE ? requested : result;
}

int adjustSampleRate(const int *supportedRates, int requested)
{
    auto calcScore = [requested](int rate) {
        return requested == rate    ? BestAVScore
                : requested <= rate ? rate - requested
                                    : requested - rate - 1000000;
    };

    const auto result = findBestAVValue(supportedRates, calcScore).first;
    return result == 0 ? requested : result;
}

static AVScore calculateScoreByChannelsCount(int supportedChannelsNumber,
                                             int requestedChannelsNumber)
{
    if (supportedChannelsNumber >= requestedChannelsNumber)
        return requestedChannelsNumber - supportedChannelsNumber;

    return supportedChannelsNumber - requestedChannelsNumber - 10000;
}

static AVScore calculateScoreByChannelsMask(int supportedChannelsNumber, uint64_t supportedMask,
                                            int requestedChannelsNumber, uint64_t requestedMask)
{
    if ((supportedMask & requestedMask) == requestedMask)
        return BestAVScore - qPopulationCount(supportedMask & ~requestedMask);

    return calculateScoreByChannelsCount(supportedChannelsNumber, requestedChannelsNumber);
}

#if QT_FFMPEG_OLD_CHANNEL_LAYOUT

uint64_t adjustChannelLayout(const uint64_t *supportedMasks, uint64_t requested)
{
    auto calcScore = [requested](uint64_t mask) {
        return calculateScoreByChannelsMask(qPopulationCount(mask), mask,
                                            qPopulationCount(requested), requested);
    };

    const auto result = findBestAVValue(supportedMasks, calcScore).first;
    return result == 0 ? requested : result;
}

#else

AVChannelLayout adjustChannelLayout(const AVChannelLayout *supportedLayouts,
                                    const AVChannelLayout &requested)
{
    auto calcScore = [&requested](const AVChannelLayout &layout) {
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
    return result.second == NotSuitableAVScore ? requested : result.first;
}

#endif

} // namespace QFFmpeg

QT_END_NAMESPACE
