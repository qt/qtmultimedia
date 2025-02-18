// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegtimecontroller_p.h"

#include "qglobal.h"
#include "qdebug.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

TimeController::TimeController()
{
    sync();
}

TimeController::PlaybackRate TimeController::playbackRate() const
{
    return m_playbackRate;
}

void TimeController::setPlaybackRate(PlaybackRate playbackRate)
{
    if (playbackRate == m_playbackRate)
        return;

    Q_ASSERT(playbackRate > 0.f);

    scrollTimeTillNow();
    m_playbackRate = playbackRate;

    if (m_softSyncData)
        m_softSyncData = makeSoftSyncData(m_timePoint, m_position, m_softSyncData->dstTimePoint);
}

void TimeController::sync(TrackPosition trackPos)
{
    sync(SteadyClock::now(), trackPos);
}

void TimeController::sync(TimePoint tp, TrackPosition pos)
{
    m_softSyncData.reset();
    m_position = pos;
    m_timePoint = tp;
}

void TimeController::syncSoft(TimePoint tp, TrackPosition pos, SteadyClock::duration fixingTime)
{
    const auto srcTime = SteadyClock::now();
    const auto srcPos = positionFromTime(srcTime, true);
    const auto dstTime = srcTime + fixingTime;

    m_position = pos;
    m_timePoint = tp;

    m_softSyncData = makeSoftSyncData(srcTime, srcPos, dstTime);
}

TrackPosition TimeController::currentPosition(SteadyClock::duration offset) const
{
    return positionFromTime(SteadyClock::now() + offset);
}

void TimeController::setPaused(bool paused)
{
    if (m_paused == paused)
        return;

    scrollTimeTillNow();
    m_paused = paused;
}

TrackPosition TimeController::positionFromTime(TimePoint tp, bool ignorePause) const
{
    tp = m_paused && !ignorePause ? m_timePoint : tp;

    if (m_softSyncData && tp < m_softSyncData->dstTimePoint) {
        const PlaybackRate rate =
                tp > m_softSyncData->srcTimePoint ? m_softSyncData->internalRate : m_playbackRate;

        return m_softSyncData->srcPosition
                + toTrackDuration(tp - m_softSyncData->srcTimePoint, rate);
    }

    return positionFromTimeInternal(tp);
}

TimeController::TimePoint TimeController::timeFromPosition(TrackPosition pos,
                                                           bool ignorePause) const
{
    auto position = m_paused && !ignorePause ? m_position : TrackPosition(pos);

    if (m_softSyncData && position < m_softSyncData->dstPosition) {
        const auto rate = position > m_softSyncData->srcPosition ? m_softSyncData->internalRate
                                                                 : m_playbackRate;
        return m_softSyncData->srcTimePoint
                + toClockDuration(position - m_softSyncData->srcPosition, rate);
    }

    return timeFromPositionInternal(position);
}

TimeController::SoftSyncData TimeController::makeSoftSyncData(const TimePoint &srcTp,
                                                              const TrackPosition &srcPos,
                                                              const TimePoint &dstTp) const
{
    SoftSyncData result;
    result.srcTimePoint = srcTp;
    result.srcPosition = srcPos;
    result.dstTimePoint = dstTp;
    result.srcPosOffest = srcPos - positionFromTimeInternal(srcTp);
    result.dstPosition = positionFromTimeInternal(dstTp);
    result.internalRate =
            static_cast<PlaybackRate>(toClockDuration(result.dstPosition - srcPos).count())
            / (dstTp - srcTp).count();

    return result;
}

TrackPosition TimeController::positionFromTimeInternal(const TimePoint &tp) const
{
    return m_position + toTrackDuration(tp - m_timePoint, m_playbackRate);
}

TimeController::TimePoint TimeController::timeFromPositionInternal(const TrackPosition &pos) const
{
    return m_timePoint + toClockDuration(pos - m_position, m_playbackRate);
}

void TimeController::scrollTimeTillNow()
{
    const auto now = SteadyClock::now();
    if (!m_paused) {
        m_position = positionFromTimeInternal(now);

        // let's forget outdated syncronizations
        if (m_softSyncData && m_softSyncData->dstTimePoint <= now)
            m_softSyncData.reset();
    } else if (m_softSyncData) {
        m_softSyncData->dstTimePoint += now - m_timePoint;
        m_softSyncData->srcTimePoint += now - m_timePoint;
    }

    m_timePoint = now;
}

SteadyClock::duration TimeController::toClockDuration(TrackDuration trackDuration, PlaybackRate rate)
{
    return std::chrono::duration_cast<SteadyClock::duration>(
            std::chrono::microseconds(trackDuration.get()) / rate);
}

TrackDuration TimeController::toTrackDuration(SteadyClock::duration clockDuration, PlaybackRate rate)
{
    return TrackDuration(
            std::chrono::duration_cast<std::chrono::microseconds>(clockDuration * rate).count());
}

} // namespace QFFmpeg

QT_END_NAMESPACE
