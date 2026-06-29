// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediaformatinfo_p.h"

#include "qffmpegcodecstorage_p.h"
#include "qffmpeg_ranges_p.h"

#include <QtCore/qvarlengtharray.h>
#include <QtCore/private/qflatmap_p.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtGui/qimagewriter.h>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals;

namespace {

template <size_t MapSize, typename Key, typename Value>
auto makeReverseLookupTable(std::initializer_list<std::pair<const Key, Value>> map)
{
    using namespace QtMultimediaPrivate;

    auto keyValuePairs = map | views::transform([](const auto &pair) {
        return std::pair<const Value, Key>(pair.second, pair.first);
    }) | ranges::to<std::vector<std::pair<const Value, Key>>>();

    return keyValuePairs | ranges::to<QVarLengthFlatMap<Value, Key, MapSize>>();
}

static constexpr auto videoCodecMap =
        std::initializer_list<std::pair<const QMediaFormat::VideoCodec, AVCodecID>>{
            { QMediaFormat::VideoCodec::MPEG1, AV_CODEC_ID_MPEG1VIDEO },
            { QMediaFormat::VideoCodec::MPEG2, AV_CODEC_ID_MPEG2VIDEO },
            { QMediaFormat::VideoCodec::MPEG4, AV_CODEC_ID_MPEG4 },
            { QMediaFormat::VideoCodec::H264, AV_CODEC_ID_H264 },
            { QMediaFormat::VideoCodec::H265, AV_CODEC_ID_HEVC },
            { QMediaFormat::VideoCodec::VP8, AV_CODEC_ID_VP8 },
            { QMediaFormat::VideoCodec::VP9, AV_CODEC_ID_VP9 },
            { QMediaFormat::VideoCodec::AV1, AV_CODEC_ID_AV1 },
            { QMediaFormat::VideoCodec::Theora, AV_CODEC_ID_THEORA },
            { QMediaFormat::VideoCodec::WMV, AV_CODEC_ID_WMV3 },
            { QMediaFormat::VideoCodec::MotionJPEG, AV_CODEC_ID_MJPEG },
        };

} // namespace

AVCodecID codecId(QMediaFormat::VideoCodec codec)
{
    static const QVarLengthFlatMap<QMediaFormat::VideoCodec, AVCodecID, videoCodecMap.size()> map(
            videoCodecMap.begin(), videoCodecMap.end());
    const auto it = map.find(codec);
    return it != map.end() ? it->second : AV_CODEC_ID_NONE;
}

QMediaFormat::VideoCodec videoCodecFromId(AVCodecID id)
{
    using namespace QtMultimediaPrivate;

    static const auto map = makeReverseLookupTable<videoCodecMap.size()>(videoCodecMap);

    const auto it = map.find(id);
    return it != map.end() ? it->second : QMediaFormat::VideoCodec::Unspecified;
}

static constexpr auto audioCodecMap =
        std::initializer_list<std::pair<const QMediaFormat::AudioCodec, AVCodecID>>{
            { QMediaFormat::AudioCodec::MP3, AV_CODEC_ID_MP3 },
            { QMediaFormat::AudioCodec::AAC, AV_CODEC_ID_AAC },
            { QMediaFormat::AudioCodec::AC3, AV_CODEC_ID_AC3 },
            { QMediaFormat::AudioCodec::EAC3, AV_CODEC_ID_EAC3 },
            { QMediaFormat::AudioCodec::FLAC, AV_CODEC_ID_FLAC },
            { QMediaFormat::AudioCodec::DolbyTrueHD, AV_CODEC_ID_TRUEHD },
            { QMediaFormat::AudioCodec::Opus, AV_CODEC_ID_OPUS },
            { QMediaFormat::AudioCodec::Vorbis, AV_CODEC_ID_VORBIS },
            { QMediaFormat::AudioCodec::Wave, AV_CODEC_ID_PCM_S16LE },
            { QMediaFormat::AudioCodec::WMA, AV_CODEC_ID_WMAPRO },
            { QMediaFormat::AudioCodec::ALAC, AV_CODEC_ID_ALAC },
        };

