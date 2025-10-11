// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEG_P_H
#define QFFMPEG_P_H

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
#include "qffmpegavaudioformat_p.h"

#include <qstring.h>
#include <optional>

inline bool operator==(const AVRational &lhs, const AVRational &rhs)
{
    return lhs.num == rhs.num && lhs.den == rhs.den;
}

inline bool operator!=(const AVRational &lhs, const AVRational &rhs)
{
    return !(lhs == rhs);
}

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

inline void setAVFrameTime(AVFrame &frame, int64_t pts, const AVRational &timeBase)
{
    frame.pts = pts;
#if QT_FFMPEG_HAS_FRAME_TIME_BASE
    frame.time_base = timeBase;
#else
    Q_UNUSED(timeBase);
#endif
}

inline void getAVFrameTime(const AVFrame &frame, int64_t &pts, AVRational &timeBase)
{
    pts = frame.pts;
#if QT_FFMPEG_HAS_FRAME_TIME_BASE
    timeBase = frame.time_base;
#else
    timeBase = { 0, 1 };
#endif
}

inline int64_t getAVFrameDuration(const AVFrame &frame)
{
#if QT_FFMPEG_HAS_FRAME_DURATION
    return frame.duration;
#else
    Q_UNUSED(frame);
    return 0;
#endif
}

struct AVDictionaryHolder
{
    AVDictionary *opts = nullptr;

    operator AVDictionary **() { return &opts; }

    AVDictionaryHolder() = default;

    Q_DISABLE_COPY(AVDictionaryHolder)

    AVDictionaryHolder(AVDictionaryHolder &&other) noexcept
        : opts(std::exchange(other.opts, nullptr))
    {
    }

    ~AVDictionaryHolder()
    {
        if (opts)
            av_dict_free(&opts);
    }
};

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

using AVHWFramesConstraintsUPtr = std::unique_ptr<
        AVHWFramesConstraints,
        AVDeleter<decltype(&av_hwframe_constraints_free), &av_hwframe_constraints_free>>;

using SwrContextUPtr = std::unique_ptr<SwrContext, AVDeleter<decltype(&swr_free), &swr_free>>;

using PixelOrSampleFormat = int;
using AVScore = int;
constexpr AVScore BestAVScore = std::numeric_limits<AVScore>::max();
constexpr AVScore DefaultAVScore = 0;
constexpr AVScore NotSuitableAVScore = std::numeric_limits<AVScore>::min();
constexpr AVScore MinAVScore = NotSuitableAVScore + 1;

const AVCodec *findAVDecoder(AVCodecID codecId,
                             const std::optional<AVHWDeviceType> &deviceType = {},
                             const std::optional<PixelOrSampleFormat> &format = {});

const AVCodec *findAVEncoder(AVCodecID codecId,
                             const std::optional<AVHWDeviceType> &deviceType = {},
                             const std::optional<PixelOrSampleFormat> &format = {});

const AVCodec *findAVEncoder(AVCodecID codecId,
                             const std::function<AVScore(const AVCodec *)> &scoresGetter);

bool isAVFormatSupported(const AVCodec *codec, PixelOrSampleFormat format);

template<typename Format>
bool hasAVFormat(const Format *fmts, Format format)
{
    return findAVFormat(fmts, [format](Format f) { return f == format; }) != Format(-1);
}

template<typename Format, typename Predicate>
Format findAVFormat(const Format *fmts, const Predicate &predicate)
{
    auto scoresGetter = [&predicate](Format fmt) {
        return predicate(fmt) ? BestAVScore : NotSuitableAVScore;
    };
    return findBestAVFormat(fmts, scoresGetter).first;
}

template <typename Predicate>
const AVCodecHWConfig *findHwConfig(const AVCodec *codec, const Predicate &predicate)
{
    for (int i = 0; const auto hwConfig = avcodec_get_hw_config(codec, i); ++i) {
        if (predicate(hwConfig))
            return hwConfig;
    }

    return nullptr;
}

template <typename Predicate>
AVPixelFormat findAVPixelFormat(const AVCodec *codec, const Predicate &predicate)
{
    const AVPixelFormat format = findAVFormat(codec->pix_fmts, predicate);
    if (format != AV_PIX_FMT_NONE)
        return format;

    auto checkHwConfig = [&predicate](const AVCodecHWConfig *config) {
        return config->pix_fmt != AV_PIX_FMT_NONE && predicate(config->pix_fmt);
    };

    if (auto hwConfig = findHwConfig(codec, checkHwConfig))
        return hwConfig->pix_fmt;

    return AV_PIX_FMT_NONE;
}

template <typename Value, typename CalculateScore>
auto findBestAVValue(const Value *values, const CalculateScore &calculateScore,
                     Value invalidValue = {})
{
    using Limits = std::numeric_limits<decltype(calculateScore(*values))>;
    std::pair result(invalidValue, Limits::min());
    if (values) {
        for (; *values != invalidValue && result.second != Limits::max(); ++values) {
            const auto score = calculateScore(*values);
            if (score > result.second)
                result = { *values, score };
        }
    }

    return result;
}

template <typename Format, typename CalculateScore>
std::pair<Format, AVScore> findBestAVFormat(const Format *fmts,
                                            const CalculateScore &calculateScore)
{
    static_assert(std::is_same_v<Format, AVSampleFormat> || std::is_same_v<Format, AVPixelFormat>,
                  "The input value is not AV format, use findBestAVValue instead.");
    return findBestAVValue(fmts, calculateScore, Format(-1));
}

bool isHwPixelFormat(AVPixelFormat format);

inline bool isSwPixelFormat(AVPixelFormat format)
{
    return !isHwPixelFormat(format);
}

bool isAVCodecExperimental(const AVCodec *codec);

void applyExperimentalCodecOptions(const AVCodec *codec, AVDictionary** opts);

AVPixelFormat pixelFormatForHwDevice(AVHWDeviceType deviceType);

const AVPacketSideData *streamSideData(const AVStream *stream, AVPacketSideDataType type);

SwrContextUPtr createResampleContext(const AVAudioFormat &inputFormat,
                                     const AVAudioFormat &outputFormat);

#ifdef Q_OS_DARWIN
bool isCVFormatSupported(uint32_t format);

std::string cvFormatToString(uint32_t format);

#endif
}

QDebug operator<<(QDebug, const AVRational &);

QT_END_NAMESPACE

#endif
