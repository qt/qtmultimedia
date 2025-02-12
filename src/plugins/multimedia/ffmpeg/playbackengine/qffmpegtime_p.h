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

#include "private/qtaggedtime_p.h"
#include "qffmpeg_p.h"

#include <chrono>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

using TrackTime = std::chrono::microseconds; // track position in microseconds, used as
                                             // a general time position in the playback engine

struct UserTrackTimeTag;
using UserTrackTime =
        QTaggedTime<qint64, UserTrackTimeTag>; // track position in milliseconds, that matches
                                               // the postion in the public API

struct AVStreamTimeTag;
using AVStreamTime = QTaggedTime<qint64, AVStreamTimeTag>; // position in AVStream, in
                                                           // 'AVStream::time_base * 1sec' units

struct AVContextTimeTag;
using AVContextTime =
        QTaggedTime<qint64, AVContextTimeTag>; // position in the AVFormatContext, in '1sec /
                                               // AV_TIME_BASE' units, which is actually
                                               // microseconds. The position is shifted on
                                               // AVFormatContext::start_time from TrackTime.

inline AVContextTime contextStartTime(const AVFormatContext *formatContext) {
    return AVContextTime(formatContext->start_time == AV_NOPTS_VALUE ? 0 : formatContext->start_time);
}

inline UserTrackTime toUserTrackTime(TrackTime trackTime)
{
    return UserTrackTime(std::chrono::duration_cast<std::chrono::milliseconds>(trackTime).count());
}

inline TrackTime toTrackTime(UserTrackTime userTrackTime)
{
    return std::chrono::milliseconds(userTrackTime.get());
}

inline TrackTime toTrackTime(AVContextTime contextTime) {
    return TrackTime(contextTime.get() * 1'000'000 / AV_TIME_BASE);
}

inline TrackTime toTrackDuration(AVStreamTime streamTime, const AVStream *avStream)
{
    return TrackTime(timeStampUs(streamTime.get(), avStream->time_base).value_or(0));
}

inline TrackTime toTrackTimePoint(AVStreamTime streamTime, const AVStream *avStream,
                                  const AVFormatContext *formatContext)
{
    return toTrackDuration(streamTime, avStream) - toTrackTime(contextStartTime(formatContext));
}

inline AVContextTime toContextTimePoint(TrackTime trackTime, const AVFormatContext *formatContext)
{

    return AVContextTime(trackTime.count() * AV_TIME_BASE / 1'000'000) + contextStartTime(formatContext);
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGTIME_P_H