AVCodecID codecId(QMediaFormat::AudioCodec codec)
{
    static const QVarLengthFlatMap<QMediaFormat::AudioCodec, AVCodecID, audioCodecMap.size()> map(
            audioCodecMap.begin(), audioCodecMap.end());
    const auto it = map.find(codec);
    return it != map.end() ? it->second : AV_CODEC_ID_NONE;
}

QMediaFormat::AudioCodec audioCodecFromId(AVCodecID id)
{
    using namespace QtMultimediaPrivate;

    static const auto map = makeReverseLookupTable<audioCodecMap.size()>(audioCodecMap);

    const auto it = map.find(id);
    return it != map.end() ? it->second : QMediaFormat::AudioCodec::Unspecified;
}

// mimetypes are mostly copied from qmediaformat.cpp. Unfortunately, FFmpeg uses
// in some cases slightly different mimetypes
constexpr struct
{
    QMediaFormat::FileFormat fileFormat;
    const char *mimeType;
    const char *name; // disambiguate if we have several muxers/demuxers
} s_outputFormatMap[] = {
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
    { QMediaFormat::Ogg, "audio/ogg", nullptr },
};

QMediaFormat::FileFormat formatForAVFormat(const AVOutputFormat *format)
{
    if (!format->mime_type || !*format->mime_type)
        return QMediaFormat::UnspecifiedFormat;

    for (const auto &m : s_outputFormatMap) {
        if (!strcmp(m.mimeType, format->mime_type)) {
            // check if the name matches. This is used to disambiguate where FFmpeg provides
            // multiple muxers or demuxers
            if (!m.name || !strcmp(m.name, format->name))
                return m.fileFormat;
        }
    }
    return QMediaFormat::UnspecifiedFormat;
}

const AVOutputFormat *avFormatForFormat(QMediaFormat::FileFormat format)
{
    if (format == QMediaFormat::QuickTime || format == QMediaFormat::Mpeg4Audio)
        format = QMediaFormat::MPEG4;
    if (format == QMediaFormat::WMA)
        format = QMediaFormat::WMV;

    for (const auto &m : s_outputFormatMap) {
        if (m.fileFormat == format)
            return av_guess_format(m.name, nullptr, m.mimeType);
    }

    return nullptr;
}

// Seems like FFmpeg uses different names for muxers and demuxers of the same format.
// that makes it somewhat cumbersome to detect things correctly.
// The input formats have a comma separated list of short names. We check the first one of those
// as the docs specify that you only append to the list
constexpr struct
{
    QMediaFormat::FileFormat fileFormat;
    QLatin1String name;
} s_inputFormatMap[] = {
    { QMediaFormat::WMV, "asf"_L1 },
    { QMediaFormat::AVI, "avi"_L1 },
    { QMediaFormat::Matroska, "matroska"_L1 },
    { QMediaFormat::MPEG4, "mov"_L1 },
    { QMediaFormat::Ogg, "ogg"_L1 },
    { QMediaFormat::WebM, "webm"_L1 },
    // Audio Formats
    // Mpeg4Audio is the same as MP4 without the video codecs
    { QMediaFormat::AAC, "aac"_L1 },
    // WMA is the same as WMV
    { QMediaFormat::FLAC, "flac"_L1 },
    { QMediaFormat::MP3, "mp3"_L1 },
    { QMediaFormat::Wave, "wav"_L1 },
};

QMediaFormat::FileFormat formatForAVInputFormat(const AVInputFormat &format)
{
    if (!format.name)
        return QMediaFormat::UnspecifiedFormat;

    const QLatin1String formatName(format.name);
    for (const auto &m : s_inputFormatMap) {
        if (formatName.startsWith(m.name))
            return m.fileFormat;
    }

    return QMediaFormat::UnspecifiedFormat;
}

enum class CodecType : uint8_t {
    Audio,
    Video,
};

