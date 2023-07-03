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
#include "private/qplatformmediaplayer_p.h"
#include "qffmpeg_p.h"
#include "qvideoframe.h"

#include <array>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

struct AVFormatContextDeleter
{
    void operator()(AVFormatContext *avFormat) const { avformat_close_input(&avFormat); }
};

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

    static QPlatformMediaPlayer::TrackType trackTypeFromMediaType(int mediaType);

    int activeTrack(QPlatformMediaPlayer::TrackType type) const;

    const QList<StreamInfo> &streamInfo(QPlatformMediaPlayer::TrackType trackType) const;

    qint64 duration() const { return m_duration; }

    const QMediaMetaData &metaData() const { return m_metaData; }

    bool isSeekable() const { return m_isSeekable; }

    QVideoFrame::RotationAngle getRotationAngle() const;

protected:
    std::optional<ContextError> recreateAVFormatContext(const QUrl &media, QIODevice *stream);

    void updateStreams();

    void updateMetaData();

    bool setActiveTrack(QPlatformMediaPlayer::TrackType type, int streamNumber);

protected:
    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> m_context;
    bool m_isSeekable = false;

    StreamIndexes m_currentAVStreamIndex = { { -1, -1, -1 } };
    StreamsMap m_streamMap;
    StreamIndexes m_requestedStreams = { { -1, -1, -1 } };
    qint64 m_duration = 0;
    QMediaMetaData m_metaData;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGMEDIADATAHOLDER_P_H
