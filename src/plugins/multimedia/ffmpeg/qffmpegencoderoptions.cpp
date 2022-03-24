/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qffmpegencoderoptions_p.h"

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
            "24", "26", "28", "34", "40",
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
            "18", "20", "31", "28", "34",
        };
        av_dict_set(opts, "crf", scales[settings.quality()], 0);
        av_dict_set(opts, "b", 0, 0);
    }
    av_dict_set(opts, "row-mt", "1", 0); // better multithreading
}

static void apply_videotoolbox(const QMediaEncoderSettings &settings, AVCodecContext *codec, AVDictionary **)
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
        int scales[] = {
            3000, 4800, 5900, 6900, 7700,
        };
        codec->global_quality = scales[settings.quality()];
        codec->flags |= AV_CODEC_FLAG_QSCALE;
#else
        codec->bit_rate = bitrateForSettings(settings);
#endif
    }
}



namespace QFFmpeg {

using ApplyOptions = void (*)(const QMediaEncoderSettings &settings, AVCodecContext *codec, AVDictionary **opts);

const struct {
    const char *name;
    ApplyOptions apply;
} videoCodecOptionTable[] = {
    { "libx264", apply_x264 },
    { "libx265xx", apply_x265 },
    { "libvpx", apply_libvpx },
    { "libvpx_vp9", apply_libvpx },
    { "h264_videotoolbox", apply_videotoolbox },
    { "hevc_videotoolbox", apply_videotoolbox },
    { "prores_videotoolbox", apply_videotoolbox },
    { "vp9_videotoolbox", apply_videotoolbox },
    { nullptr, nullptr }
};

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