template <CodecType Type>
bool codecSupportsFormat(const AVOutputFormat *format, AVCodecID codecId)
{
    const int result = avformat_query_codec(format, codecId, FF_COMPLIANCE_NORMAL);
    switch (result) {
    case 1:
        return true;
    case AVERROR_PATCHWELCOME:
    case 0:
        return false;
    default:
        break;
    }

    if (result < 0) {
        // A negative result means that the codec may work, but information is unavailable
        switch (Type) {
        case CodecType::Audio:
            return codecId == format->audio_codec;
        case CodecType::Video:
            return codecId == format->video_codec;
        }
    }

    return false;
}

QFFmpegMediaFormatInfo::QFFmpegMediaFormatInfo()
{
    namespace ranges = QtMultimediaPrivate::ranges;
    namespace views = QtMultimediaPrivate::views;

    using VideoCodec = QMediaFormat::VideoCodec;
    using AudioCodec = QMediaFormat::AudioCodec;

    QList<AudioCodec> audioEncoders; // All audio encoders that Qt support
    QList<AudioCodec> extraAudioDecoders; // All audio decoders that do not support encoding
    QList<VideoCodec> videoEncoders; // All video encoders that Qt support
    QList<VideoCodec> extraVideoDecoders; // All video decoders that do not support encoding
    using Descriptors =
            QFFmpeg::FFmpegValueIteratorRange<const AVCodecDescriptor *, avcodec_descriptor_next>;

    // Sort all FFmpeg's codecs into the buckets
    for (const AVCodecDescriptor *descriptor : Descriptors{}) {

        const bool canEncode{ QFFmpeg::findAVEncoder(descriptor->id).has_value() };
        const bool canDecode{ QFFmpeg::findAVDecoder(descriptor->id).has_value() };
        const bool isAVCodec = canEncode || canDecode;

        if (!isAVCodec)
            continue; // subtitle codec

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
    using MuxerRange = QFFmpeg::FFmpegOpaqueIteratorRange<const AVOutputFormat *, av_muxer_iterate>;
    for (const AVOutputFormat *outputFormat : MuxerRange()) {
        QMediaFormat::FileFormat mediaFormat = formatForAVFormat(outputFormat);
        if (mediaFormat == QMediaFormat::UnspecifiedFormat)
            continue;

        CodecMap encoder;
        encoder.format = mediaFormat;

        auto supportedAudioCodecs = audioEncoders | views::filter([&](AudioCodec codec) {
            return codecSupportsFormat<CodecType::Audio>(outputFormat, codecId(codec));
        });
        encoder.audio = supportedAudioCodecs | ranges::to<QList<AudioCodec>>();

        auto supportedVideoCodecs = videoEncoders | views::filter([&](VideoCodec codec) {
            return codecSupportsFormat<CodecType::Video>(outputFormat, codecId(codec));
        });
        encoder.video = supportedVideoCodecs | ranges::to<QList<VideoCodec>>();

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
    auto findDecoder = [&](QMediaFormat::FileFormat fmt) -> CodecMap * {
        auto it = ranges::find_if(decoders, [&](const CodecMap &m) {
            return m.format == fmt;
        });
        return it != decoders.end() ? &*it : nullptr;
    };

    if (extraAudioDecoders.contains(AudioCodec::WMA)) {
        if (auto *wma = findDecoder(QMediaFormat::WMA))
            wma->audio.append(AudioCodec::WMA);
        if (auto *wmv = findDecoder(QMediaFormat::WMV))
            wmv->audio.append(AudioCodec::WMA);
    }

    if (extraVideoDecoders.contains(VideoCodec::WMV)) {
        if (auto *wmv = findDecoder(QMediaFormat::WMV))
            wmv->video.append(VideoCodec::WMV);
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

    fixupCodecMaps();
}

QFFmpegMediaFormatInfo::~QFFmpegMediaFormatInfo() = default;

QMediaFormat::AudioCodec QFFmpegMediaFormatInfo::audioCodecForAVCodecId(AVCodecID id)
{
    return audioCodecFromId(id);
}

QMediaFormat::VideoCodec QFFmpegMediaFormatInfo::videoCodecForAVCodecId(AVCodecID id)
{
    return videoCodecFromId(id);
}

QMediaFormat::FileFormat
QFFmpegMediaFormatInfo::fileFormatForAVInputFormat(const AVInputFormat &format)
{
    return formatForAVInputFormat(format);
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
