// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegcodec_p.h"
#include "qffmpeg_p.h"
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {
namespace {

#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG

Q_STATIC_LOGGING_CATEGORY(qLcFFmpegUtils, "qt.multimedia.ffmpeg.utils");

void logGetCodecConfigError(const AVCodec *codec, AVCodecConfig config, int error)
{
    qCWarning(qLcFFmpegUtils) << "Failed to retrieve config" << config << "for codec" << codec->name
                              << "with error" << error << err2str(error);
}

template <typename T>
const T *getCodecConfig(const AVCodec *codec, AVCodecConfig config)
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

const AVPixelFormat *getCodecPixelFormats(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<AVPixelFormat>(codec, AV_CODEC_CONFIG_PIX_FORMAT);
#else
    return codec->pix_fmts;
#endif
}

const AVSampleFormat *getCodecSampleFormats(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<AVSampleFormat>(codec, AV_CODEC_CONFIG_SAMPLE_FORMAT);
#else
    return codec->sample_fmts;
#endif
}

const int *getCodecSampleRates(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<int>(codec, AV_CODEC_CONFIG_SAMPLE_RATE);
#else
    return codec->supported_samplerates;
#endif
}

const ChannelLayoutT *getCodecChannelLayouts(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<AVChannelLayout>(codec, AV_CODEC_CONFIG_CHANNEL_LAYOUT);
#elif QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
    return codec->ch_layouts;
#else
    return codec->channel_layouts;
#endif
}

const AVRational *getCodecFrameRates(const AVCodec *codec)
{
#if QT_FFMPEG_HAS_AVCODEC_GET_SUPPORTED_CONFIG
    return getCodecConfig<AVRational>(codec, AV_CODEC_CONFIG_FRAME_RATE);
#else
    return codec->supported_framerates;
#endif
}
} // namespace

Codec::Codec(const AVCodec *codec) : m_codec{ codec } { }

bool Codec::isValid() const noexcept
{
    return m_codec != nullptr;
}

const AVCodec* Codec::get() const noexcept
{
    return m_codec;
}

AVCodecID Codec::id() const noexcept
{
    if (!m_codec)
        return AV_CODEC_ID_NONE;

    return m_codec->id;
}

QLatin1StringView Codec::name() const noexcept
{
    if (!m_codec)
        return {};

    return QLatin1StringView{ m_codec->name };
}

AVMediaType Codec::type() const noexcept
{
    if (!m_codec)
        return AVMEDIA_TYPE_UNKNOWN;

    return m_codec->type;
}

// See AV_CODEC_CAP_*
int Codec::capabilities() const noexcept
{
    if (!m_codec)
        return 0;

    return m_codec->capabilities;
}

bool Codec::isEncoder() const noexcept
{
    if (!m_codec)
        return false;

    return av_codec_is_encoder(m_codec) != 0;
}

bool Codec::isDecoder() const noexcept
{
    if (!m_codec)
        return false;

    return av_codec_is_decoder(m_codec) != 0;
}

bool Codec::isExperimental() const noexcept
{
    if (!m_codec)
        return false;

    return (m_codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL) != 0;
}

const AVPixelFormat *Codec::pixelFormats() const noexcept {
    if (!m_codec)
        return nullptr;

    return getCodecPixelFormats(m_codec);
}

const AVSampleFormat* Codec::sampleFormats() const noexcept
{
    if (!m_codec)
        return nullptr;

    return getCodecSampleFormats(m_codec);
}

const int* Codec::sampleRates() const noexcept
{
    if (!m_codec)
        return nullptr;

    return getCodecSampleRates(m_codec);
}

const ChannelLayoutT* Codec::channelLayouts() const noexcept
{
    if (!m_codec)
        return nullptr;

    return getCodecChannelLayouts(m_codec);
}

const AVRational* Codec::frameRates() const noexcept
{
    if (!m_codec)
        return nullptr;

    return getCodecFrameRates(m_codec);
}

const AVCodecHWConfig* Codec::hwConfig(int i) const noexcept
{
    if (!m_codec)
        return nullptr;

    return avcodec_get_hw_config(m_codec, i);
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

} // namespace QFFmpeg

QT_END_NAMESPACE
