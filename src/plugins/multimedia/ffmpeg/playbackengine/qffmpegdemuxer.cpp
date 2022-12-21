// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegdemuxer_p.h"

QT_BEGIN_NAMESPACE

// queue up max 16M of encoded data, that should always be enough
// (it's around 2 secs of 4K HDR video, longer for almost all other formats)
static constexpr quint64 MaxQueueSize = 16 * 1024 * 1024;

namespace QFFmpeg {

Demuxer::Demuxer(AVFormatContext *context, qint64 seekPos, const StreamIndexes &streamIndexes)
    : m_context(context), m_seekPos(seekPos)
{
    Q_ASSERT(m_context);

    for (auto i = 0; i < QPlatformMediaPlayer::NTrackTypes; ++i) {
        if (streamIndexes[i] >= 0)
            m_streams[streamIndexes[i]] = { static_cast<QPlatformMediaPlayer::TrackType>(i) };
    }
}

void Demuxer::doNextStep()
{
    ensureSeeked();

    Packet packet(AVPacketUPtr{ av_packet_alloc() });
    if (av_read_frame(m_context, packet.avPacket()) < 0) {
        setAtEnd(true);
        return;
    }

    const auto streamIndex = packet.avPacket()->stream_index;

    auto it = m_streams.find(streamIndex);

    if (it != m_streams.end()) {
        it->second.dataSize += packet.avPacket()->size;
        it->second.duration += packet.avPacket()->duration;

        switch (it->second.trackType) {
        case QPlatformMediaPlayer::TrackType::VideoStream:
            emit requestProcessVideoPacket(packet);
            break;
        case QPlatformMediaPlayer::TrackType::AudioStream:
            emit requestProcessAudioPacket(packet);
            break;
        case QPlatformMediaPlayer::TrackType::SubtitleStream:
            emit requestProcessSubtitlePacket(packet);
            break;
        default:
            Q_ASSERT(!"Unknown track type");
        }
    }

    scheduleNextStep(false);
}

void Demuxer::onPacketProcessed(Packet packet)
{
    if (packet.isValid()) {
        const auto streamIndex = packet.avPacket()->stream_index;
        auto it = m_streams.find(streamIndex);

        if (it != m_streams.end()) {
            it->second.dataSize -= packet.avPacket()->size;
            it->second.duration -= packet.avPacket()->duration;

            Q_ASSERT(it->second.dataSize >= 0);
            Q_ASSERT(it->second.duration >= 0);
        }
    }

    scheduleNextStep();
}

bool Demuxer::canDoNextStep() const
{
    if (!PlaybackEngineObject::canDoNextStep() || isAtEnd() || m_streams.empty())
        return false;

    const bool hasSmallDuration =
            std::any_of(m_streams.begin(), m_streams.end(),
                        [](const auto &s) { return s.second.duration < 200; });

    if (hasSmallDuration)
        return true;

    const auto dataSize =
            std::accumulate(m_streams.begin(), m_streams.end(), quint64(0),
                            [](quint64 value, const auto &s) { return value + s.second.dataSize; });

    if (dataSize > MaxQueueSize)
        return false;

    return true;
}

void Demuxer::ensureSeeked()
{
    if (std::exchange(m_seeked, true))
        return;

    const qint64 seekPos = m_seekPos * AV_TIME_BASE / 1000000;
    auto err = av_seek_frame(m_context, -1, seekPos, AVSEEK_FLAG_BACKWARD);

    if (err < 0) {
        qDebug() << err2str(err);
        emit error(err, err2str(err));
        return;
    }

    setAtEnd(false);
    scheduleNextStep();
}

Demuxer::RequestingSignal Demuxer::signalByTrackType(QPlatformMediaPlayer::TrackType trackType)
{
    switch (trackType) {
    case QPlatformMediaPlayer::TrackType::VideoStream:
        return &Demuxer::requestProcessVideoPacket;
    case QPlatformMediaPlayer::TrackType::AudioStream:
        return &Demuxer::requestProcessAudioPacket;
    case QPlatformMediaPlayer::TrackType::SubtitleStream:
        return &Demuxer::requestProcessSubtitlePacket;
    default:
        Q_ASSERT(!"Unknown track type");
    }

    return nullptr;
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegdemuxer_p.cpp"
