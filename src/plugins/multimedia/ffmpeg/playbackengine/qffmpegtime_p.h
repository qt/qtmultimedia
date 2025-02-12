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

#include <chrono>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

using TrackTime = std::chrono::microseconds; // track position in microseconds, used as
                                             // a general time position in the playback engine
using UserTrackTime =
        qint64; // track position in milliseconds, that matches the postion in the public API
using AVStreamTime = qint64; // position in AVStream, in 'AVStream::time_base * 1sec' units
using AVContextTime = qint64; // position in the AVFormatContext, in '1sec / AV_TIME_BASE' units,
                              // which is actually microseconds. The position is shifted on
                              // AVFormatContext::start_time from TrackTime.

inline UserTrackTime toUserTrackTime(TrackTime trackTime)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(trackTime).count();
}

inline TrackTime fromUserTrackTime(UserTrackTime userTrackTime)
{
    return std::chrono::milliseconds(userTrackTime);
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGTIME_P_H
