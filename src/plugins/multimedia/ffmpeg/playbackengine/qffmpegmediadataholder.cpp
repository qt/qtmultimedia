// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegmediadataholder_p.h"

#include "qffmpegmediametadata_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegioutils_p.h"
#include "qiodevice.h"
#include "qdatetime.h"
#include "qloggingcategory.h"

#include <QtMultimedia/qplaybackoptions.h>

#include <math.h>
#include <optional>

extern "C" {
#include "libavutil/display.h"
}

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcMediaDataHolder, "qt.multimedia.ffmpeg.mediadataholder")

namespace QFFmpeg {

static std::optional<TrackDuration> streamDuration(const AVStream &stream)
{
    if (stream.duration > 0)
        return toTrackDuration(AVStreamDuration(stream.duration), &stream);

    // In some cases ffmpeg reports negative duration that is definitely invalid.
    // However, the correct duration may be read from the metadata.

    if (stream.duration < 0 && stream.duration != AV_NOPTS_VALUE) {
        qCWarning(qLcMediaDataHolder) << "AVStream duration" << stream.duration
                                      << "is invalid. Taking it from the metadata";
    }

    if (const auto duration = av_dict_get(stream.metadata, "DURATION", nullptr, 0)) {
        const auto time = QTime::fromString(QString::fromUtf8(duration->value));
        return TrackDuration(qint64(1000) * time.msecsSinceStartOfDay());
    }

    return {};
}

static QTransform displayMatrixToTransform(const int32_t *displayMatrix)
{
    // displayMatrix is stored as
    //
    //  . -- X axis
    //  |
    //  |    | a b u |
    //  Y    | c d v |
    // axis  | x y w |
    //
    // where a, b, c, d, x, y are 16.16 fixed-point values,
    // and u, v, w are 30.2 point values.
    // Only a, b, c, d impacts on mirroring and rotation,
    // so it's enough to propagate them to QTransform.
    //
    // If we were interested in getting proper XY scales,
    // we would divide a,b,c,d by 2^16. The whole scale doesn't
    // impact mirroring and rotation, so we don't do so.

    auto toRotateMirrorValue = [displayMatrix](int index) {
        // toRotateScaleValue would be:
        // return displayMatrix[index] / qreal(1 << 16);
        return displayMatrix[index];
    };

    return QTransform(toRotateMirrorValue(0), toRotateMirrorValue(1),
                      toRotateMirrorValue(3), toRotateMirrorValue(4),
                      0, 0);
}

static VideoTransformation streamTransformation(const AVStream *stream)
{
    Q_ASSERT(stream);

    using SideDataSize = decltype(AVPacketSideData::size);
    constexpr SideDataSize displayMatrixSize = sizeof(int32_t) * 9;
    const AVPacketSideData *sideData = streamSideData(stream, AV_PKT_DATA_DISPLAYMATRIX);
    if (!sideData || sideData->size < displayMatrixSize)
        return {};

    const auto displayMatrix = reinterpret_cast<const int32_t *>(sideData->data);
    const QTransform transform = displayMatrixToTransform(displayMatrix);
    const VideoTransformationOpt result = qVideoTransformationFromMatrix(transform);
    if (!result) {
        qCWarning(qLcMediaDataHolder)
                << "Video stream contains malformed display matrix" << transform;
        return {};
    }
    return *result;
}

static bool colorTransferSupportsHdr(const AVStream *stream)
{
    if (!stream)
        return false;

    const AVCodecParameters *codecPar = stream->codecpar;
    if (!codecPar)
        return false;

    const QVideoFrameFormat::ColorTransfer colorTransfer = fromAvColorTransfer(codecPar->color_trc);

    // Assume that content is using HDR if the color transfer supports high
    // dynamic range. The video may still not utilize the extended range,
    // but we can't determine the actual range without decoding frames.
    return colorTransfer == QVideoFrameFormat::ColorTransfer_ST2084
            || colorTransfer == QVideoFrameFormat::ColorTransfer_STD_B67;
}

VideoTransformation MediaDataHolder::transformation() const
{
    // TODO: Add QMediaMetaData::Mirrored and take from it and QMediaMetaData::Orientation:
    // int orientation = m_metaData.value(QMediaMetaData::Orientation).toInt();
    // return static_cast<QtVideo::Rotation>(orientation);

    const int streamIndex = m_currentAVStreamIndex[QPlatformMediaPlayer::VideoStream];
    if (streamIndex < 0)
        return {};

    return streamTransformation(m_context->streams[streamIndex]);
}

AVFormatContext *MediaDataHolder::avContext()
{
    return m_context.get();
}

int MediaDataHolder::currentStreamIndex(QPlatformMediaPlayer::TrackType trackType) const
{
    return m_currentAVStreamIndex[trackType];
}

static void insertMediaData(QMediaMetaData &metaData, QPlatformMediaPlayer::TrackType trackType,
                            const AVStream *stream)
{
    Q_ASSERT(stream);
    const auto *codecPar = stream->codecpar;

    switch (trackType) {
    case QPlatformMediaPlayer::VideoStream:
        metaData.insert(QMediaMetaData::VideoBitRate, (int)codecPar->bit_rate);
        metaData.insert(QMediaMetaData::VideoCodec,
                        QVariant::fromValue(QFFmpegMediaFormatInfo::videoCodecForAVCodecId(
                                codecPar->codec_id)));
        metaData.insert(QMediaMetaData::Resolution, QSize(codecPar->width, codecPar->height));
        metaData.insert(QMediaMetaData::VideoFrameRate,
                        qreal(stream->avg_frame_rate.num) / qreal(stream->avg_frame_rate.den));
        metaData.insert(QMediaMetaData::Orientation,
                        QVariant::fromValue(streamTransformation(stream).rotation));
        metaData.insert(QMediaMetaData::HasHdrContent, colorTransferSupportsHdr(stream));
        break;
    case QPlatformMediaPlayer::AudioStream:
        metaData.insert(QMediaMetaData::AudioBitRate, (int)codecPar->bit_rate);
        metaData.insert(QMediaMetaData::AudioCodec,
                        QVariant::fromValue(QFFmpegMediaFormatInfo::audioCodecForAVCodecId(
                                codecPar->codec_id)));
        break;
    default:
        break;
    }
};

QPlatformMediaPlayer::TrackType MediaDataHolder::trackTypeFromMediaType(int mediaType)
{
    switch (mediaType) {
    case AVMEDIA_TYPE_AUDIO:
        return QPlatformMediaPlayer::AudioStream;
    case AVMEDIA_TYPE_VIDEO:
        return QPlatformMediaPlayer::VideoStream;
    case AVMEDIA_TYPE_SUBTITLE:
        return QPlatformMediaPlayer::SubtitleStream;
    default:
        return QPlatformMediaPlayer::NTrackTypes;
    }
}

namespace {
q23::expected<AVFormatContextUPtr, MediaDataHolder::ContextError>
loadMedia(const QUrl &mediaUrl, QIODevice *stream, const QPlaybackOptions &playbackOptions,
          const std::shared_ptr<ICancelToken> &cancelToken)
{
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;

    const QByteArray url = mediaUrl.toString(QUrl::PreferLocalFile).toUtf8();

    AVFormatContextUPtr context{ avformat_alloc_context() };

    if (stream) {
        if (!stream->isOpen()) {
            if (!stream->open(QIODevice::ReadOnly))
                return q23::unexpected{
                    MediaDataHolder::ContextError{
                            QMediaPlayer::ResourceError,
                            QLatin1String("Could not open source device."),
                    },
                };
        }

        auto seek = &seekQIODevice;

        if (!stream->isSequential()) {
            stream->seek(0);
        } else {
            context->ctx_flags |= AVFMTCTX_UNSEEKABLE;
            seek = nullptr;
        }

        constexpr int bufferSize = 32768;
        unsigned char *buffer = (unsigned char *)av_malloc(bufferSize);
        context->pb = avio_alloc_context(buffer, bufferSize, false, stream, &readQIODevice, nullptr,
                                         seek);
    }

    AVDictionaryHolder dict;
    {
        const milliseconds timeout = playbackOptions.networkTimeout();
        av_dict_set_int(dict, "timeout", duration_cast<microseconds>(timeout).count(), 0);
        qCDebug(qLcMediaDataHolder) << "Using custom network timeout:" << timeout;
    }

    {
        const int probeSize = playbackOptions.probeSize();
        if (probeSize != -1) {
            constexpr int minProbeSizeFFmpeg = 32;
            if (probeSize >= minProbeSizeFFmpeg) {
                av_dict_set_int(dict, "probesize", probeSize, 0);
                qCDebug(qLcMediaDataHolder) << "Using custom probesize" << probeSize;
            }
            else
                qCWarning(qLcMediaDataHolder) << "Invalid probe size, using default";
        }
    }

    const QByteArray protocolWhitelist = qgetenv("QT_FFMPEG_PROTOCOL_WHITELIST");
    if (!protocolWhitelist.isNull())
        av_dict_set(dict, "protocol_whitelist", protocolWhitelist.data(), 0);

    if (playbackOptions.playbackIntent() == QPlaybackOptions::PlaybackIntent::LowLatencyStreaming) {
        av_dict_set(dict, "fflags", "nobuffer", 0);
        av_dict_set_int(dict, "flush_packets", 1, 0);
        qCDebug(qLcMediaDataHolder) << "Enabled low latency streaming";
    }

    context->interrupt_callback.opaque = cancelToken.get();
    context->interrupt_callback.callback = [](void *opaque) {
        const auto *cancelToken = static_cast<const ICancelToken *>(opaque);
        if (cancelToken && cancelToken->isCancelled())
            return 1;
        return 0;
    };

    int ret = 0;
    {
        AVFormatContext *contextRaw = context.release();
        ret = avformat_open_input(&contextRaw, url.constData(), nullptr, dict);
        context.reset(contextRaw);
    }

    if (ret < 0) {
        auto code = QMediaPlayer::ResourceError;
        if (ret == AVERROR(EACCES))
            code = QMediaPlayer::AccessDeniedError;
        else if (ret == AVERROR(EINVAL) || ret == AVERROR_INVALIDDATA)
            code = QMediaPlayer::FormatError;

        qCWarning(qLcMediaDataHolder)
                << "Could not open media. FFmpeg error description:" << AVError(ret);

        return q23::unexpected{
            MediaDataHolder::ContextError{ code, QMediaPlayer::tr("Could not open file") },
        };
    }

    ret = avformat_find_stream_info(context.get(), nullptr);
    if (ret < 0) {
        return q23::unexpected{
            MediaDataHolder::ContextError{
                    QMediaPlayer::FormatError,
                    QMediaPlayer::tr("Could not find stream information for media file") },
        };
    }

    if (qLcMediaDataHolder().isInfoEnabled())
        av_dump_format(context.get(), 0, url.constData(), 0);


    return context;
}

} // namespace

MediaDataHolder::Maybe MediaDataHolder::create(const QUrl &url, QIODevice *stream,
                                               const QPlaybackOptions &options,
                                               const std::shared_ptr<ICancelToken> &cancelToken)
{
    q23::expected context = loadMedia(url, stream, options, cancelToken);
    if (context) {
        // MediaDataHolder is wrapped in a shared pointer to interop with signal/slot mechanism
        return std::make_shared<MediaDataHolder>(
                MediaDataHolder{ std::move(context.value()), cancelToken });
    }
    return q23::unexpected{ context.error() };
}

MediaDataHolder::MediaDataHolder(AVFormatContextUPtr context,
                                 const std::shared_ptr<ICancelToken> &cancelToken)
    : m_cancelToken{ cancelToken }
{
    Q_ASSERT(context);

    m_context = std::move(context);
    m_isSeekable = !(m_context->ctx_flags & AVFMTCTX_UNSEEKABLE);

    for (unsigned int i = 0; i < m_context->nb_streams; ++i) {

        const auto *stream = m_context->streams[i];
        const auto trackType = trackTypeFromMediaType(stream->codecpar->codec_type);

        if (trackType == QPlatformMediaPlayer::NTrackTypes)
            continue;

        if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC)
            continue; // Ignore attached picture streams because we treat them as metadata

        if (stream->time_base.num <= 0 || stream->time_base.den <= 0) {
            // An invalid stream timebase is not expected to be given by FFmpeg
            qCWarning(qLcMediaDataHolder) << "A stream for the track type" << trackType
                                          << "has an invalid timebase:" << stream->time_base;
            continue;
        }

        auto metaData = QFFmpegMetaData::fromAVMetaData(stream->metadata);
        const bool isDefault = stream->disposition & AV_DISPOSITION_DEFAULT;

        if (trackType != QPlatformMediaPlayer::SubtitleStream) {
            insertMediaData(metaData, trackType, stream);

            if (isDefault && m_requestedStreams[trackType] < 0)
                m_requestedStreams[trackType] = m_streamMap[trackType].size();
        }

        if (auto duration = streamDuration(*stream)) {
            m_duration = qMax(m_duration, *duration);
            metaData.insert(QMediaMetaData::Duration, toUserDuration(*duration).get());
        }

        m_streamMap[trackType].append({ (int)i, isDefault, metaData });
    }

