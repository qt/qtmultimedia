// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef qffmpegplaybackutils_p_H
#define qffmpegplaybackutils_p_H

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

#include <qtypes.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegtime_p.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

struct LoopOffset
{
    TrackPosition loopStartTimeUs =
            TrackPosition(0); // Accumulated duration of previous loops (microseconds)
    int loopIndex = 0; // Counts the number of times the media has been played
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // qffmpegplaybackutils_p_H
