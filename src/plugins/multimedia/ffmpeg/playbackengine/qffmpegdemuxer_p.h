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

#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackengineobject_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegpacket_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackutils_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegtime_p.h>
#include <QtMultimedia/private/qplatformmediaplayer_p.h>

#include <unordered_map>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class Demuxer : public PlaybackEngineObject
{
    Q_OBJECT
public:
    Demuxer(AVFormatContext *context, TrackPosition initialPosUs, bool seekPending,
            const LoopOffset &loopOffset, const StreamIndexes &streamIndexes, int loops);

    using RequestingSignal = void (Demuxer::*)(Packet);
    static RequestingSignal signalByTrackType(QPlatformMediaPlayer::TrackType trackType);

    void setLoops(int loopsCount);

public slots:
    void onPacketProcessed(Packet);

signals:
    void requestProcessAudioPacket(Packet);
    void requestProcessVideoPacket(Packet);
    void requestProcessSubtitlePacket(Packet);
    void firstPacketFound(Id id, TrackPosition absSeekPos);
    void packetsBuffered();

protected:
    std::chrono::milliseconds timerInterval() const override;

private:
    bool canDoNextStep() const override;

    void doNextStep() override;

    void ensureSeeked();

private:
    struct StreamData
    {
        QPlatformMediaPlayer::TrackType trackType = QPlatformMediaPlayer::TrackType::NTrackTypes;
        TrackDuration bufferedDuration = TrackDuration(0);
        qint64 bufferedSize = 0;

        TrackPosition maxSentPacketsPos = TrackPosition(0);
        TrackPosition maxProcessedPacketPos = TrackPosition(0);

        bool isDataLimitReached = false;
    };

    void updateStreamDataLimitFlag(StreamData &streamData);

private:
    AVFormatContext *m_context = nullptr;
    bool m_seeked = false;
    bool m_firstPacketFound = false;
    std::unordered_map<int, StreamData> m_streams;
    TrackPosition m_posInLoopUs = TrackPosition(0); // Position in current loop in [0, duration()]
    LoopOffset m_loopOffset;
    TrackPosition m_maxPacketsEndPos = TrackPosition(0);
    QAtomicInt m_loops = QMediaPlayer::Once;
    bool m_buffered = false;
    qsizetype m_demuxerRetryCount = 0;
    static constexpr qsizetype s_maxDemuxerRetries = 10; // Arbitrarily chosen
    static constexpr std::chrono::milliseconds s_demuxerRetryInterval = std::chrono::milliseconds(10);
};

} // namespace QFFmpeg

QT_END_NAMESPACE // QFFMPEGDEMUXER_P_H

#endif
