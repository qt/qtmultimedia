// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGTIME_P_H
#define QFFMPEGTIME_P_H

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

#include <QtCore/qdebug.h>
#include <QtMultimedia/private/qtaggedtime_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>

#include <chrono>

#ifdef Q_OS_WIN
#  include <QtCore/qt_windows.h>
#  include <QtCore/private/qwin32ticks_p.h>
#  include <realtimeapiset.h>
#endif

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

using namespace std::chrono_literals;

struct TrackTimeTag;
// track position in microseconds, used as
// a general time position in the playback engine
using TrackPosition = QTaggedTimePoint<qint64, TrackTimeTag>;
using TrackDuration = QTaggedDuration<qint64, TrackTimeTag>;

struct UserTrackTimeTag;
// track position in milliseconds, that matches the postion in the public API
using UserTrackPosition = QTaggedTimePoint<qint64, UserTrackTimeTag>;
using UserTrackDuration = QTaggedDuration<qint64, UserTrackTimeTag>;

struct AVStreamTimeTag;
// position in AVStream, in 'AVStream::time_base * 1sec' units
using AVStreamPosition = QTaggedTimePoint<qint64, AVStreamTimeTag>;
using AVStreamDuration = QTaggedDuration<qint64, AVStreamTimeTag>;

struct AVContextTimeTag;
// position in the AVFormatContext, in '1sec / AV_TIME_BASE' units, which is actually
// microseconds. The position is shifted on AVFormatContext::start_time from TrackTime.
using AVContextPosition = QTaggedTimePoint<qint64, AVContextTimeTag>;
using AVContextDuration = QTaggedDuration<qint64, AVContextTimeTag>;

#ifdef Q_OS_WIN
// std::chrono::steady_clock is based on QueryPerformanceCounter on Windows, which keeps counting
// while the system is suspended. After wake-up the playback timeline would be ahead of the media
// by the whole sleeping time, and video-only media would play extremely fast. The interrupt time
// doesn't include the sleeping time, like CLOCK_MONOTONIC on Linux and CLOCK_UPTIME_RAW on macOS.
struct UnbiasedInterruptClock
{
    using duration = QtPrivate::win32_ticks;
    using time_point = std::chrono::time_point<UnbiasedInterruptClock, duration>;

    static time_point now() noexcept
    {
        ULONGLONG interruptTime = 0; // 100 ns units, excludes the time in sleep/hibernation
        QueryUnbiasedInterruptTimePrecise(&interruptTime);
        return time_point(duration(static_cast<duration::rep>(interruptTime)));
    }
};

using SteadyClock = UnbiasedInterruptClock;
#else
using SteadyClock = std::chrono::steady_clock;
#endif

inline std::optional<AVContextDuration> contextStartOffset(const AVFormatContext *formatContext)
{
    if (formatContext->start_time == AV_NOPTS_VALUE)
        return std::nullopt;
    return AVContextDuration(formatContext->start_time);
}

inline UserTrackPosition toUserPosition(TrackPosition trackPosition)
{
    return UserTrackPosition(trackPosition.get() / 1000);
}

inline UserTrackDuration toUserDuration(TrackDuration trackDuration)
{
    return UserTrackDuration(trackDuration.get() / 1000);
}

inline TrackDuration toTrackDuration(AVContextDuration contextDuration)
{
    return TrackDuration(contextDuration.get() * 1'000'000 / AV_TIME_BASE);
}

inline TrackPosition toTrackPosition(UserTrackPosition userTrackPosition)
{
    return TrackPosition(userTrackPosition.get() * 1000);
}

inline TrackDuration toTrackDuration(UserTrackDuration userTrackDuration)
{
    return TrackDuration(userTrackDuration.get() * 1000);
}

inline TrackDuration toTrackDuration(AVStreamDuration streamDuration, const AVStream *avStream)
{
    return TrackDuration(timeStampUs(streamDuration.get(), avStream->time_base).value_or(0));
}

inline TrackPosition toTrackPosition(AVStreamPosition streamPosition, const AVStream *avStream,
                                     const AVFormatContext *formatContext)
{
    auto duration = toTrackDuration(streamPosition.asDuration(), avStream);
    if (formatContext) {
        std::optional contextStart = contextStartOffset(formatContext);
        if (contextStart)
            duration -= toTrackDuration(*contextStart);
    }

    return duration.asTimePoint();
}

inline AVContextPosition toContextPosition(TrackPosition trackPosition,
                                           const AVFormatContext *formatContext)
{
    return AVContextPosition(trackPosition.get() * AV_TIME_BASE / 1'000'000)
            + contextStartOffset(formatContext).value_or(AVContextDuration::zero());
}

inline QDebug operator<<(QDebug debug, const TrackPosition &position)
{
    return debug << "TrackPosition{" << position.get() << "us}";
}

inline QDebug operator<<(QDebug debug, const TrackDuration &duration)
{
    return debug << "TrackDuration{" << duration.get() << "us}";
}

inline QDebug operator<<(QDebug debug, const UserTrackPosition &position)
{
    return debug << "UserTrackPosition{" << position.get() << "ms}";
}

inline QDebug operator<<(QDebug debug, const UserTrackDuration &duration)
{
    return debug << "UserTrackDuration{" << duration.get() << "ms}";
}

inline QDebug operator<<(QDebug debug, const AVStreamPosition &position)
{
    return debug << "AVStreamPosition{" << position.get() << "stream units}";
}

inline QDebug operator<<(QDebug debug, const AVStreamDuration &duration)
{
    return debug << "AVStreamDuration{" << duration.get() << "stream units}";
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGTIME_P_H
