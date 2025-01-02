// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediaformatinfo_p.h"
#include "qaudioformat.h"
#include "qimagewriter.h"

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcMediaFormatInfo, "qt.multimedia.ffmpeg.mediaformatinfo")

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

static AVCodecID codecId(QMediaFormat::AudioCodec codec)
{
    for (const auto &c : audioCodecMap) {
        if (c.codec == codec)
            return c.id;
    }
    return AV_CODEC_ID_NONE;
}

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

template <typename AVFormat>
static QMediaFormat::FileFormat formatForAVFormat(AVFormat *format)
{

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

static const AVOutputFormat *avFormatForFormat(QMediaFormat::FileFormat format)
{
    if (format == QMediaFormat::QuickTime || format == QMediaFormat::Mpeg4Audio)
        format = QMediaFormat::MPEG4;
    if (format == QMediaFormat::WMA)
        format = QMediaFormat::WMV;

    auto *m = map;
    while (m->fileFormat != QMediaFormat::UnspecifiedFormat) {
        if (m->fileFormat == format)
            return av_guess_format(m->name, nullptr, m->mimeType);
        ++m;
    }

    return nullptr;
}


QFFmpegMediaFormatInfo::QFFmpegMediaFormatInfo()
{
    qCDebug(qLcMediaFormatInfo) << ">>>> listing codecs";

    QList<QMediaFormat::AudioCodec> audioEncoders;
    QList<QMediaFormat::AudioCodec> extraAudioDecoders;
    QList<QMediaFormat::VideoCodec> videoEncoders;
    QList<QMediaFormat::VideoCodec> extraVideoDecoders;

    const AVCodecDescriptor *descriptor = nullptr;
    while ((descriptor = avcodec_descriptor_next(descriptor))) {
        const bool canEncode = QFFmpeg::findAVEncoder(descriptor->id) != nullptr;
        const bool canDecode = QFFmpeg::findAVDecoder(descriptor->id) != nullptr;
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
        } else if (descriptor->type == AVMEDIA_TYPE_AUDIO
                   && audioCodec != QMediaFormat::AudioCodec::Unspecified) {
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
//    qCDebug(qLcMediaFormatInfo) << ">>>> Muxers";
    void *opaque = nullptr;
    const AVOutputFormat *outputFormat = nullptr;
    while ((outputFormat = av_muxer_iterate(&opaque))) {
        auto mediaFormat = formatForAVFormat(outputFormat);
        if (mediaFormat == QMediaFormat::UnspecifiedFormat)
            continue;
//        qCDebug(qLcMediaFormatInfo) << "    mux:" << outputFormat->name << outputFormat->long_name << outputFormat->mime_type << outputFormat->extensions << mediaFormat;

        CodecMap encoder;
        encoder.format = mediaFormat;

        for (auto codec : audioEncoders) {
            auto id = codecId(codec);
            // only add the codec if it can be used with this container
            if (avformat_query_codec(outputFormat, id, FF_COMPLIANCE_NORMAL) == 1) {
                // add codec for container
//                qCDebug(qLcMediaFormatInfo) << "        " << codec << Qt::hex << av_codec_get_tag(outputFormat->codec_tag, id);
                encoder.audio.append(codec);
            }
        }
        for (auto codec : videoEncoders) {
            auto id = codecId(codec);
            // only add the codec if it can be used with this container
            if (avformat_query_codec(outputFormat, id, FF_COMPLIANCE_NORMAL) == 1) {
                // add codec for container
//                qCDebug(qLcMediaFormatInfo) << "        " << codec << Qt::hex << av_codec_get_tag(outputFormat->codec_tag, id);
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

#ifdef Q_OS_WINDOWS
    // MediaFoundation HVEC encoder fails when processing frames
    for (auto &encoder : encoders) {
        auto h265index = encoder.video.indexOf(QMediaFormat::VideoCodec::H265);
        if (h265index >= 0)
            encoder.video.removeAt(h265index);
    }
#endif

//    qCDebug(qLcMediaFormatInfo) << "extraDecoders:" << extraAudioDecoders << extraVideoDecoders;
    // FFmpeg can currently only decode WMA and WMV, not encode
    if (extraAudioDecoders.contains(QMediaFormat::AudioCodec::WMA)) {
        decoders[QMediaFormat::WMA].audio.append(QMediaFormat::AudioCodec::WMA);
        decoders[QMediaFormat::WMV].audio.append(QMediaFormat::AudioCodec::WMA);
    }
    if (extraVideoDecoders.contains(QMediaFormat::VideoCodec::WMV)) {
        decoders[QMediaFormat::WMV].video.append(QMediaFormat::VideoCodec::WMV);
    }

    // Add image formats we support. We currently simply use Qt's built-in image write
    // to save images. That doesn't give us HDR support or support for larger bit depths,
    // but most cameras can currently not generate those anyway.
    const auto imgFormats = QImageWriter::supportedImageFormats();
    for (const auto &f : imgFormats) {
        if (f == "png")
            imageFormats.append(QImageCapture::PNG);
        else if (f == "jpeg")
            imageFormats.append(QImageCapture::JPEG);
        else if (f == "tiff")
            imageFormats.append(QImageCapture::Tiff);
        else if (f == "webp")
            imageFormats.append(QImageCapture::WebP);
    }

}

QFFmpegMediaFormatInfo::~QFFmpegMediaFormatInfo() = default;

QMediaFormat::AudioCodec QFFmpegMediaFormatInfo::audioCodecForAVCodecId(AVCodecID id)
{
    for (const auto &c : audioCodecMap) {
        if (c.id == id)
            return c.codec;
    }
    return QMediaFormat::AudioCodec::Unspecified;
}

QMediaFormat::VideoCodec QFFmpegMediaFormatInfo::videoCodecForAVCodecId(AVCodecID id)
{
    for (const auto &c : videoCodecMap) {
        if (c.id == id)
            return c.codec;
    }
    return QMediaFormat::VideoCodec::Unspecified;
}

QMediaFormat::FileFormat
QFFmpegMediaFormatInfo::fileFormatForAVInputFormat(const AVInputFormat *format)
{
    // Seems like FFmpeg uses different names for muxers and demuxers of the same format.
    // that makes it somewhat cumbersome to detect things correctly.
    // The input formats have a comma separated list of short names. We check the first one of those
    // as the docs specify that you only append to the list
    static const struct
    {
        QMediaFormat::FileFormat fileFormat;
        const char *name;
    } map[QMediaFormat::LastFileFormat + 1] = {
        { QMediaFormat::WMV, "asf" },
        { QMediaFormat::AVI, "avi" },
        { QMediaFormat::Matroska, "matroska" },
        { QMediaFormat::MPEG4, "mov" },
        { QMediaFormat::Ogg, "ogg" },
        { QMediaFormat::WebM, "webm" },
        // Audio Formats
        // Mpeg4Audio is the same as MP4 without the video codecs
        { QMediaFormat::AAC, "aac"},
        // WMA is the same as WMV
        { QMediaFormat::FLAC, "flac" },
        { QMediaFormat::MP3, "mp3" },
        { QMediaFormat::Wave, "wav" },
        { QMediaFormat::UnspecifiedFormat, nullptr }
    };

    if (!format->name)
        return QMediaFormat::UnspecifiedFormat;

    auto *m = map;
    while (m->fileFormat != QMediaFormat::UnspecifiedFormat) {
        if (!strncmp(m->name, format->name, strlen(m->name)))
            return m->fileFormat;
        ++m;
    }

    return QMediaFormat::UnspecifiedFormat;
}

const AVOutputFormat *
QFFmpegMediaFormatInfo::outputFormatForFileFormat(QMediaFormat::FileFormat format)
{
    return avFormatForFormat(format);
}

AVCodecID QFFmpegMediaFormatInfo::codecIdForVideoCodec(QMediaFormat::VideoCodec codec)
{
    return codecId(codec);
}

AVCodecID QFFmpegMediaFormatInfo::codecIdForAudioCodec(QMediaFormat::AudioCodec codec)
{
    return codecId(codec);
}

QAudioFormat::SampleFormat QFFmpegMediaFormatInfo::sampleFormat(AVSampleFormat format)
{
    switch (format) {
    case AV_SAMPLE_FMT_NONE:
    default:
        return QAudioFormat::Unknown;
    case AV_SAMPLE_FMT_U8:          ///< unsigned 8 bits
    case AV_SAMPLE_FMT_U8P:         ///< unsigned 8 bits: planar
            return QAudioFormat::UInt8;
    case AV_SAMPLE_FMT_S16:         ///< signed 16 bits
    case AV_SAMPLE_FMT_S16P:        ///< signed 16 bits: planar
        return QAudioFormat::Int16;
    case AV_SAMPLE_FMT_S32:         ///< signed 32 bits
    case AV_SAMPLE_FMT_S32P:        ///< signed 32 bits: planar
        return QAudioFormat::Int32;
    case AV_SAMPLE_FMT_FLT:         ///< float
    case AV_SAMPLE_FMT_FLTP:        ///< float: planar
        return QAudioFormat::Float;
    case AV_SAMPLE_FMT_DBL:         ///< double
    case AV_SAMPLE_FMT_DBLP:        ///< double: planar
    case AV_SAMPLE_FMT_S64:         ///< signed 64 bits
    case AV_SAMPLE_FMT_S64P:        ///< signed 64 bits, planar
        // let's use float
        return QAudioFormat::Float;
    }
}

AVSampleFormat QFFmpegMediaFormatInfo::avSampleFormat(QAudioFormat::SampleFormat format)
{
    switch (format) {
    case QAudioFormat::UInt8:
        return AV_SAMPLE_FMT_U8;
    case QAudioFormat::Int16:
        return AV_SAMPLE_FMT_S16;
    case QAudioFormat::Int32:
        return AV_SAMPLE_FMT_S32;
    case QAudioFormat::Float:
        return AV_SAMPLE_FMT_FLT;
    default:
        return AV_SAMPLE_FMT_NONE;
    }
}

int64_t QFFmpegMediaFormatInfo::avChannelLayout(QAudioFormat::ChannelConfig channelConfig)
{
    int64_t avChannelLayout = 0;
    if (channelConfig & (1 << QAudioFormat::FrontLeft))
        avChannelLayout |= AV_CH_FRONT_LEFT;
    if (channelConfig & (1 << QAudioFormat::FrontRight))
        avChannelLayout |= AV_CH_FRONT_RIGHT;
    if (channelConfig & (1 << QAudioFormat::FrontCenter))
        avChannelLayout |= AV_CH_FRONT_CENTER;
    if (channelConfig & (1 << QAudioFormat::LFE))
        avChannelLayout |= AV_CH_LOW_FREQUENCY;
    if (channelConfig & (1 << QAudioFormat::BackLeft))
        avChannelLayout |= AV_CH_BACK_LEFT;
    if (channelConfig & (1 << QAudioFormat::BackRight))
        avChannelLayout |= AV_CH_BACK_RIGHT;
    if (channelConfig & (1 << QAudioFormat::FrontLeftOfCenter))
        avChannelLayout |= AV_CH_FRONT_LEFT_OF_CENTER;
    if (channelConfig & (1 << QAudioFormat::FrontRightOfCenter))
        avChannelLayout |= AV_CH_FRONT_RIGHT_OF_CENTER;
    if (channelConfig & (1 << QAudioFormat::BackCenter))
        avChannelLayout |= AV_CH_BACK_CENTER;
    if (channelConfig & (1 << QAudioFormat::LFE2))
        avChannelLayout |= AV_CH_LOW_FREQUENCY_2;
    if (channelConfig & (1 << QAudioFormat::SideLeft))
        avChannelLayout |= AV_CH_SIDE_LEFT;
    if (channelConfig & (1 << QAudioFormat::SideRight))
        avChannelLayout |= AV_CH_SIDE_RIGHT;
    if (channelConfig & (1 << QAudioFormat::TopFrontLeft))
        avChannelLayout |= AV_CH_TOP_FRONT_LEFT;
    if (channelConfig & (1 << QAudioFormat::TopFrontRight))
        avChannelLayout |= AV_CH_TOP_FRONT_RIGHT;
    if (channelConfig & (1 << QAudioFormat::TopFrontCenter))
        avChannelLayout |= AV_CH_TOP_FRONT_CENTER;
    if (channelConfig & (1 << QAudioFormat::TopCenter))
        avChannelLayout |= AV_CH_TOP_CENTER;
    if (channelConfig & (1 << QAudioFormat::TopBackLeft))
        avChannelLayout |= AV_CH_TOP_BACK_LEFT;
    if (channelConfig & (1 << QAudioFormat::TopBackRight))
        avChannelLayout |= AV_CH_TOP_BACK_RIGHT;
    if (channelConfig & (1 << QAudioFormat::TopBackCenter))
        avChannelLayout |= AV_CH_TOP_BACK_CENTER;
    // The defines used below got added together for FFmpeg 4.4
#ifdef AV_CH_TOP_SIDE_LEFT
    if (channelConfig & (1 << QAudioFormat::TopSideLeft))
        avChannelLayout |= AV_CH_TOP_SIDE_LEFT;
    if (channelConfig & (1 << QAudioFormat::TopSideRight))
        avChannelLayout |= AV_CH_TOP_SIDE_RIGHT;
    if (channelConfig & (1 << QAudioFormat::BottomFrontCenter))
        avChannelLayout |= AV_CH_BOTTOM_FRONT_CENTER;
    if (channelConfig & (1 << QAudioFormat::BottomFrontLeft))
        avChannelLayout |= AV_CH_BOTTOM_FRONT_LEFT;
    if (channelConfig & (1 << QAudioFormat::BottomFrontRight))
        avChannelLayout |= AV_CH_BOTTOM_FRONT_RIGHT;
#endif
    return avChannelLayout;
}

QAudioFormat::ChannelConfig QFFmpegMediaFormatInfo::channelConfigForAVLayout(int64_t avChannelLayout)
{
    quint32 channelConfig = 0;
    if (avChannelLayout & AV_CH_FRONT_LEFT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::FrontLeft);
    if (avChannelLayout & AV_CH_FRONT_RIGHT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::FrontRight);
    if (avChannelLayout & AV_CH_FRONT_CENTER)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::FrontCenter);
    if (avChannelLayout & AV_CH_LOW_FREQUENCY)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::LFE);
    if (avChannelLayout & AV_CH_BACK_LEFT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::BackLeft);
    if (avChannelLayout & AV_CH_BACK_RIGHT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::BackRight);
    if (avChannelLayout & AV_CH_FRONT_LEFT_OF_CENTER)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::FrontLeftOfCenter);
    if (avChannelLayout & AV_CH_FRONT_RIGHT_OF_CENTER)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::FrontRightOfCenter);
    if (avChannelLayout & AV_CH_BACK_CENTER)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::BackCenter);
    if (avChannelLayout & AV_CH_LOW_FREQUENCY_2)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::LFE2);
    if (avChannelLayout & AV_CH_SIDE_LEFT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::SideLeft);
    if (avChannelLayout & AV_CH_SIDE_RIGHT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::SideRight);
    if (avChannelLayout & AV_CH_TOP_FRONT_LEFT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::TopFrontLeft);
    if (avChannelLayout & AV_CH_TOP_FRONT_RIGHT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::TopFrontRight);
    if (avChannelLayout & AV_CH_TOP_FRONT_CENTER)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::TopFrontCenter);
    if (avChannelLayout & AV_CH_TOP_CENTER)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::TopCenter);
    if (avChannelLayout & AV_CH_TOP_BACK_LEFT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::TopBackLeft);
    if (avChannelLayout & AV_CH_TOP_BACK_RIGHT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::TopBackRight);
    if (avChannelLayout & AV_CH_TOP_BACK_CENTER)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::TopBackCenter);
        // The defines used below got added together for FFmpeg 4.4