    // With some media files, streams may be lacking duration info. Let's
    // get it from ffmpeg's duration estimation instead.
    if (m_duration == TrackDuration(0) && m_context->duration > 0ll) {
        m_duration = toTrackDuration(AVContextDuration(m_context->duration));
    }

    for (auto trackType :
         { QPlatformMediaPlayer::VideoStream, QPlatformMediaPlayer::AudioStream }) {
        auto &requestedStream = m_requestedStreams[trackType];
        auto &streamMap = m_streamMap[trackType];

        if (requestedStream < 0 && !streamMap.empty())
            requestedStream = 0;

        if (requestedStream >= 0)
            m_currentAVStreamIndex[trackType] = streamMap[requestedStream].avStreamIndex;
    }

    updateMetaData();
}

namespace {

/*!
    \internal

    Attempt to find an attached picture from the context's streams.
    This will find ID3v2 pictures on audio files, and also pictures
    attached to videos.
 */
QImage getAttachedPicture(const AVFormatContext *context)
{
    if (!context)
        return {};

    for (unsigned int i = 0; i < context->nb_streams; ++i) {
        const AVStream* stream = context->streams[i];
        if (!stream || !(stream->disposition & AV_DISPOSITION_ATTACHED_PIC))
            continue;

        const AVPacket *compressedImage = &stream->attached_pic;
        if (!compressedImage || !compressedImage->data || compressedImage->size <= 0)
            continue;

        // Feed raw compressed data to QImage::fromData, which will decompress it
        // if it is a recognized format.
        QImage image = QImage::fromData({ compressedImage->data, compressedImage->size });
        if (!image.isNull())
            return image;
    }

    return {};
}

} // namespace

