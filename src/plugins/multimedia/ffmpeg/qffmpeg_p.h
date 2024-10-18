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

#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
void logGetCodecConfigError(const AVCodec *codec, AVCodecConfig config, int error);

template <typename T>
inline const T *getCodecConfig(const AVCodec *codec, AVCodecConfig config)
{
    const T *result = nullptr;
    const auto error = avcodec_get_supported_config(
            nullptr, codec, config, 0u, reinterpret_cast<const void **>(&result), nullptr);
    if (error != 0) {
        logGetCodecConfigError(codec, config, error);
        return nullptr;
    }
    return result;
}
#endif

inline const AVPixelFormat *getCodecPixelFormats(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<AVPixelFormat>(codec, AV_CODEC_CONFIG_PIX_FORMAT);
#else
    return codec->pix_fmts;
#endif
}

inline const AVSampleFormat *getCodecSampleFormats(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<AVSampleFormat>(codec, AV_CODEC_CONFIG_SAMPLE_FORMAT);
#else
    return codec->sample_fmts;
#endif
}

inline const int *getCodecSampleRates(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<int>(codec, AV_CODEC_CONFIG_SAMPLE_RATE);
#else
    return codec->supported_samplerates;
#endif
}

#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT

inline const AVChannelLayout *getCodecChannelLayouts(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<AVChannelLayout>(codec, AV_CODEC_CONFIG_CHANNEL_LAYOUT);
#else
    return codec->ch_layouts;
#endif
}

#else

inline const uint64_t *getCodecChannelLayouts(const AVCodec *codec)
{
    return codec->channel_layouts;
}

#endif

inline const AVRational *getCodecFrameRates(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<AVRational>(codec, AV_CODEC_CONFIG_FRAME_RATE);
#else
    return codec->supported_framerates;
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

template <typename T>
inline constexpr auto InvalidAvValue = T{};

template<>
inline constexpr auto InvalidAvValue<AVSampleFormat> = AV_SAMPLE_FMT_NONE;

template<>
inline constexpr auto InvalidAvValue<AVPixelFormat> = AV_PIX_FMT_NONE;

bool isAVFormatSupported(const AVCodec *codec, PixelOrSampleFormat format);

template <typename Format>
bool hasAVValue(const Format *fmts, Format format)
{
    return findAVValue(fmts, [format](Format f) { return f == format; }) != InvalidAvValue<Format>;
}

template <typename AVValue, typename Predicate>
AVValue findAVValue(const AVValue *fmts, const Predicate &predicate)
{
    auto scoresGetter = [&predicate](AVValue value) {
        return predicate(value) ? BestAVScore : NotSuitableAVScore;
    };
    return findBestAVValue(fmts, scoresGetter).first;
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
    const AVPixelFormat format = findAVValue(codec->pix_fmts, predicate);
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
auto findBestAVValue(const Value *values, const CalculateScore &calculateScore)
{
    using Limits = std::numeric_limits<decltype(calculateScore(*values))>;

    const Value invalidValue = InvalidAvValue<Value>;
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

bool isHwPixelFormat(AVPixelFormat format);

inline bool isSwPixelFormat(AVPixelFormat format)
{
    return !isHwPixelFormat(format);
}

bool isAVCodecExperimental(const AVCodec *codec);

void applyExperimentalCodecOptions(const AVCodec *codec, AVDictionary** opts);

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
}

QDebug operator<<(QDebug, const AVRational &);

#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
QDebug operator<<(QDebug, const AVChannelLayout &);
#endif

QT_END_NAMESPACE

#endif
