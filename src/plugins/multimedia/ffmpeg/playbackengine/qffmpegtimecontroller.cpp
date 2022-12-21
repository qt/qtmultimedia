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

float TimeController::playbackRate() const
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
        m_softSyncData = makeSoftSyncData(m_softSyncData->srcTimePoint, m_softSyncData->srcPosition,
                                          m_softSyncData->dstTimePoint);
}

void TimeController::sync(qint64 trackPos)
{
    sync(Clock::now(), trackPos);
}

void TimeController::sync(const TimePoint &tp, qint64 pos)
{
    m_softSyncData.reset();
    m_position = TrackTime(pos);
    m_timePoint = tp;
}

void TimeController::syncSoft(const TimePoint &tp, qint64 pos, const Clock::duration &fixingTime)
{
    const auto srcTime = Clock::now();
    const auto srcPos = positionFromTime(srcTime, true);
    const auto dstTime = srcTime + fixingTime;

    m_position = TrackTime(pos);
    m_timePoint = tp;

    m_softSyncData = makeSoftSyncData(srcTime, TrackTime(srcPos), dstTime);
}

qint64 TimeController::currentPosition(const Clock::duration &offset) const
{
    return positionFromTime(Clock::now() + offset);
}

void TimeController::setPaused(bool paused)
{
    if (m_paused == paused)
        return;

    scrollTimeTillNow();
    m_paused = paused;
}

qint64 TimeController::positionFromTime(TimePoint tp, bool ignorePause) const
{
    tp = m_paused && !ignorePause ? m_timePoint : tp;

    if (m_softSyncData && tp < m_softSyncData->dstTimePoint) {
        const float rate =
                tp > m_softSyncData->srcTimePoint ? m_softSyncData->internalRate : m_playbackRate;

        return (m_softSyncData->srcPosition
                + toTrackTime((tp - m_softSyncData->srcTimePoint) * rate))
                .count();
    }

    return positionFromTimeInternal(tp).count();
}

TimeController::TimePoint TimeController::timeFromPosition(qint64 pos, bool ignorePause) const
{
    auto position = m_paused && !ignorePause ? m_position : TrackTime(pos);

    if (m_softSyncData && position < m_softSyncData->dstPosition) {
        const auto rate = position > m_softSyncData->srcPosition ? m_softSyncData->internalRate
                                                                 : m_playbackRate;
        return m_softSyncData->srcTimePoint
                + toClockTime((position - m_softSyncData->srcPosition) / rate);
    }

    return timeFromPositionInternal(position);
}

TimeController::SoftSyncData TimeController::makeSoftSyncData(const TimePoint &srcTp,
                                                              const TrackTime &srcPos,
                                                              const TimePoint &dstTp) const
{
    SoftSyncData result;
    result.srcTimePoint = srcTp;
    result.srcPosition = srcPos;
    result.dstTimePoint = dstTp;
    result.srcPosOffest = srcPos - positionFromTimeInternal(srcTp);
    result.dstPosition = positionFromTimeInternal(dstTp);
    result.internalRate =
            static_cast<PlaybackRate>(toClockTime(TrackTime(result.dstPosition - srcPos)).count())
            / (dstTp - srcTp).count();

    return result;
}

TimeController::TrackTime TimeController::positionFromTimeInternal(const TimePoint &tp) const
{
    return m_position + toTrackTime((tp - m_timePoint) * m_playbackRate);
}

TimeController::TimePoint TimeController::timeFromPositionInternal(const TrackTime &pos) const
{
    return m_timePoint + toClockTime(TrackTime(pos - m_position) / m_playbackRate);
}

void TimeController::scrollTimeTillNow()
{
    const auto now = Clock::now();
    if (!m_paused)
        m_position = positionFromTimeInternal(now);
    else if (m_softSyncData) {
        m_softSyncData->dstTimePoint += now - m_timePoint;
        m_softSyncData->srcTimePoint += now - m_timePoint;
    }

    m_timePoint = now;
}

template<typename T>
TimeController::Clock::duration TimeController::toClockTime(const T &t)
{
    return std::chrono::duration_cast<Clock::duration>(t);
}

template<typename T>
TimeController::TrackTime TimeController::toTrackTime(const T &t)
{
    return std::chrono::duration_cast<TrackTime>(t);
}

} // namespace QFFmpeg

QT_END_NAMESPACE
