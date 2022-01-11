/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qffmpegmediaformatinfo_p.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

static struct {
    AVCodecID id;
    QMediaFormat::VideoCodec codec;
} videoCodecMap [] = {
    { AV_CODEC_ID_MPEG1VIDEO, QMediaFormat::VideoCodec::MPEG1 },
    { AV_CODEC_ID_MPEG2VIDEO, QMediaFormat::VideoCodec::MPEG2 },
    { AV_CODEC_ID_MPEG4, QMediaFormat::VideoCodec::MPEG4 },
    { AV_CODEC_ID_H264, QMediaFormat::VideoCodec::H264 },
    { AV_CODEC_ID_HEVC, QMediaFormat::VideoCodec::H265 },
    { AV_CODEC_ID_VP8, QMediaFormat::VideoCodec::VP8 },
    { AV_CODEC_ID_VP9, QMediaFormat::VideoCodec::VP9 },
    { AV_CODEC_ID_AV1, QMediaFormat::VideoCodec::AV1 },
    { AV_CODEC_ID_THEORA, QMediaFormat::VideoCodec::Theora },
    { AV_CODEC_ID_WMV3, QMediaFormat::VideoCodec::WMV },
    { AV_CODEC_ID_MJPEG, QMediaFormat::VideoCodec::MotionJPEG }
};

QMediaFormat::VideoCodec QFFmpegMediaFormatInfo::videoCodecForAVCodecId(AVCodecID id)
{
    for (const auto &c : videoCodecMap) {
        if (c.id == id)
            return c.codec;
    }
    return QMediaFormat::VideoCodec::Unspecified;
}

static AVCodecID codecId(QMediaFormat::VideoCodec codec)
{
    for (const auto &c : videoCodecMap) {
        if (c.codec == codec)
            return c.id;
    }
    return AV_CODEC_ID_NONE;
}

static struct {
    AVCodecID id;
    QMediaFormat::AudioCodec codec;
} audioCodecMap [] = {
    { AV_CODEC_ID_MP3, QMediaFormat::AudioCodec::MP3 },
    { AV_CODEC_ID_AAC, QMediaFormat::AudioCodec::AAC },
    { AV_CODEC_ID_AC3, QMediaFormat::AudioCodec::AC3 },
    { AV_CODEC_ID_EAC3, QMediaFormat::AudioCodec::EAC3 },
    { AV_CODEC_ID_FLAC, QMediaFormat::AudioCodec::FLAC },
    { AV_CODEC_ID_TRUEHD, QMediaFormat::AudioCodec::DolbyTrueHD },
    { AV_CODEC_ID_OPUS, QMediaFormat::AudioCodec::Opus },
    { AV_CODEC_ID_VORBIS, QMediaFormat::AudioCodec::Vorbis },
    { AV_CODEC_ID_PCM_S16LE, QMediaFormat::AudioCodec::Wave },
    { AV_CODEC_ID_WMAPRO, QMediaFormat::AudioCodec::WMA },
    { AV_CODEC_ID_ALAC, QMediaFormat::AudioCodec::ALAC }
};

QMediaFormat::AudioCodec QFFmpegMediaFormatInfo::audioCodecForAVCodecId(AVCodecID id)
{
    for (const auto &c : audioCodecMap) {
        if (c.id == id)
            return c.codec;
    }
    return QMediaFormat::AudioCodec::Unspecified;
}

static AVCodecID codecId(QMediaFormat::AudioCodec codec)
{
    for (const auto &c : audioCodecMap) {
        if (c.codec == codec)
            return c.id;
    }
    return AV_CODEC_ID_NONE;
}


template <typename AVFormat>
static QMediaFormat::FileFormat formatForAVFormat(AVFormat *format)
{
    // mimetypes are mostly copied from qmediaformat.cpp. Unfortunately, FFmpeg uses
    // in some cases slightly different mimetypes
    static const struct
    {
        QMediaFormat::FileFormat fileFormat;
        const char *mimeType;
        const char *name; // disambiguate if we have several muxers/demuxers
    } map[QMediaFormat::LastFileFormat + 1] = {
        { QMediaFormat::WMV, "video/x-ms-asf", "asf" },
        { QMediaFormat::AVI, "video/x-msvideo", nullptr },
        { QMediaFormat::Matroska, "video/x-matroska", nullptr },
        { QMediaFormat::MPEG4, "video/mp4", "mp4" },
        { QMediaFormat::Ogg, "video/ogg", nullptr },
        // QuickTime is the same as MP4
        { QMediaFormat::WebM, "video/webm", "webm" },
         // Audio Formats
        // Mpeg4Audio is the same as MP4 without the video codecs
        { QMediaFormat::AAC, "audio/aac", nullptr },
        // WMA is the same as WMV
        { QMediaFormat::FLAC, "audio/x-flac", nullptr },
        { QMediaFormat::MP3, "audio/mpeg", "mp3" },
        { QMediaFormat::Wave, "audio/x-wav", nullptr },
        { QMediaFormat::UnspecifiedFormat, nullptr, nullptr }
    };

    if (!format->mime_type || !*format->mime_type)
        return QMediaFormat::UnspecifiedFormat;

    auto *m = map;
    while (m->fileFormat != QMediaFormat::UnspecifiedFormat) {
        if (m->mimeType && !strcmp(m->mimeType, format->mime_type)) {
            // check if the name matches. This is used to disambiguate where FFmpeg provides
            // multiple muxers or demuxers
            if (!m->name || !strcmp(m->name, format->name))
                return m->fileFormat;
        }
        ++m;
    }

    return QMediaFormat::UnspecifiedFormat;
}


