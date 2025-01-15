// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegcodecstorage_p.h"
#include "qaudioformat.h"
#include "qimagewriter.h"

QT_BEGIN_NAMESPACE

static constexpr struct {
    AVCodecID id;
    QMediaFormat::VideoCodec codec;
} s_videoCodecMap [] = {
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
    for (const auto &c : s_videoCodecMap) {
        if (c.codec == codec)
            return c.id;
    }
    return AV_CODEC_ID_NONE;
}

static constexpr struct {
    AVCodecID id;
    QMediaFormat::AudioCodec codec;
} s_audioCodecMap [] = {
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
    for (const auto &c : s_audioCodecMap) {
        if (c.codec == codec)
            return c.id;
    }
    return AV_CODEC_ID_NONE;
}

// mimetypes are mostly copied from qmediaformat.cpp. Unfortunately, FFmpeg uses
// in some cases slightly different mimetypes
static constexpr struct
{
    QMediaFormat::FileFormat fileFormat;
    const char *mimeType;
    const char *name; // disambiguate if we have several muxers/demuxers
} s_mimeMap[] = {
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
    { QMediaFormat::Wave, "audio/x-wav", nullptr }
};

template <typename AVFormat>
static QMediaFormat::FileFormat formatForAVFormat(AVFormat *format)
{
    if (!format->mime_type || !*format->mime_type)
        return QMediaFormat::UnspecifiedFormat;

    for (const auto &m : s_mimeMap) {
        if (m.mimeType && !strcmp(m.mimeType, format->mime_type)) {
            // check if the name matches. This is used to disambiguate where FFmpeg provides
            // multiple muxers or demuxers
            if (!m.name || !strcmp(m.name, format->name))
                return m.fileFormat;
        }
    }
    return QMediaFormat::UnspecifiedFormat;
}

static const AVOutputFormat *avFormatForFormat(QMediaFormat::FileFormat format)
{
    if (format == QMediaFormat::QuickTime || format == QMediaFormat::Mpeg4Audio)
        format = QMediaFormat::MPEG4;
    if (format == QMediaFormat::WMA)
        format = QMediaFormat::WMV;

    for (const auto &m : s_mimeMap) {
        if (m.fileFormat == format)
            return av_guess_format(m.name, nullptr, m.mimeType);
    }

    return nullptr;
}

QFFmpegMediaFormatInfo::QFFmpegMediaFormatInfo()
{
    using VideoCodec = QMediaFormat::VideoCodec;
    using AudioCodec = QMediaFormat::AudioCodec;

    QList<AudioCodec> audioEncoders; // All audio encoders that Qt support
    QList<AudioCodec> extraAudioDecoders; // All audio decoders that do not support encoding
    QList<VideoCodec> videoEncoders; // All video encoders that Qt support
    QList<VideoCodec> extraVideoDecoders; // All video decoders that do not support encoding

    // Sort all FFmpeg's codecs into the buckets
    const AVCodecDescriptor *descriptor = nullptr;
    while ((descriptor = avcodec_descriptor_next(descriptor))) {

        const bool canEncode{ QFFmpeg::findAVEncoder(descriptor->id).has_value() };
        const bool canDecode{ QFFmpeg::findAVDecoder(descriptor->id).has_value() };

        const VideoCodec videoCodec = videoCodecForAVCodecId(descriptor->id);
        const AudioCodec audioCodec = audioCodecForAVCodecId(descriptor->id);

        if (descriptor->type == AVMEDIA_TYPE_VIDEO && videoCodec != VideoCodec::Unspecified) {
            if (canEncode) {
                if (!videoEncoders.contains(videoCodec))
                    videoEncoders.append(videoCodec);
            } else if (canDecode) {
                if (!extraVideoDecoders.contains(videoCodec))
                    extraVideoDecoders.append(videoCodec);
            }
        } else if (descriptor->type == AVMEDIA_TYPE_AUDIO && audioCodec != AudioCodec::Unspecified) {
            if (canEncode) {
                if (!audioEncoders.contains(audioCodec))
                    audioEncoders.append(audioCodec);
            } else if (canDecode) {
                if (!extraAudioDecoders.contains(audioCodec))
                    extraAudioDecoders.append(audioCodec);
            }
        }
    }

    // Update 'encoders' list with muxer/encoder combinations that Qt supports
    void *opaque = nullptr;
    const AVOutputFormat *outputFormat = nullptr;
    while ((outputFormat = av_muxer_iterate(&opaque))) {
        QMediaFormat::FileFormat mediaFormat = formatForAVFormat(outputFormat);
        if (mediaFormat == QMediaFormat::UnspecifiedFormat)
            continue;

        CodecMap encoder;
        encoder.format = mediaFormat;

        for (AudioCodec codec : audioEncoders) {
            const AVCodecID id = codecId(codec);
            // Only add the codec if it can be used with this container. A negative
            // result means that the codec may work, but information is unavailable
            const int result = avformat_query_codec(outputFormat, id, FF_COMPLIANCE_NORMAL);
            if (result == 1 || (result < 0 && id == outputFormat->audio_codec)) {
                // add codec for container
                encoder.audio.append(codec);
            }
        }

        for (VideoCodec codec : videoEncoders) {
            const AVCodecID id = codecId(codec);
            // Only add the codec if it can be used with this container. A negative
            // result means that the codec may work, but information is unavailable
            const int result = avformat_query_codec(outputFormat, id, FF_COMPLIANCE_NORMAL);
            if (result == 1 || (result < 0 && id == outputFormat->video_codec)) {
                // add codec for container
                encoder.video.append(codec);
            }
        }

        // If no encoders support either audio or video, we skip this format.
        if (encoder.audio.isEmpty() && encoder.video.isEmpty())
            continue;

        // Handle special cases
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
            if (!encoder.audio.contains(AudioCodec::Wave))
                continue;
            encoder.audio = { AudioCodec::Wave };
            break;
        default:
            break;
        }

        encoders.append(encoder);
    }

    // FFmpeg doesn't allow querying supported codecs for demuxers.
    // We take a simple approximation stating that we can decode what we
    // can encode. That's a safe subset.
    decoders = encoders;

