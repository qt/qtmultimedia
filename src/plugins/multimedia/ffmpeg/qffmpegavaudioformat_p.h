// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGAVAUDIOFORMAT_P_H
#define QFFMPEGAVAUDIOFORMAT_P_H

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

#include "qffmpegdefs_p.h"
#include <private/qtmultimediaglobal_p.h>

QT_BEGIN_NAMESPACE

class QAudioFormat;

namespace QFFmpeg {

struct AVAudioFormat
{
    AVAudioFormat(const AVCodecParameters *codecPar);

    AVAudioFormat(const QAudioFormat &audioFormat);

#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    uint64_t channelLayoutMask;
#else
    AVChannelLayout channelLayout;
#endif
    AVSampleFormat sampleFormat;
    int sampleRate;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGAVAUDIOFORMAT_P_H
