// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegencoderoptions_p.h"

#if QT_CONFIG(vaapi)
#include <va/va.h>
#endif

QT_BEGIN_NAMESPACE

// unfortunately there is no common way to specify options for the encoders. The code here tries to map our settings sensibly
// to options available in different encoders

// For constant quality options, we're trying to map things to approx those bit rates for 1080p@30fps (in Mbps):
//         VeryLow  Low     Normal      High    VeryHigh
// H264:   0.8M     1.5M    3.5M        6M      10M
// H265:   0.5M     1.0M    2.5M        4M      7M

[[maybe_unused]]
static int bitrateForSettings(const QMediaEncoderSettings &settings, bool hdr = false)
{
    // calculate an acceptable bitrate depending on video codec, resolution, framerate and requested quality
    // The calculations are rather heuristic here, trying to take into account how well codecs compress using
    // the tables above.

    // The table here is for 30FPS
    const double bitsPerPixel[int(QMediaFormat::VideoCodec::LastVideoCodec)+1][QMediaRecorder::VeryHighQuality+1] = {
        { 1.2, 2.25, 5, 9, 15 }, // MPEG1,
        { 0.8, 1.5, 3.5, 6, 10 }, // MPEG2
        { 0.4, 0.75, 1.75, 3, 5 }, // MPEG4
        { 0.4, 0.75, 1.75, 3, 5 }, // H264
        { 0.3, 0.5, 0.2, 2, 3 }, // H265
        { 0.4, 0.75, 1.75, 3, 5 }, // VP8
        { 0.3, 0.5, 0.2, 2, 3 }, // VP9
        { 0.2, 0.4, 0.9, 1.5, 2.5 }, // AV1
        { 0.4, 0.75, 1.75, 3, 5 }, // Theora
        { 0.8, 1.5, 3.5, 6, 10 }, // WMV
        { 16, 24, 32, 40, 48 }, // MotionJPEG
    };

    QSize s = settings.videoResolution();
    double bitrate = bitsPerPixel[int(settings.videoCodec())][settings.quality()]*s.width()*s.height();

    if (settings.videoCodec() != QMediaFormat::VideoCodec::MotionJPEG) {
        // We assume that doubling the framerate requires 1.5 times the amount of data (not twice, as intraframe
        // differences will be smaller). 4 times the frame rate uses thus 2.25 times the data, etc.
        float rateMultiplier = log2(settings.videoFrameRate()/30.);
        bitrate *= pow(1.5, rateMultiplier);
    } else {
        // MotionJPEG doesn't optimize between frames, so we have a linear dependency on framerate
        bitrate *= settings.videoFrameRate()/30.;
    }

    // HDR requires 10bits per pixel instead of 8, so apply a factor of 1.25.
    if (hdr)
        bitrate *= 1.25;
    return bitrate;
}

static void apply_openh264(const QMediaEncoderSettings &settings, AVCodecContext *codec,
                           AVDictionary **opts)
{
    if (settings.encodingMode() == QMediaRecorder::ConstantBitRateEncoding
        || settings.encodingMode() == QMediaRecorder::AverageBitRateEncoding) {
        codec->bit_rate = settings.videoBitRate();
        av_dict_set(opts, "rc_mode", "bitrate", 0);
    } else {
        av_dict_set(opts, "rc_mode", "quality", 0);
        static const int q[] = { 51, 48, 38, 25, 5 };
        codec->qmax = codec->qmin = q[settings.quality()];
    }
}

static void apply_x264(const QMediaEncoderSettings &settings, AVCodecContext *codec, AVDictionary **opts)
{
    if (settings.encodingMode() == QMediaRecorder::ConstantBitRateEncoding || settings.encodingMode() == QMediaRecorder::AverageBitRateEncoding) {
        codec->bit_rate = settings.videoBitRate();
    } else {
        const char *scales[] = {
            "29", "26", "23", "21", "19"
        };
        av_dict_set(opts, "crf", scales[settings.quality()], 0);
    }
}

static void apply_x265(const QMediaEncoderSettings &settings, AVCodecContext *codec, AVDictionary **opts)
{
    if (settings.encodingMode() == QMediaRecorder::ConstantBitRateEncoding || settings.encodingMode() == QMediaRecorder::AverageBitRateEncoding) {
        codec->bit_rate = settings.videoBitRate();
    } else {
        const char *scales[QMediaRecorder::VeryHighQuality+1] = {
            "40", "34", "28", "26", "24",
        };
        av_dict_set(opts, "crf", scales[settings.quality()], 0);
    }
}

static void apply_libvpx(const QMediaEncoderSettings &settings, AVCodecContext *codec, AVDictionary **opts)
{
    if (settings.encodingMode() == QMediaRecorder::ConstantBitRateEncoding || settings.encodingMode() == QMediaRecorder::AverageBitRateEncoding) {
        codec->bit_rate = settings.videoBitRate();
    } else {
        const char *scales[QMediaRecorder::VeryHighQuality+1] = {
            "38", "34", "31", "28", "25",
        };
        av_dict_set(opts, "crf", scales[settings.quality()], 0);
        av_dict_set(opts, "b", 0, 0);
    }
    av_dict_set(opts, "row-mt", "1", 0); // better multithreading
}

#ifdef Q_OS_DARWIN
static void apply_videotoolbox(const QMediaEncoderSettings &settings, AVCodecContext *codec, AVDictionary **opts)
{
    if (settings.encodingMode() == QMediaRecorder::ConstantBitRateEncoding || settings.encodingMode() == QMediaRecorder::AverageBitRateEncoding) {
        codec->bit_rate = settings.videoBitRate();
    } else {
        // only use quality on macOS/ARM, as FFmpeg doesn't support it on the other platforms and would throw
        // an error when initializing the codec
#if defined(Q_OS_MACOS) && defined(Q_PROCESSOR_ARM_64)
        // Videotoolbox describes quality as a number from 0 to 1, with low == 0.25, normal 0.5, high 0.75 and lossless = 1
        // ffmpeg uses a different scale going from 0 to 11800.
        // Values here are adjusted to agree approximately with the target bit rates listed above
        const int scales[] = {
            3000, 4800, 5900, 6900, 7700,
        };
        codec->global_quality = scales[settings.quality()];
        codec->flags |= AV_CODEC_FLAG_QSCALE;
#else
        codec->bit_rate = bitrateForSettings(settings);
#endif
    }

    // Videotooldox hw acceleration fails of some hardwares,
    // allow_sw makes sw encoding available if hw encoding failed.
    // Under the hood, ffmpeg sets
    // kVTVideoEncoderSpecification_EnableHardwareAcceleratedVideoEncoder instead of
    // kVTVideoEncoderSpecification_RequireHardwareAcceleratedVideoEncoder
    av_dict_set(opts, "allow_sw", "1", 0);
}
#endif

#if QT_CONFIG(vaapi)
static void apply_vaapi(const QMediaEncoderSettings &settings, AVCodecContext *codec, AVDictionary **/*opts*/)
{
    // See also vaapi_encode_init_rate_control() in libavcodec
    if (settings.encodingMode() == QMediaRecorder::ConstantBitRateEncoding) {
        codec->bit_rate = settings.videoBitRate();
        codec->rc_max_rate = settings.videoBitRate();
    } else if (settings.encodingMode() == QMediaRecorder::AverageBitRateEncoding) {
        codec->bit_rate = settings.videoBitRate();
    } else {
        const int *quality = nullptr;
        // unfortunately, all VA codecs use different quality scales :/
        switch (settings.videoCodec()) {
        case QMediaFormat::VideoCodec::MPEG2: {
            static const int q[] = { 20, 15, 10, 8, 6 };
            quality = q;
            break;
        }
        case QMediaFormat::VideoCodec::MPEG4:
        case QMediaFormat::VideoCodec::H264: {
            static const int q[] = { 29, 26, 23, 21, 19 };
            quality = q;
            break;
        }
        case QMediaFormat::VideoCodec::H265: {
            static const int q[] = { 40, 34, 28, 26, 24 };
            quality = q;
            break;
        }
        case QMediaFormat::VideoCodec::VP8: {
            static const int q[] = { 56, 48, 40, 34, 28 };
            quality = q;
            break;
        }
        case QMediaFormat::VideoCodec::VP9: {
            static const int q[] = { 124, 112, 100, 88, 76 };
            quality = q;
            break;
        }
        case QMediaFormat::VideoCodec::MotionJPEG: {
            static const int q[] = { 40, 60, 80, 90, 95 };
            quality = q;
            break;
        }
        case QMediaFormat::VideoCodec::AV1:
        case QMediaFormat::VideoCodec::Theora:
        case QMediaFormat::VideoCodec::WMV:
        default:
            break;
        }

        if (quality)
            codec->global_quality = quality[settings.quality()];
    }
}
#endif

static void apply_nvenc(const QMediaEncoderSettings &settings, AVCodecContext *codec,
                        AVDictionary **opts)
{
    switch (settings.encodingMode()) {
    case QMediaRecorder::EncodingMode::AverageBitRateEncoding:
        av_dict_set(opts, "vbr", "1", 0);
        codec->bit_rate = settings.videoBitRate();
        break;
    case QMediaRecorder::EncodingMode::ConstantBitRateEncoding:
        av_dict_set(opts, "cbr", "1", 0);
        codec->bit_rate = settings.videoBitRate();
        codec->rc_max_rate = codec->rc_min_rate = codec->bit_rate;
        break;
    case QMediaRecorder::EncodingMode::ConstantQualityEncoding: {
        static const char *q[] = { "51", "48", "35", "15", "1" };
        av_dict_set(opts, "cq", q[settings.quality()], 0);
    } break;
    default:
        break;
    }
}

#ifdef Q_OS_WINDOWS
static void apply_mf(const QMediaEncoderSettings &settings, AVCodecContext *codec, AVDictionary **opts)
{
    if (settings.encodingMode() == QMediaRecorder::ConstantBitRateEncoding || settings.encodingMode() == QMediaRecorder::AverageBitRateEncoding) {
        codec->bit_rate = settings.videoBitRate();
        av_dict_set(opts, "rate_control", "cbr", 0);
    } else {
        av_dict_set(opts, "rate_control", "quality", 0);
        const char *scales[] = {
            "25", "50", "75", "90", "100"
        };
        av_dict_set(opts, "quality", scales[settings.quality()], 0);
    }
}
#endif

#ifdef Q_OS_ANDROID
static void apply_mediacodec(const QMediaEncoderSettings &settings, AVCodecContext *codec,
                             AVDictionary **opts)
{
    codec->bit_rate = settings.videoBitRate();

    const int quality[] = { 25, 50, 75, 90, 100 };
    codec->global_quality = quality[settings.quality()];

    switch (settings.encodingMode()) {
    case QMediaRecorder::EncodingMode::AverageBitRateEncoding:
        av_dict_set(opts, "bitrate_mode", "vbr", 1);
        break;
    case QMediaRecorder::EncodingMode::ConstantBitRateEncoding:
        av_dict_set(opts, "bitrate_mode", "cbr", 1);
        break;
    case QMediaRecorder::EncodingMode::ConstantQualityEncoding:
        //        av_dict_set(opts, "bitrate_mode", "cq", 1);
        av_dict_set(opts, "bitrate_mode", "cbr", 1);
        break;
    default:
        break;
    }

    switch (settings.videoCodec()) {
    case QMediaFormat::VideoCodec::H264: {
        const char *levels[] = { "2.2", "3.2", "4.2", "5.2", "6.2" };
        av_dict_set(opts, "level", levels[settings.quality()], 1);
        codec->profile = FF_PROFILE_H264_HIGH;
        break;
    }
    case QMediaFormat::VideoCodec::H265: {
        const char *levels[] = { "h2.1", "h3.1", "h4.1", "h5.1", "h6.1" };
        av_dict_set(opts, "level", levels[settings.quality()], 1);
        codec->profile = FF_PROFILE_HEVC_MAIN;
        break;
    }
    default:
        break;
    }
}
#endif

namespace QFFmpeg {

using ApplyOptions = void (*)(const QMediaEncoderSettings &settings, AVCodecContext *codec, AVDictionary **opts);

const struct {
    const char *name;
    ApplyOptions apply;
} videoCodecOptionTable[] = { { "libx264", apply_x264 },
                              { "libx265xx", apply_x265 },
                              { "libvpx", apply_libvpx },
                              { "libvpx_vp9", apply_libvpx },
                              { "libopenh264", apply_openh264 },
                              { "h264_nvenc", apply_nvenc },
                              { "hevc_nvenc", apply_nvenc },
                              { "av1_nvenc", apply_nvenc },
#ifdef Q_OS_DARWIN
                              { "h264_videotoolbox", apply_videotoolbox },
                              { "hevc_videotoolbox", apply_videotoolbox },
                              { "prores_videotoolbox", apply_videotoolbox },
                              { "vp9_videotoolbox", apply_videotoolbox },
#endif
#if QT_CONFIG(vaapi)
                              { "mpeg2_vaapi", apply_vaapi },
                              { "mjpeg_vaapi", apply_vaapi },
                              { "h264_vaapi", apply_vaapi },
                              { "hevc_vaapi", apply_vaapi },
                              { "vp8_vaapi", apply_vaapi },
                              { "vp9_vaapi", apply_vaapi },
#endif
#ifdef Q_OS_WINDOWS
                              { "hevc_mf", apply_mf },
                              { "h264_mf", apply_mf },
#endif
#ifdef Q_OS_ANDROID
                              { "hevc_mediacodec", apply_mediacodec },
                              { "h264_mediacodec", apply_mediacodec },
#endif
                              { nullptr, nullptr } };

const struct {
    const char *name;
    ApplyOptions apply;
} audioCodecOptionTable[] = {
    { nullptr, nullptr }
};

void applyVideoEncoderOptions(const QMediaEncoderSettings &settings, const QByteArray &codecName, AVCodecContext *codec, AVDictionary **opts)
{
    av_dict_set(opts, "threads", "auto", 0); // we always want automatic threading

    auto *table = videoCodecOptionTable;
    while (table->name) {
        if (codecName == table->name) {
            table->apply(settings, codec, opts);
            return;
        }

        ++table;
    }
}

void applyAudioEncoderOptions(const QMediaEncoderSettings &settings, const QByteArray &codecName, AVCodecContext *codec, AVDictionary **opts)
{
    codec->thread_count = -1; // we always want automatic threading
    if (settings.encodingMode() == QMediaRecorder::ConstantBitRateEncoding || settings.encodingMode() == QMediaRecorder::AverageBitRateEncoding)
        codec->bit_rate = settings.audioBitRate();

    auto *table = audioCodecOptionTable;
    while (table->name) {
        if (codecName == table->name) {
            table->apply(settings, codec, opts);
            return;
        }

        ++table;
    }

}

}

QT_END_NAMESPACE
