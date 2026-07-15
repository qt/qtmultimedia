// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGDEFS_P_H
#define QFFMPEGDEFS_P_H

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

#include "qtconfigmacros.h"

#include <limits>
#include <unordered_set>
#include <variant>
#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

static_assert(LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 70, 100),
              "FFmpeg 4.4 or newer is required (libavutil >= 56.70)");

#define QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT \
    (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 24, 100)) // since FFmpeg n5.1
#define QT_FFMPEG_HAS_FRAME_TIME_BASE \
    (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 18, 100)) // since FFmpeg n5.0
#define QT_FFMPEG_HAS_FRAME_DURATION \
    (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(60, 3, 100)) // since FFmpeg n6.0
#define QT_FFMPEG_STREAM_SIDE_DATA_DEPRECATED \
    (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(60, 15, 100)) // since FFmpeg n6.1
#define QT_FFMPEG_HAS_D3D12VA \
    (LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(59, 8, 100)) // since FFmpeg n7.0
#define QT_FFMPEG_SWR_CONST_CH_LAYOUT \
    (LIBSWRESAMPLE_VERSION_INT >= AV_VERSION_INT(4, 9, 100))
#define QT_FFMPEG_AVIO_WRITE_CONST \
    (LIBAVFORMAT_VERSION_MAJOR >= 61)
#define QT_CODEC_PARAMETERS_HAVE_FRAMERATE \
    (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(60, 11, 100)) // since FFmpeg n6.1
#define QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG \
    (LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(59, 39, 100)) // since FFmpeg n7.1
#define QT_FFMPEG_HAS_SWS_FLAGS_ENUM \
    (LIBSWSCALE_VERSION_INT >= AV_VERSION_INT(9, 1, 100)) // since FFmpeg n8.0

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
using ChannelLayoutT = AVChannelLayout;
#else
using ChannelLayoutT = uint64_t;
#endif

#if !QT_FFMPEG_HAS_SWS_FLAGS_ENUM
using SwsFlags = int;
#endif

enum class AVScore : int {
    BestAVScore = std::numeric_limits<int>::max(),
    DefaultAVScore = 0,
    NotSuitableAVScore = std::numeric_limits<int>::min(),
    MinAVScore = std::numeric_limits<int>::min() + 1,
};

inline constexpr AVScore BestAVScore = AVScore::BestAVScore;
inline constexpr AVScore DefaultAVScore = AVScore::DefaultAVScore;
inline constexpr AVScore NotSuitableAVScore = AVScore::NotSuitableAVScore;
inline constexpr AVScore MinAVScore = AVScore::MinAVScore;

constexpr inline AVScore operator+(AVScore a, int b)
{
    return AVScore(qToUnderlying(a) + b);
}
constexpr inline AVScore operator-(AVScore a, int b)
{
    return AVScore(qToUnderlying(a) - b);
}
constexpr inline AVScore operator+(int a, AVScore b)
{
    return AVScore(a + qToUnderlying(b));
}
constexpr inline AVScore operator-(int a, AVScore b)
{
    return AVScore(a - qToUnderlying(b));
}
constexpr inline AVScore operator-(AVScore a, AVScore b)
{
    return AVScore(qToUnderlying(a) - qToUnderlying(b));
}
constexpr inline AVScore &operator+=(AVScore &a, int b)
{
    return a = a + b;
}
constexpr inline AVScore &operator-=(AVScore &a, int b)
{
    return a = a - b;
}
inline QDebug operator<<(QDebug dbg, AVScore score)
{
    dbg << qToUnderlying(score);
    return dbg;
}

using PixelOrSampleFormat = std::variant<AVPixelFormat, AVSampleFormat>;
using AVPixelFormatSet = std::unordered_set<AVPixelFormat>;

} // namespace QFFmpeg

QT_END_NAMESPACE

namespace std {

template <>
struct numeric_limits<QT_PREPEND_NAMESPACE(QFFmpeg)::AVScore>
{
    using Type = QT_PREPEND_NAMESPACE(QFFmpeg)::AVScore;
    constexpr static Type min() noexcept { return Type::NotSuitableAVScore; }
    constexpr static Type max() noexcept { return Type::BestAVScore; }
};

} // namespace std

#ifndef AV_PROFILE_H264_HIGH
#  define AV_PROFILE_H264_HIGH FF_PROFILE_H264_HIGH
#endif
#ifndef AV_PROFILE_HEVC_MAIN
#  define AV_PROFILE_HEVC_MAIN FF_PROFILE_HEVC_MAIN
#endif

#endif // QFFMPEGDEFS_P_H
