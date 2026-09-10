// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGMEDIADATAHOLDER_P_H
#define QFFMPEGMEDIADATAHOLDER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qexpected_p.h>
#include <QtMultimedia/qmediametadata.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/private/qplatformmediaplayer_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegtime_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>

#include <array>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

struct ICancelToken
{
    virtual ~ICancelToken() = default;
    virtual bool isCancelled() const = 0;
};

class MediaDataHolder
{
public:
    using TrackType = QPlatformMediaPlayer::TrackType;
    struct StreamInfo
    {
        int avStreamIndex = -1;
        bool isDefault = false;
        QMediaMetaData metaData;
    };

    struct ContextError
    {
        QMediaPlayer::Error code{};
        QString description;
    };

    using StreamsMap = QPlatformMediaPlayer::TrackTypeMap<QList<StreamInfo>>;
    using StreamIndexes = QPlatformMediaPlayer::TrackTypeMap<int>;

    MediaDataHolder() = default;
    MediaDataHolder(AVDemuxerContextUPtr context, const std::shared_ptr<ICancelToken> &cancelToken);

    static std::optional<TrackType> trackTypeFromMediaType(int mediaType);

    int activeTrack(TrackType type) const;

    const QList<StreamInfo> &streamInfo(TrackType trackType) const;

    TrackDuration duration() const { return m_duration; }

    const QMediaMetaData &metaData() const { return m_metaData; }

    bool isSeekable() const { return m_isSeekable; }

    VideoTransformation transformation() const;

    AVFormatContext *avContext();

    int currentStreamIndex(TrackType trackType) const;

    using Maybe = q23::expected<std::shared_ptr<MediaDataHolder>, ContextError>;
    static Maybe create(const QUrl &url, QIODevice *stream, const QPlaybackOptions &options,
                        const std::shared_ptr<ICancelToken> &cancelToken);

    bool setActiveTrack(TrackType type, int streamNumber);

private:
    void updateMetaData();

    std::shared_ptr<ICancelToken> m_cancelToken; // NOTE: Cancel token may be accessed by
                                                 // AVFormatContext during destruction and
                                                 // must outlive the context object
    AVDemuxerContextUPtr m_context;

    bool m_isSeekable = false;

    StreamIndexes m_currentAVStreamIndex = { -1, -1, -1 };
    StreamsMap m_streamMap;
    StreamIndexes m_requestedStreams = { -1, -1, -1 };
    TrackDuration m_duration = TrackDuration(0);
    QMediaMetaData m_metaData;
    std::optional<QImage> m_cachedThumbnail;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGMEDIADATAHOLDER_P_H
