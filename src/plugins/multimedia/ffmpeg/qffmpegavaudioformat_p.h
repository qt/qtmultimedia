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

#include <QtFFmpegMediaPluginImpl/private/qffmpegdefs_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
inline bool operator==(const AVChannelLayout &lhs, const AVChannelLayout &rhs)
{
    return lhs.order == rhs.order && lhs.nb_channels == rhs.nb_channels && lhs.u.mask == rhs.u.mask;
}

inline bool operator!=(const AVChannelLayout &lhs, const AVChannelLayout &rhs)
{
    return !(lhs == rhs);
}

#endif

QT_BEGIN_NAMESPACE

class QAudioFormat;

namespace QFFmpeg {

struct AVAudioFormat
{
    AVAudioFormat(const AVCodecContext *context);

    AVAudioFormat(const AVCodecParameters *codecPar);

    AVAudioFormat(const QAudioFormat &audioFormat);

#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
    AVChannelLayout channelLayout;
#else
    uint64_t channelLayoutMask;
#endif
    AVSampleFormat sampleFormat;
    int sampleRate;
};

bool operator==(const AVAudioFormat &lhs, const AVAudioFormat &rhs);

inline bool operator!=(const AVAudioFormat &lhs, const AVAudioFormat &rhs)
{
    return !(lhs == rhs);
}

QDebug operator<<(QDebug, const AVAudioFormat &);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGAVAUDIOFORMAT_P_H
