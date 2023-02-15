// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEG_P_H
#define QFFMPEG_P_H

#include <private/qtmultimediaglobal_p.h>
#include <qstring.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#define QT_FFMPEG_OLD_CHANNEL_LAYOUT (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59,24,100))

QT_BEGIN_NAMESPACE

namespace QFFmpeg
{

inline std::optional<qint64> mul(qint64 a, AVRational b)
{
    return b.den != 0 ? (a * b.num + b.den / 2) / b.den : std::optional<qint64>{};
}

inline std::optional<qreal> mul(qreal a, AVRational b)
{
    return b.den != 0 ? a * qreal(b.num) / qreal(b.den) : std::optional<qreal>{};
}

inline std::optional<qint64> timeStampMs(qint64 ts, AVRational base)
{
    return mul(1'000 * ts, base);
}

inline std::optional<qint64> timeStampUs(qint64 ts, AVRational base)
{
    return mul(1'000'000 * ts, base);
}

inline std::optional<float> toFloat(AVRational r)
{
    return r.den != 0 ? float(r.num) / float(r.den) : std::optional<float>{};
}

inline QString err2str(int errnum)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE + 1] = {};
    av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, errnum);
    return QString::fromLocal8Bit(buffer);
}

template<typename FunctionType, FunctionType F>
struct AVDeleter
{
    template<typename T>
    void operator()(T *object) const
    {
        if (object)
            F(&object);
    }
};

using AVFrameUPtr = std::unique_ptr<AVFrame, AVDeleter<decltype(&av_frame_free), &av_frame_free>>;

inline AVFrameUPtr makeAVFrame()
{
    return AVFrameUPtr(av_frame_alloc());
}

using AVPacketUPtr =
        std::unique_ptr<AVPacket, AVDeleter<decltype(&av_packet_free), &av_packet_free>>;

using AVCodecContextUPtr =
        std::unique_ptr<AVCodecContext,
                        AVDeleter<decltype(&avcodec_free_context), &avcodec_free_context>>;

using AVBufferUPtr =
        std::unique_ptr<AVBufferRef, AVDeleter<decltype(&av_buffer_unref), &av_buffer_unref>>;

QT_END_NAMESPACE

}

#endif