QT_BEGIN_NAMESPACE

QFFmpegMediaFormatInfo::QFFmpegMediaFormatInfo()
{
    qDebug() << ">>>> listing codecs";

    QList<QMediaFormat::AudioCodec> audioEncoders;
    QList<QMediaFormat::AudioCodec> extraAudioDecoders;
    QList<QMediaFormat::VideoCodec> videoEncoders;
    QList<QMediaFormat::VideoCodec> extraVideoDecoders;

    const AVCodecDescriptor *descriptor = nullptr;
    while ((descriptor = avcodec_descriptor_next(descriptor))) {
        bool canEncode = (avcodec_find_encoder(descriptor->id) != nullptr);
        bool canDecode = (avcodec_find_decoder(descriptor->id) != nullptr);
        auto videoCodec = videoCodecForAVCodecId(descriptor->id);
        auto audioCodec = audioCodecForAVCodecId(descriptor->id);
        if (descriptor->type == AVMEDIA_TYPE_VIDEO && videoCodec != QMediaFormat::VideoCodec::Unspecified) {
            if (canEncode) {
                if (!videoEncoders.contains(videoCodec))
                    videoEncoders.append(videoCodec);
            } else if (canDecode) {
                if (!extraVideoDecoders.contains(videoCodec))
                    extraVideoDecoders.append(videoCodec);
            }
        }

        else if (descriptor->type == AVMEDIA_TYPE_AUDIO && audioCodec != QMediaFormat::AudioCodec::Unspecified) {
            if (canEncode) {
                if (!audioEncoders.contains(audioCodec))
                    audioEncoders.append(audioCodec);
            } else if (canDecode) {
                if (!extraAudioDecoders.contains(audioCodec))
                    extraAudioDecoders.append(audioCodec);
            }
        }
    }

    // get demuxers
//    qDebug() << ">>>> Muxers";
    void *opaque = nullptr;
    const AVOutputFormat *outputFormat = nullptr;
    while ((outputFormat = av_muxer_iterate(&opaque))) {
        auto mediaFormat = formatForAVFormat(outputFormat);
        if (mediaFormat == QMediaFormat::UnspecifiedFormat)
            continue;
//        qDebug() << "    mux:" << outputFormat->name << outputFormat->long_name << outputFormat->mime_type << outputFormat->extensions << mediaFormat;

        CodecMap encoder;
        encoder.format = mediaFormat;

        for (auto codec : audioEncoders) {
            auto id = codecId(codec);
            // only add the codec if it can be used with this container
            if (avformat_query_codec(outputFormat, id, FF_COMPLIANCE_NORMAL) == 1) {
                // add codec for container
//                qDebug() << "        " << codec << Qt::hex << av_codec_get_tag(outputFormat->codec_tag, id);
                encoder.audio.append(codec);
            }
        }
        for (auto codec : videoEncoders) {
            auto id = codecId(codec);
            // only add the codec if it can be used with this container
            if (avformat_query_codec(outputFormat, id, FF_COMPLIANCE_NORMAL) == 1) {
                // add codec for container
//                qDebug() << "        " << codec << Qt::hex << av_codec_get_tag(outputFormat->codec_tag, id);
                encoder.video.append(codec);
            }
        }

        // sanity checks and handling special cases
        if (encoder.audio.isEmpty() && encoder.video.isEmpty())
            continue;
        switch (encoder.format) {
        case QMediaFormat::WMV:
            // add WMA
            encoders.append({ QMediaFormat::WMA, encoder.audio, {} });
            break;
        case QMediaFormat::MPEG4:
            // add Mpeg4Audio and QuickTime
            encoders.append({ QMediaFormat::QuickTime, encoder.audio, encoder.video });
            encoders.append({ QMediaFormat::Mpeg4Audio, encoder.audio, {} });
            break;
        case QMediaFormat::Wave:
            // FFmpeg allows other encoded formats in WAV containers, but we do not want that
            if (!encoder.audio.contains(QMediaFormat::AudioCodec::Wave))
                continue;
            encoder.audio = { QMediaFormat::AudioCodec::Wave };
            break;
        default:
            break;
        }
        encoders.append(encoder);
    }

    // FFmpeg doesn't allow querying supported codecs for decoders
    // we take a simple approximation stating that we can decode what we
    // can encode. That's a safe subset.
    decoders = encoders;

//    qDebug() << "extraDecoders:" << extraAudioDecoders << extraVideoDecoders;
    // FFmpeg can currently only decode WMA and WMV, not encode
    if (extraAudioDecoders.contains(QMediaFormat::AudioCodec::WMA)) {
        decoders[QMediaFormat::WMA].audio.append(QMediaFormat::AudioCodec::WMA);
        decoders[QMediaFormat::WMV].audio.append(QMediaFormat::AudioCodec::WMA);
    }
    if (extraVideoDecoders.contains(QMediaFormat::VideoCodec::WMV)) {
        decoders[QMediaFormat::WMV].video.append(QMediaFormat::VideoCodec::WMV);
    }
}

QFFmpegMediaFormatInfo::~QFFmpegMediaFormatInfo() = default;

QT_END_NAMESPACE
