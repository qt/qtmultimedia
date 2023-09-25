// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGDEMUXER_P_H
#define QFFMPEGDEMUXER_P_H

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

#include "playbackengine/qffmpegplaybackengineobject_p.h"
#include "private/qplatformmediaplayer_p.h"
#include "playbackengine/qffmpegpacket_p.h"
#include "playbackengine/qffmpegpositionwithoffset_p.h"

#include <unordered_map>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class Demuxer : public PlaybackEngineObject
{
    Q_OBJECT
public:
    Demuxer(AVFormatContext *context, const PositionWithOffset &posWithOffset,
            const StreamIndexes &streamIndexes, int loops);

    using RequestingSignal = void (Demuxer::*)(Packet);
    static RequestingSignal signalByTrackType(QPlatformMediaPlayer::TrackType trackType);

    void setLoops(int loopsCount);

public slots:
    void onPacketProcessed(Packet);

signals:
    void requestProcessAudioPacket(Packet);
    void requestProcessVideoPacket(Packet);
    void requestProcessSubtitlePacket(Packet);
    void firstPacketFound(TimePoint tp, qint64 trackPos);

private:
    bool canDoNextStep() const override;

    void doNextStep() override;

    void ensureSeeked();

private:
    struct StreamData
    {
        QPlatformMediaPlayer::TrackType trackType = QPlatformMediaPlayer::TrackType::NTrackTypes;
        qint64 bufferingTime = 0;
        qint64 bufferingSize = 0;
    };

    AVFormatContext *m_context = nullptr;
    bool m_seeked = false;
    bool m_firstPacketFound = false;
    std::unordered_map<int, StreamData> m_streams;
    PositionWithOffset m_posWithOffset;
    qint64 m_endPts = 0;
    QAtomicInt m_loops = QMediaPlayer::Once;
};

} // namespace QFFmpeg

QT_END_NAMESPACE // QFFMPEGDEMUXER_P_H

#endif
