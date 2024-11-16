// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegcodec_p.h"
#include "qffmpeg_p.h"
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

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

} // namespace QFFmpeg

QT_END_NAMESPACE
