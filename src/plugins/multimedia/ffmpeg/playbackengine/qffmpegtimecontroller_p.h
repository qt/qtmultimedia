// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGTIMECONTROLLER_P_H
#define QFFMPEGTIMECONTROLLER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qglobal.h"

#include <chrono>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class TimeController
{
    using TrackTime = std::chrono::microseconds;

public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using PlaybackRate = float;

    TimeController();

    float playbackRate() const;

    void setPlaybackRate(PlaybackRate playbackRate);

    void sync(qint64 trackPos = 0);

    void sync(const TimePoint &tp, qint64 pos);

    void syncSoft(const TimePoint &tp, qint64 pos,
                  const Clock::duration &fixingTime = std::chrono::seconds(4));

    qint64 currentPosition(const Clock::duration &offset = Clock::duration{ 0 }) const;

    void setPaused(bool paused);

    qint64 positionFromTime(TimePoint tp, bool ignorePause = false) const;

    TimePoint timeFromPosition(qint64 pos, bool ignorePause = false) const;

private:
    struct SoftSyncData
    {
        TimePoint srcTimePoint;
        TrackTime srcPosition;
        TimePoint dstTimePoint;
        TrackTime srcPosOffest;
        TrackTime dstPosition;
        PlaybackRate internalRate = 1;
    };

    SoftSyncData makeSoftSyncData(const TimePoint &srcTp, const TrackTime &srcPos,
                                  const TimePoint &dstTp) const;

    TrackTime positionFromTimeInternal(const TimePoint &tp) const;

    TimePoint timeFromPositionInternal(const TrackTime &pos) const;

    void scrollTimeTillNow();

    template<typename T>
    static Clock::duration toClockTime(const T &t);

    template<typename T>
    static TrackTime toTrackTime(const T &t);

private:
    bool m_paused = true;
    PlaybackRate m_playbackRate = 1;
    TrackTime m_position;
    TimePoint m_timePoint = {};
    std::optional<SoftSyncData> m_softSyncData;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGTIMECONTROLLER_P_H
