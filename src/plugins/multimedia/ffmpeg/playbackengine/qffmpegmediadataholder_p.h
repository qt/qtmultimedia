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

#include "qmediametadata.h"
#include <QtFFmpegMediaPluginImpl/private/qffmpegtime_p.h>
#include <QtMultimedia/private/qplatformmediaplayer_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include "qvideoframe.h"
#include <QtMultimedia/private/qmultimediautils_p.h>

#include <array>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

struct ICancelToken
{
    virtual ~ICancelToken() = default;
    virtual bool isCancelled() const = 0;
};

using AVFormatContextUPtr = std::unique_ptr<AVFormatContext, AVDeleter<decltype(&avformat_close_input), &avformat_close_input>>;

class MediaDataHolder
{
public:
    struct StreamInfo
    {
        int avStreamIndex = -1;
        bool isDefault = false;
        QMediaMetaData metaData;
    };

    struct ContextError
    {
        int code = 0;
        QString description;
    };

    using StreamsMap = std::array<QList<StreamInfo>, QPlatformMediaPlayer::NTrackTypes>;
    using StreamIndexes = std::array<int, QPlatformMediaPlayer::NTrackTypes>;

    MediaDataHolder() = default;
    MediaDataHolder(AVFormatContextUPtr context, const std::shared_ptr<ICancelToken> &cancelToken);

    static QPlatformMediaPlayer::TrackType trackTypeFromMediaType(int mediaType);

    int activeTrack(QPlatformMediaPlayer::TrackType type) const;

    const QList<StreamInfo> &streamInfo(QPlatformMediaPlayer::TrackType trackType) const;

    TrackDuration duration() const { return m_duration; }

    const QMediaMetaData &metaData() const { return m_metaData; }

    bool isSeekable() const { return m_isSeekable; }

    VideoTransformation transformation() const;

    AVFormatContext *avContext();

    int currentStreamIndex(QPlatformMediaPlayer::TrackType trackType) const;

    using Maybe = QMaybe<QSharedPointer<MediaDataHolder>, ContextError>;
    static Maybe create(const QUrl &url, QIODevice *stream,
                        const std::shared_ptr<ICancelToken> &cancelToken);

    bool setActiveTrack(QPlatformMediaPlayer::TrackType type, int streamNumber);

private:
    void updateMetaData();

    std::shared_ptr<ICancelToken> m_cancelToken; // NOTE: Cancel token may be accessed by
                                                 // AVFormatContext during destruction and
                                                 // must outlive the context object
    AVFormatContextUPtr m_context;

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
