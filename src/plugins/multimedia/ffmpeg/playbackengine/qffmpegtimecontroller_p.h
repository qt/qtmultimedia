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
#include <QtFFmpegMediaPluginImpl/private/qffmpegtime_p.h>

#include <chrono>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class TimeController
{
public:
    using TimePoint = RealClock::time_point;
    using PlaybackRate = float;

    TimeController();

    PlaybackRate playbackRate() const;

    void setPlaybackRate(PlaybackRate playbackRate);

    void sync(TrackPosition trackPos = TrackPosition(0));

    void sync(TimePoint tp, TrackPosition pos);

    void syncSoft(TimePoint tp, TrackPosition pos,
                  RealClock::duration fixingTime = std::chrono::seconds(4));

    TrackPosition currentPosition(RealClock::duration offset = RealClock::duration{ 0 }) const;

    void setPaused(bool paused);

    TrackPosition positionFromTime(TimePoint tp, bool ignorePause = false) const;

    TimePoint timeFromPosition(TrackPosition pos, bool ignorePause = false) const;

private:
    struct SoftSyncData
    {
        TimePoint srcTimePoint;
        TrackPosition srcPosition = 0;
        TimePoint dstTimePoint;
        TrackDuration srcPosOffest = 0;
        TrackPosition dstPosition = 0;
        PlaybackRate internalRate = 1;
    };

    SoftSyncData makeSoftSyncData(const TimePoint &srcTp, const TrackPosition &srcPos,
                                  const TimePoint &dstTp) const;

    TrackPosition positionFromTimeInternal(const TimePoint &tp) const;

    TimePoint timeFromPositionInternal(const TrackPosition &pos) const;

    void scrollTimeTillNow();

    static RealClock::duration toClockDuration(TrackDuration trackDuration,
                                               PlaybackRate rate = 1.f);

    static TrackDuration toTrackDuration(RealClock::duration clockDuration, PlaybackRate rate);

private:
    bool m_paused = true;
    PlaybackRate m_playbackRate = 1;
    TrackPosition m_position = 0;
    TimePoint m_timePoint;
    std::optional<SoftSyncData> m_softSyncData;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGTIMECONTROLLER_P_H
