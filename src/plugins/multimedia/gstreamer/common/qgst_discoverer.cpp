// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "common/qgst_discoverer_p.h"

#include <QtMultimedia/qmediaformat.h>

#include <common/qglist_helper_p.h>
#include <common/qgst_debug_p.h>
#include <common/qgstreamermetadata_p.h>
#include <common/qgstutils_p.h>
#include <uri_handler/qgstreamer_qiodevice_handler_p.h>

QT_BEGIN_NAMESPACE

namespace QGst {

namespace {

template <typename StreamInfo>
struct GstDiscovererStreamInfoList : QGstUtils::GListRangeAdaptor<StreamInfo *>
{
    using BaseType = QGstUtils::GListRangeAdaptor<StreamInfo *>;
    using BaseType::BaseType;

    ~GstDiscovererStreamInfoList() { gst_discoverer_stream_info_list_free(BaseType::head); }
};

QGstTagListHandle duplicateTagList(const GstTagList *tagList)
{
    if (!tagList)
        return {};
    return QGstTagListHandle{
        gst_tag_list_copy(tagList),
        QGstTagListHandle::HasRef,
    };
}

QGstDiscovererStreamInfo parseGstDiscovererStreamInfo(GstDiscovererStreamInfo *info)
{
    QGstDiscovererStreamInfo result;

    result.streamID = QString::fromUtf8(gst_discoverer_stream_info_get_stream_id(info));
    result.tags = duplicateTagList(gst_discoverer_stream_info_get_tags(info));

    result.streamNumber = gst_discoverer_stream_info_get_stream_number(info);

    result.caps = QGstCaps{
        gst_discoverer_stream_info_get_caps(info),
        QGstCaps::HasRef,
    };

    return result;
}

template <typename T>
constexpr bool isStreamInfo = std::is_same_v<std::decay_t<T>, GstDiscovererVideoInfo>
        || std::is_same_v<std::decay_t<T>, GstDiscovererAudioInfo>
        || std::is_same_v<std::decay_t<T>, GstDiscovererSubtitleInfo>
        || std::is_same_v<std::decay_t<T>, GstDiscovererContainerInfo>;

template <typename T>
QGstDiscovererStreamInfo parseGstDiscovererStreamInfo(T *info)
{
    static_assert(isStreamInfo<T>);
    return parseGstDiscovererStreamInfo(GST_DISCOVERER_STREAM_INFO(info));
}

QGstDiscovererVideoInfo parseGstDiscovererVideoInfo(GstDiscovererVideoInfo *info)
{
    QGstDiscovererVideoInfo result;
    static_cast<QGstDiscovererStreamInfo &>(result) = parseGstDiscovererStreamInfo(info);

    result.size = QSize{
        int(gst_discoverer_video_info_get_width(info)),
        int(gst_discoverer_video_info_get_height(info)),
    };

    result.bitDepth = gst_discoverer_video_info_get_depth(info);
    result.framerate = Fraction{
        int(gst_discoverer_video_info_get_framerate_num(info)),
        int(gst_discoverer_video_info_get_framerate_denom(info)),
    };

    result.pixelAspectRatio = Fraction{
        int(gst_discoverer_video_info_get_par_num(info)),
        int(gst_discoverer_video_info_get_par_denom(info)),
    };

    result.isInterlaced = gst_discoverer_video_info_is_interlaced(info);
    result.bitrate = int(gst_discoverer_video_info_get_bitrate(info));
    result.maxBitrate = int(gst_discoverer_video_info_get_max_bitrate(info));
    result.isImage = int(gst_discoverer_video_info_is_image(info));

    return result;
}

QGstDiscovererAudioInfo parseGstDiscovererAudioInfo(GstDiscovererAudioInfo *info)
{
    QGstDiscovererAudioInfo result;
    static_cast<QGstDiscovererStreamInfo &>(result) = parseGstDiscovererStreamInfo(info);

    result.channels = int(gst_discoverer_audio_info_get_channels(info));
    result.channelMask = gst_discoverer_audio_info_get_channel_mask(info);
    result.sampleRate = gst_discoverer_audio_info_get_sample_rate(info);
    result.bitsPerSample = gst_discoverer_audio_info_get_depth(info);

    result.bitrate = int(gst_discoverer_audio_info_get_bitrate(info));
    result.maxBitrate = int(gst_discoverer_audio_info_get_max_bitrate(info));
    result.language = QGstUtils::codeToLanguage(gst_discoverer_audio_info_get_language(info));

    return result;
}

QGstDiscovererSubtitleInfo parseGstDiscovererSubtitleInfo(GstDiscovererSubtitleInfo *info)
{
    QGstDiscovererSubtitleInfo result;
    static_cast<QGstDiscovererStreamInfo &>(result) = parseGstDiscovererStreamInfo(info);
    result.language = QGstUtils::codeToLanguage(gst_discoverer_subtitle_info_get_language(info));
    return result;
}

QGstDiscovererContainerInfo parseGstDiscovererContainerInfo(GstDiscovererContainerInfo *info)
{
    QGstDiscovererContainerInfo result;
    static_cast<QGstDiscovererStreamInfo &>(result) = parseGstDiscovererStreamInfo(info);

    result.tags = duplicateTagList(gst_discoverer_container_info_get_tags(info));

    return result;
}

QGstDiscovererInfo parseGstDiscovererInfo(GstDiscovererInfo *info)
{
    using namespace QGstUtils;

    QGstDiscovererInfo result;
    result.isLive = gst_discoverer_info_get_live(info);
    result.isSeekable = gst_discoverer_info_get_seekable(info);

    GstClockTime duration = gst_discoverer_info_get_duration(info);
    if (duration != GST_CLOCK_TIME_NONE)
        result.duration = std::chrono::nanoseconds{ duration };

    QGstDiscovererStreamInfoHandle streamInfo{
        gst_discoverer_info_get_stream_info(info),
        QGstDiscovererStreamInfoHandle::HasRef,
    };
    if (streamInfo && GST_IS_DISCOVERER_CONTAINER_INFO(streamInfo.get()))
        result.containerInfo =
                parseGstDiscovererContainerInfo(GST_DISCOVERER_CONTAINER_INFO(streamInfo.get()));
    result.tags = duplicateTagList(gst_discoverer_info_get_tags(info));

    GstDiscovererStreamInfoList<GstDiscovererVideoInfo> videoStreams{
        gst_discoverer_info_get_video_streams(info),
    };
    for (GstDiscovererVideoInfo *videoInfo : videoStreams) {
        QGstDiscovererVideoInfo item = parseGstDiscovererVideoInfo(videoInfo);
        if (!item.streamID.isNull()) // Ignore stream info without stream ID
            result.videoStreams.emplace_back(std::move(item));
    }
    GstDiscovererStreamInfoList<GstDiscovererAudioInfo> audioStreams{
        gst_discoverer_info_get_audio_streams(info),
    };
    for (GstDiscovererAudioInfo *audioInfo : audioStreams) {
        QGstDiscovererAudioInfo item = parseGstDiscovererAudioInfo(audioInfo);
        if (!item.streamID.isNull()) // Ignore stream info without stream ID
            result.audioStreams.emplace_back(std::move(item));
    }
    GstDiscovererStreamInfoList<GstDiscovererSubtitleInfo> subtitleStreams{
        gst_discoverer_info_get_subtitle_streams(info),
    };
    for (GstDiscovererSubtitleInfo *subtitleInfo : subtitleStreams) {
        QGstDiscovererSubtitleInfo item = parseGstDiscovererSubtitleInfo(subtitleInfo);
        if (!item.streamID.isNull()) // Ignore stream info without stream ID
            result.subtitleStreams.emplace_back(std::move(item));
    }

    GstDiscovererStreamInfoList<GstDiscovererContainerInfo> containerStreams{
        gst_discoverer_info_get_container_streams(info),
    };
    for (GstDiscovererContainerInfo *containerInfo : containerStreams)
        result.containerStreams.emplace_back(parseGstDiscovererContainerInfo(containerInfo));

    return result;
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------

static constexpr std::chrono::nanoseconds discovererTimeout = std::chrono::seconds(10);

QGstDiscoverer::QGstDiscoverer()
    : m_instance{
          gst_discoverer_new(discovererTimeout.count(), nullptr),
          QGstDiscovererHandle::HasRef,
      }
{
}

q23::expected<QGstDiscovererInfo, QUniqueGErrorHandle> QGstDiscoverer::discover(const QString &uri)
{
    return discover(uri.toUtf8().constData());
}

q23::expected<QGstDiscovererInfo, QUniqueGErrorHandle> QGstDiscoverer::discover(const QUrl &url)
{
    return discover(url.toEncoded().constData());
}

q23::expected<QGstDiscovererInfo, QUniqueGErrorHandle> QGstDiscoverer::discover(QIODevice *device)
{
    return discover(qGstRegisterQIODevice(device));
}

q23::expected<QGstDiscovererInfo, QUniqueGErrorHandle> QGstDiscoverer::discover(const char *uri)
{
    QUniqueGErrorHandle error;
    QGstDiscovererInfoHandle info{
        gst_discoverer_discover_uri(m_instance.get(), uri, &error),
        QGstDiscovererInfoHandle::HasRef,
    };

    if (error)
        return q23::unexpected{
            std::move(error),
        };

    QGstDiscovererInfo result = parseGstDiscovererInfo(info.get());
    return result;
}

//----------------------------------------------------------------------------------------------------------------------

QMediaMetaData toContainerMetadata(const QGstDiscovererInfo &info)
{
    QMediaMetaData metadata;
    using Key = QMediaMetaData::Key;
    using namespace std::chrono;

    if (info.containerInfo)
        extendMetaDataFromTagList(metadata, info.containerInfo->tags);
    else
        extendMetaDataFromTagList(metadata, info.tags);

    auto updateMetadata = [&](Key key, auto value) {
        QVariant currentValue = metadata.value(key);
        if (!currentValue.isValid() || currentValue != value)
            metadata.insert(key, value);
    };

    if (info.duration)
        updateMetadata(Key::Duration,
                       QVariant::fromValue(round<milliseconds>(*info.duration).count()));

    return metadata;
}

void addMissingKeysFromTaglist(QMediaMetaData &metadata, const QGstTagListHandle &tagList)
{
    QMediaMetaData tagMetaData = taglistToMetaData(tagList);
    for (auto element : tagMetaData.asKeyValueRange()) {
        if (metadata.keys().contains(element.first))
            continue;

        metadata.insert(element.first, element.second);
    }
}

template <typename ValueType>
static void updateMetadata(QMediaMetaData &metadata, QMediaMetaData::Key key, ValueType value)
{
    QVariant currentValue = metadata.value(key);
    if (!currentValue.isValid() || currentValue != value)
        metadata.insert(key, value);
}

QMediaMetaData toStreamMetadata(const QGstDiscovererVideoInfo &info)
{
    QMediaMetaData metadata;
    using Key = QMediaMetaData::Key;

    updateMetadata(metadata, Key::VideoBitRate, info.bitrate);

    extendMetaDataFromCaps(metadata, info.caps);
    addMissingKeysFromTaglist(metadata, info.tags);

    return metadata;
}

QMediaMetaData toStreamMetadata(const QGstDiscovererAudioInfo &info)
{
    QMediaMetaData metadata;
    using Key = QMediaMetaData::Key;

    updateMetadata(metadata, Key::AudioBitRate, info.bitrate);
    updateMetadata(metadata, Key::Language, info.language);

    extendMetaDataFromCaps(metadata, info.caps);
    addMissingKeysFromTaglist(metadata, info.tags);

    return metadata;
}

QMediaMetaData toStreamMetadata(const QGstDiscovererSubtitleInfo &info)
{
    QMediaMetaData metadata;
    using Key = QMediaMetaData::Key;

    updateMetadata(metadata, Key::Language, info.language);

    extendMetaDataFromCaps(metadata, info.caps);
    addMissingKeysFromTaglist(metadata, info.tags);

    return metadata;
}

} // namespace QGst

QT_END_NAMESPACE
