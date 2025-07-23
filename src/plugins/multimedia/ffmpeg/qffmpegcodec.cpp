// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegcodec_p.h"
#include "qffmpeg_p.h"

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {
namespace {

template <typename T>
inline constexpr auto InvalidAvValue = T{};

template <>
inline constexpr auto InvalidAvValue<AVSampleFormat> = AV_SAMPLE_FMT_NONE;

template <>
inline constexpr auto InvalidAvValue<AVPixelFormat> = AV_PIX_FMT_NONE;

template <typename T>
QSpan<const T> makeSpan(const T *values)
{
    if (!values)
        return {};

    qsizetype size = 0;
    while (values[size] != InvalidAvValue<T>)
        ++size;

    return QSpan<const T>{ values, size };
}

#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG

Q_STATIC_LOGGING_CATEGORY(qLcFFmpegUtils, "qt.multimedia.ffmpeg.utils");

void logGetCodecConfigError(const AVCodec *codec, AVCodecConfig config, int error)
{
    qCWarning(qLcFFmpegUtils) << "Failed to retrieve config" << config << "for codec" << codec->name
                              << "with error" << error << AVError(error);
}

template <typename T>
QSpan<const T> getCodecConfig(const AVCodec *codec, AVCodecConfig config)
{
    const T *result = nullptr;
    int size = 0;
    const auto error = avcodec_get_supported_config(
            nullptr, codec, config, 0u, reinterpret_cast<const void **>(&result), &size);
    if (error != 0) {
        logGetCodecConfigError(codec, config, error);
        return {};
    }

    // Sanity check of FFmpeg's array layout. If it is not nullptr, it should end with a terminator,
    // and be non-empty. A non-null but empty config would mean that no values are accepted by the
    // codec, which does not make sense.
    Q_ASSERT(!result || (size > 0 && result[size] == InvalidAvValue<T>));

    // Returns empty span if 'result' is nullptr. This can be misleading, as it may
    // mean that 'any' value is allowed, or that the result is 'unknown'.
    return QSpan<const T>{ result, size };
}
#endif

QSpan<const AVPixelFormat> getCodecPixelFormats(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<AVPixelFormat>(codec, AV_CODEC_CONFIG_PIX_FORMAT);
#else
    return makeSpan(codec->pix_fmts);
#endif
}

QSpan<const AVSampleFormat> getCodecSampleFormats(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<AVSampleFormat>(codec, AV_CODEC_CONFIG_SAMPLE_FORMAT);
#else
    return makeSpan(codec->sample_fmts);
#endif
}

QSpan<const int> getCodecSampleRates(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<int>(codec, AV_CODEC_CONFIG_SAMPLE_RATE);
#else
    return makeSpan(codec->supported_samplerates);
#endif
}

#ifdef Q_OS_WINDOWS

auto stereoLayout() // unused function on non-Windows platforms
{
    constexpr uint64_t mask = AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT;
#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
    AVChannelLayout channelLayout{};
    av_channel_layout_from_mask(&channelLayout, mask);
    return channelLayout;
#else
    return mask;
#endif
};

#endif

QSpan<const ChannelLayoutT> getCodecChannelLayouts(const AVCodec *codec)
{
    QSpan<const ChannelLayoutT> layout;
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    layout = getCodecConfig<AVChannelLayout>(codec, AV_CODEC_CONFIG_CHANNEL_LAYOUT);
#elif QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
    layout = makeSpan(codec->ch_layouts);
#else
    layout = makeSpan(codec->channel_layouts);
#endif

#ifdef Q_OS_WINDOWS
    // The mp3_mf codec does not report any layout restrictions, but does not
    // handle more than 2 channels. We therefore make this explicit here.
    if (layout.empty() && QLatin1StringView(codec->name) == QLatin1StringView("mp3_mf")) {
        static const ChannelLayoutT defaultLayout[] = { stereoLayout() };
        layout = defaultLayout;
    }
#endif
    return layout;
}

QSpan<const AVRational> getCodecFrameRates(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<AVRational>(codec, AV_CODEC_CONFIG_FRAME_RATE);
#else
    return makeSpan(codec->supported_framerates);
#endif
}
} // namespace

Codec::Codec(const AVCodec *codec) : m_codec{ codec }
{
    Q_ASSERT(m_codec);
}

const AVCodec* Codec::get() const noexcept
{
    Q_ASSERT(m_codec);
    return m_codec;
}

AVCodecID Codec::id() const noexcept
{
    Q_ASSERT(m_codec);

    return m_codec->id;
}

QLatin1StringView Codec::name() const noexcept
{
    Q_ASSERT(m_codec);

    return QLatin1StringView{ m_codec->name };
}

AVMediaType Codec::type() const noexcept
{
    Q_ASSERT(m_codec);

    return m_codec->type;
}

// See AV_CODEC_CAP_*
int Codec::capabilities() const noexcept
{
    Q_ASSERT(m_codec);

    return m_codec->capabilities;
}

bool Codec::isEncoder() const noexcept
{
    Q_ASSERT(m_codec);

    return av_codec_is_encoder(m_codec) != 0;
}

bool Codec::isDecoder() const noexcept
{
    Q_ASSERT(m_codec);

    return av_codec_is_decoder(m_codec) != 0;
}

bool Codec::isExperimental() const noexcept
{
    Q_ASSERT(m_codec);

    return (m_codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL) != 0;
}

QSpan<const AVPixelFormat> Codec::pixelFormats() const noexcept
{
    Q_ASSERT(m_codec);

    if (m_codec->type != AVMEDIA_TYPE_VIDEO)
        return {};

    return getCodecPixelFormats(m_codec);
}

QSpan<const AVSampleFormat> Codec::sampleFormats() const noexcept
{
    Q_ASSERT(m_codec);

    if (m_codec->type != AVMEDIA_TYPE_AUDIO)
        return {};

    return getCodecSampleFormats(m_codec);
}

QSpan<const int> Codec::sampleRates() const noexcept
{
    Q_ASSERT(m_codec);

    if (m_codec->type != AVMEDIA_TYPE_AUDIO)
        return {};

    return getCodecSampleRates(m_codec);
}

QSpan<const ChannelLayoutT> Codec::channelLayouts() const noexcept
{
    Q_ASSERT(m_codec);

    if (m_codec->type != AVMEDIA_TYPE_AUDIO)
        return {};

    return getCodecChannelLayouts(m_codec);
}

QSpan<const AVRational> Codec::frameRates() const noexcept
{
    Q_ASSERT(m_codec);

    if (m_codec->type != AVMEDIA_TYPE_VIDEO)
        return {};

    return getCodecFrameRates(m_codec);
}

std::vector<const AVCodecHWConfig *> Codec::hwConfigs() const noexcept
{
    Q_ASSERT(m_codec);

    // For most codecs, hwConfig is empty so we optimize for
    // the hot path and do not preallocate/reserve any memory.
    std::vector<const AVCodecHWConfig *> configs;

    for (int index = 0; auto config = avcodec_get_hw_config(m_codec, index); ++index)
        configs.push_back(config);

    return configs;
}

CodecIterator CodecIterator::begin()
{
    CodecIterator iterator;
    iterator.m_codec = av_codec_iterate(&iterator.m_state);
    return iterator;
}

CodecIterator CodecIterator::end()
{
    return { };
}

CodecIterator &CodecIterator::operator++() noexcept
{
    Q_ASSERT(m_codec);
    m_codec = av_codec_iterate(&m_state);
    return *this;
}

Codec CodecIterator::operator*() const noexcept
{
    Q_ASSERT(m_codec); // Avoid dereferencing end() iterator
    return Codec{ m_codec };
}

bool CodecIterator::operator!=(const CodecIterator &other) const noexcept
{
    return m_codec != other.m_codec;
}

QSpan<const AVPixelFormat> makeSpan(const AVPixelFormat *values)
{
    return makeSpan<AVPixelFormat>(values);
}


} // namespace QFFmpeg

QT_END_NAMESPACE