#ifdef Q_OS_WINDOWS
    // MediaFoundation HVEC encoder fails when processing frames
    for (auto &encoder : encoders) {
        auto h265index = encoder.video.indexOf(VideoCodec::H265);
        if (h265index >= 0)
            encoder.video.removeAt(h265index);
    }
#endif

    // FFmpeg's Matroska muxer does not work with H264 video codec
    for (auto &encoder : encoders) {
        if (encoder.format == QMediaFormat::Matroska) {
            encoder.video.removeAll(VideoCodec::H264);

            // And on macOS, also not with H265
#ifdef Q_OS_MACOS
            encoder.video.removeAll(VideoCodec::H265);
#endif
        }
    }

    // FFmpeg can currently only decode WMA and WMV, not encode
    if (extraAudioDecoders.contains(AudioCodec::WMA)) {
        decoders[QMediaFormat::WMA].audio.append(AudioCodec::WMA);
        decoders[QMediaFormat::WMV].audio.append(AudioCodec::WMA);
    }

    if (extraVideoDecoders.contains(VideoCodec::WMV)) {
        decoders[QMediaFormat::WMV].video.append(VideoCodec::WMV);
    }

    // Add image formats we support. We currently simply use Qt's built-in image write
    // to save images. That doesn't give us HDR support or support for larger bit depths,
    // but most cameras can currently not generate those anyway.
    const QList<QByteArray> imgFormats = QImageWriter::supportedImageFormats();
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
    for (const auto &c : s_audioCodecMap) {
        if (c.id == id)
            return c.codec;
    }
    return QMediaFormat::AudioCodec::Unspecified;
}

QMediaFormat::VideoCodec QFFmpegMediaFormatInfo::videoCodecForAVCodecId(AVCodecID id)
{
    for (const auto &c : s_videoCodecMap) {
        if (c.id == id)
            return c.codec;
    }
    return QMediaFormat::VideoCodec::Unspecified;
}

QMediaFormat::FileFormat
QFFmpegMediaFormatInfo::fileFormatForAVInputFormat(const AVInputFormat &format)
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

    if (!format.name)
        return QMediaFormat::UnspecifiedFormat;

    auto *m = map;
    while (m->fileFormat != QMediaFormat::UnspecifiedFormat) {
        if (!strncmp(m->name, format.name, strlen(m->name)))
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

QAudioFormat
QFFmpegMediaFormatInfo::audioFormatFromCodecParameters(const AVCodecParameters &codecpar)
{
    QAudioFormat format;
    format.setSampleFormat(sampleFormat(AVSampleFormat(codecpar.format)));
    format.setSampleRate(codecpar.sample_rate);
#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
    uint64_t channelLayout = 0;
    if (codecpar.ch_layout.order == AV_CHANNEL_ORDER_NATIVE)
        channelLayout = codecpar.ch_layout.u.mask;
    else
        channelLayout = avChannelLayout(QAudioFormat::defaultChannelConfigForChannelCount(codecpar.ch_layout.nb_channels));
#else
    uint64_t channelLayout = codecpar.channel_layout;
    if (!channelLayout)
        channelLayout = avChannelLayout(QAudioFormat::defaultChannelConfigForChannelCount(codecpar.channels));
#endif
    format.setChannelConfig(channelConfigForAVLayout(channelLayout));
    return format;
}

QT_END_NAMESPACE