#ifdef AV_CH_TOP_SIDE_LEFT
    if (avChannelLayout & AV_CH_TOP_SIDE_LEFT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::TopSideLeft);
    if (avChannelLayout & AV_CH_TOP_SIDE_RIGHT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::TopSideRight);
    if (avChannelLayout & AV_CH_BOTTOM_FRONT_CENTER)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::BottomFrontCenter);
    if (avChannelLayout & AV_CH_BOTTOM_FRONT_LEFT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::BottomFrontLeft);
    if (avChannelLayout & AV_CH_BOTTOM_FRONT_RIGHT)
        channelConfig |= QAudioFormat::channelConfig(QAudioFormat::BottomFrontRight);
#endif
    return QAudioFormat::ChannelConfig(channelConfig);
}

QAudioFormat QFFmpegMediaFormatInfo::audioFormatFromCodecParameters(AVCodecParameters *codecpar)
{
    QAudioFormat format;
    format.setSampleFormat(sampleFormat(AVSampleFormat(codecpar->format)));
    format.setSampleRate(codecpar->sample_rate);
#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    uint64_t channelLayout = codecpar->channel_layout;
    if (!channelLayout)
        channelLayout = avChannelLayout(QAudioFormat::defaultChannelConfigForChannelCount(codecpar->channels));
#else
    uint64_t channelLayout = 0;
    if (codecpar->ch_layout.order == AV_CHANNEL_ORDER_NATIVE)
        channelLayout = codecpar->ch_layout.u.mask;
    else
        channelLayout = avChannelLayout(QAudioFormat::defaultChannelConfigForChannelCount(codecpar->ch_layout.nb_channels));
#endif
    format.setChannelConfig(channelConfigForAVLayout(channelLayout));
    return format;
}

QT_END_NAMESPACE