void MediaDataHolder::updateMetaData()
{
    m_metaData = {};

    if (!m_context)
        return;

    m_metaData = QFFmpegMetaData::fromAVMetaData(m_context->metadata);
    m_metaData.insert(QMediaMetaData::FileFormat,
                      QVariant::fromValue(QFFmpegMediaFormatInfo::fileFormatForAVInputFormat(
                              *m_context->iformat)));
    m_metaData.insert(QMediaMetaData::Duration, toUserDuration(m_duration).get());

    if (!m_cachedThumbnail.has_value())
        m_cachedThumbnail = getAttachedPicture(m_context.get());

    if (!m_cachedThumbnail->isNull())
        m_metaData.insert(QMediaMetaData::ThumbnailImage, m_cachedThumbnail.value());

    for (auto trackType :
         { QPlatformMediaPlayer::AudioStream, QPlatformMediaPlayer::VideoStream }) {
        const auto streamIndex = m_currentAVStreamIndex[trackType];
        if (streamIndex >= 0)
            insertMediaData(m_metaData, trackType, m_context->streams[streamIndex]);
    }
}

bool MediaDataHolder::setActiveTrack(QPlatformMediaPlayer::TrackType type, int streamNumber)
{
    if (!m_context)
        return false;

    if (streamNumber < 0 || streamNumber >= m_streamMap[type].size())
        streamNumber = -1;
    if (m_requestedStreams[type] == streamNumber)
        return false;
    m_requestedStreams[type] = streamNumber;
    const int avStreamIndex = m_streamMap[type].value(streamNumber).avStreamIndex;

    const int oldIndex = m_currentAVStreamIndex[type];
    qCDebug(qLcMediaDataHolder) << ">>>>> change track" << type << "from" << oldIndex << "to"
                                << avStreamIndex;

    // TODO: maybe add additional verifications
    m_currentAVStreamIndex[type] = avStreamIndex;

    updateMetaData();

    return true;
}

int MediaDataHolder::activeTrack(QPlatformMediaPlayer::TrackType type) const
{
    return type < QPlatformMediaPlayer::NTrackTypes ? m_requestedStreams[type] : -1;
}

const QList<MediaDataHolder::StreamInfo> &MediaDataHolder::streamInfo(
        QPlatformMediaPlayer::TrackType trackType) const
{
    Q_ASSERT(trackType < QPlatformMediaPlayer::NTrackTypes);

    return m_streamMap[trackType];
}

} // namespace QFFmpeg

QT_END_NAMESPACE
