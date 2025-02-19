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

#include <QtFFmpegMediaPluginImpl/private/qffmpegdefs_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegcodec_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegavaudioformat_p.h>
#include <QtMultimedia/qvideoframeformat.h>

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
    if (b.den == 0)
        return {};

    auto multiplyAndRound = [](qint64 a, AVRational b) { //
        return (a * b.num + b.den / 2) / b.den;
    };

    if (a < 0)
        return -multiplyAndRound(-a, b);
    else
        return multiplyAndRound(a, b);
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
    return frame.pkt_duration;
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
    template <typename T, std::invoke_result_t<FunctionType, T **> * = nullptr>
    void operator()(T *object) const
    {
        if (object)
            F(&object);
    }

    template <typename T, std::invoke_result_t<FunctionType, T *> * = nullptr>
    void operator()(T *object) const
    {
        F(object);
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

using SwsContextUPtr =
        std::unique_ptr<SwsContext, AVDeleter<decltype(&sws_freeContext), &sws_freeContext>>;

bool isAVFormatSupported(const Codec &codec, PixelOrSampleFormat format);

// Returns true if the range contains the value, false otherwise
template <typename Value>
bool hasValue(QSpan<const Value> range, Value value)
{
    return std::find(range.begin(), range.end(), value) != range.end();
}

// Search for the first element in the range that satisfies the predicate
// The predicate is evaluated for each value in the range until it returns
// true, and the corresponding value is returned. If no match is found,
// std::nullopt is returned.
template <typename Value, typename Predicate>
std::optional<Value> findIf(QSpan<const Value> range, const Predicate &predicate)
{
    const auto value = std::find_if(range.begin(), range.end(), predicate);
    if (value == range.end())
        return {};
    return *value;
}

// Search the codec's pixel formats for a format that matches the predicate.
// If no pixel format is found, repeat the search through the pixel formats
// of all the codec's hardware configs. If no matching pixel format is found,
// std::nullopt is returned. The predicate is evaluated once for each pixel
// format until the predicate returns true.
template <typename Predicate>
std::optional<AVPixelFormat> findAVPixelFormat(const Codec &codec, const Predicate &predicate)
{
    if (codec.type() != AVMEDIA_TYPE_VIDEO)
        return {};

    const auto pixelFormats = codec.pixelFormats();

    if (const auto format = findIf(pixelFormats, predicate))
        return format;

    // No matching pixel format was found. Check the pixel format
    // of the codec's hardware config.
    for (const AVCodecHWConfig *const config : codec.hwConfigs()) {
        const AVPixelFormat format = config->pix_fmt;

        if (format == AV_PIX_FMT_NONE)
            continue;

        if (predicate(format))
            return format;
    }
    return {};
}

// Evaluate the function for each of the codec's pixel formats and each of
// the pixel formats supported by the codec's hardware configs.
template <typename Function>
void forEachAVPixelFormat(const Codec &codec, const Function &function)
{
    findAVPixelFormat(codec, [&function](AVPixelFormat format) {
        function(format);
        return false; // Evaluate the function for all pixel formats
    });
}

template <typename ValueT, typename ScoreT = AVScore>
struct ValueAndScore
{
    std::optional<ValueT> value;
    ScoreT score = std::numeric_limits<ScoreT>::min();
};

template <typename Value, typename CalculateScore,
          typename ScoreType = std::invoke_result_t<CalculateScore, Value>>
ValueAndScore<Value, ScoreType> findBestAVValueWithScore(QSpan<const Value> values,
                                                         const CalculateScore &calculateScore)
{
    static_assert(std::is_invocable_v<CalculateScore, Value>);

    ValueAndScore<Value, ScoreType> result;
    for (const Value &val : values) {
        const ScoreType score = calculateScore(val);
        if (score > result.score)
            result = { val, score }; // Note: Optional is only set if score > Limits::min()

        if (result.score == std::numeric_limits<ScoreType>::max())
            break;
    }

    return result;
}

template <typename Value, typename CalculateScore>
std::optional<Value> findBestAVValue(QSpan<const Value> values,
                                     const CalculateScore &calculateScore)
{
    return findBestAVValueWithScore(values, calculateScore).value;
}

bool isHwPixelFormat(AVPixelFormat format);

inline bool isSwPixelFormat(AVPixelFormat format)
{
    return !isHwPixelFormat(format);
}

void applyExperimentalCodecOptions(const Codec &codec, AVDictionary **opts);

AVPixelFormat pixelFormatForHwDevice(AVHWDeviceType deviceType);

AVPacketSideData *addStreamSideData(AVStream *stream, AVPacketSideData sideData);

const AVPacketSideData *streamSideData(const AVStream *stream, AVPacketSideDataType type);

SwrContextUPtr createResampleContext(const AVAudioFormat &inputFormat,
                                     const AVAudioFormat &outputFormat);

QVideoFrameFormat::ColorTransfer fromAvColorTransfer(AVColorTransferCharacteristic colorTrc);

AVColorTransferCharacteristic toAvColorTransfer(QVideoFrameFormat::ColorTransfer colorTrc);

QVideoFrameFormat::ColorSpace fromAvColorSpace(AVColorSpace colorSpace);

AVColorSpace toAvColorSpace(QVideoFrameFormat::ColorSpace colorSpace);

QVideoFrameFormat::ColorRange fromAvColorRange(AVColorRange colorRange);

AVColorRange toAvColorRange(QVideoFrameFormat::ColorRange colorRange);

AVHWDeviceContext *avFrameDeviceContext(const AVFrame *frame);

SwsContextUPtr createSwsContext(const QSize &srcSize, AVPixelFormat srcPixFmt, const QSize &dstSize,
                                AVPixelFormat dstPixFmt, int conversionType = SWS_BICUBIC);

#ifdef Q_OS_DARWIN
bool isCVFormatSupported(uint32_t format);

std::string cvFormatToString(uint32_t format);

#endif
} // namespace QFFmpeg

QDebug operator<<(QDebug, const AVRational &);

#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
QDebug operator<<(QDebug, const AVChannelLayout &);
#endif

QT_END_NAMESPACE

#endif
