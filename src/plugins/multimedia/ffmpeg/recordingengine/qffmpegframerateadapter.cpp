// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegframerateadapter_p.h"

#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

void FrameRateAdapter::setRates(std::optional<double> sourceRate, double targetRate)
{
    if (targetRate <= 0 || (sourceRate && qFuzzyCompare(*sourceRate, targetRate))) {
        m_slotDuration = 0;
        m_slotMatchEpsilon = 0;
        return;
    }

    m_slotDuration = static_cast<qint64>(VideoFrameTimeBase / targetRate);

    if (sourceRate) {
        // Fixed-rate source, expect steady period
        m_sourceDuration = static_cast<qint64>(qRound(VideoFrameTimeBase / *sourceRate));

        // Jitter tolerance: 1% of source period. Frames arriving more than this
        // past a slot boundary belong to the next slot and should not be treated
        // as landing on the current slot boundary.
        constexpr double JitterFraction = 0.01;
        m_slotMatchEpsilon = qRound(static_cast<double>(*m_sourceDuration) * JitterFraction);
    } else {
        // Variable-rate source
        m_sourceDuration = std::nullopt;

        // No source rate info: use a small absolute tolerance (1ms)
        m_slotMatchEpsilon = 1000;
    }
}

std::vector<FrameInfo> FrameRateAdapter::adapt(const QVideoFrame &frame, bool adjustTimeBase)
{
    std::vector<FrameInfo> result;

    // Can't adapt without target frame rate
    if (!isActive()) {
        result.push_back({ frame, adjustTimeBase, std::nullopt });
        return result;
    }

    const qint64 frameStart = frame.startTime();

    // Can't adapt without a valid start time
    if (frameStart == -1) {
        result.push_back({ frame, adjustTimeBase, std::nullopt });
        return result;
    }

    if (m_nextSlotTime < 0)
        m_nextSlotTime = frameStart;

    qint64 frameEnd = frame.endTime();
    if (frameEnd == -1 && m_sourceDuration)
        frameEnd = frameStart + *m_sourceDuration;

    // Repeat previous frame to fill skipped slots
    while (frameStart > m_nextSlotTime + m_slotMatchEpsilon)
        result.push_back(emitPendingFrame());

    m_pendingFrame = { frame, adjustTimeBase, m_nextSlotTime };
    // If a source is variable-rate and the frame has no endTime, prepare frame to be flushed on EOS
    m_pendingNeedsFlush = !m_sourceDuration && frameEnd == -1;

    // Emit for current slot if frame arrival aligns with slot start
    Q_ASSERT(frameStart <= m_nextSlotTime + m_slotMatchEpsilon);
    if (frameStart >= m_nextSlotTime)
        result.push_back(emitPendingFrame());

    // Copy frame over all encoding slots it's presented for
    if (frameEnd > frameStart) {
        while (frameEnd > (m_nextSlotTime + m_slotMatchEpsilon))
            result.push_back(emitPendingFrame());
    }

    return result;
}

std::optional<FrameInfo> FrameRateAdapter::flush()
{
    if (m_pendingNeedsFlush)
        return emitPendingFrame(false);

    m_pendingFrame = std::nullopt;
    return std::nullopt;
}

void FrameRateAdapter::reset()
{
    m_nextSlotTime = -1;
    m_pendingFrame = std::nullopt;
    m_pendingNeedsFlush = false;
}

FrameInfo FrameRateAdapter::emitPendingFrame(bool keepPending)
{
    Q_ASSERT(m_pendingFrame);
    m_pendingNeedsFlush = false;
    m_nextSlotTime += m_slotDuration;
    if (keepPending) {
        auto pending = *m_pendingFrame;
        m_pendingFrame->adjustTimeBase = false;
        m_pendingFrame->overriddenPts = m_nextSlotTime;
        return pending;
    }

    auto pending = *std::move(m_pendingFrame);
    m_pendingFrame = std::nullopt;
    return pending;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
