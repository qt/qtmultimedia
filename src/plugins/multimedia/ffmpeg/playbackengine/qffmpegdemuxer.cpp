// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegdemuxer_p.h"
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

// 4 sec for buffering. TODO: maybe move to env var customization
static constexpr qint64 MaxBufferedDurationUs = 4'000'000;

// around 4 sec of hdr video
static constexpr qint64 MaxBufferedSize = 32 * 1024 * 1024;

namespace QFFmpeg {

static Q_LOGGING_CATEGORY(qLcDemuxer, "qt.multimedia.ffmpeg.demuxer");

static qint64 streamTimeToUs(const AVStream *stream, qint64 time)
{
    Q_ASSERT(stream);

    const auto res = mul(time * 1000000, stream->time_base);
    return res ? *res : time;
}

static qint64 packetEndPos(const AVStream *stream, const Packet &packet)
{
    return packet.loopOffset().pos
            + streamTimeToUs(stream, packet.avPacket()->pts + packet.avPacket()->duration);
}

Demuxer::Demuxer(AVFormatContext *context, const PositionWithOffset &posWithOffset,
                 const StreamIndexes &streamIndexes, int loops)
    : m_context(context), m_posWithOffset(posWithOffset), m_loops(loops)
{
    qCDebug(qLcDemuxer) << "Create demuxer."
                        << "pos:" << posWithOffset.pos << "loop offset:" << posWithOffset.offset.pos
                        << "loop index:" << posWithOffset.offset.index << "loops:" << loops;

    Q_ASSERT(m_context);

    for (auto i = 0; i < QPlatformMediaPlayer::NTrackTypes; ++i) {
        if (streamIndexes[i] >= 0) {
            const auto trackType = static_cast<QPlatformMediaPlayer::TrackType>(i);
            qCDebug(qLcDemuxer) << "Activate demuxing stream" << i << ", trackType:" << trackType;
            m_streams[streamIndexes[i]] = { trackType };
        }
    }
}

void Demuxer::doNextStep()
{
    ensureSeeked();

    Packet packet(m_posWithOffset.offset, AVPacketUPtr{ av_packet_alloc() }, id());
    if (av_read_frame(m_context, packet.avPacket()) < 0) {
        ++m_posWithOffset.offset.index;

        const auto loops = m_loops.loadAcquire();
        if (loops >= 0 && m_posWithOffset.offset.index >= loops) {
            qCDebug(qLcDemuxer) << "finish demuxing";

            if (!std::exchange(m_buffered, true))
                emit packetsBuffered();

            setAtEnd(true);
        } else {
            m_seeked = false;
            m_posWithOffset.pos = 0;
            m_posWithOffset.offset.pos = m_maxPacketsEndPos;
            m_maxPacketsEndPos = 0;

            ensureSeeked();

            qCDebug(qLcDemuxer) << "Demuxer loops changed. Index:" << m_posWithOffset.offset.index
                                << "Offset:" << m_posWithOffset.offset.pos;

            scheduleNextStep(false);
        }

        return;
    }

    auto &avPacket = *packet.avPacket();

    const auto streamIndex = avPacket.stream_index;
    const auto stream = m_context->streams[streamIndex];

    auto it = m_streams.find(streamIndex);
    if (it != m_streams.end()) {
        auto &streamData = it->second;

        const auto endPos = packetEndPos(stream, packet);
        m_maxPacketsEndPos = qMax(m_maxPacketsEndPos, endPos);

        // Increase buffered metrics as the packet has been processed.

        streamData.bufferedDuration += streamTimeToUs(stream, avPacket.duration);
        streamData.bufferedSize += avPacket.size;
        streamData.maxSentPacketsPos = qMax(streamData.maxSentPacketsPos, endPos);
        updateStreamDataLimitFlag(streamData);

        if (!m_buffered && streamData.isDataLimitReached) {
            m_buffered = true;
            emit packetsBuffered();
        }

        if (!m_firstPacketFound) {
            m_firstPacketFound = true;
            const auto pos = streamTimeToUs(stream, avPacket.pts);
            emit firstPacketFound(std::chrono::steady_clock::now(), pos);
        }

        auto signal = signalByTrackType(it->second.trackType);
        emit (this->*signal)(packet);
    }

    scheduleNextStep(false);
}

void Demuxer::onPacketProcessed(Packet packet)
{
    Q_ASSERT(packet.isValid());

    if (packet.sourceId() != id())
        return;

    auto &avPacket = *packet.avPacket();

    const auto streamIndex = avPacket.stream_index;
    const auto stream = m_context->streams[streamIndex];
    auto it = m_streams.find(streamIndex);

    if (it != m_streams.end()) {
        auto &streamData = it->second;

        // Decrease buffered metrics as new data (the packet) has been received (buffered)

        streamData.bufferedDuration -= streamTimeToUs(stream, avPacket.duration);
        streamData.bufferedSize -= avPacket.size;
        streamData.maxProcessedPacketPos =
                qMax(streamData.maxProcessedPacketPos, packetEndPos(stream, packet));

        Q_ASSERT(it->second.bufferedDuration >= 0);
        Q_ASSERT(it->second.bufferedSize >= 0);

        updateStreamDataLimitFlag(streamData);
    }

    scheduleNextStep();
}

bool Demuxer::canDoNextStep() const
{
    auto isDataLimitReached = [](const auto &streamIndexToData) {
        return streamIndexToData.second.isDataLimitReached;
    };

    // Demuxer waits:
    //     - if it's paused
    //     - if the end has been reached
    //     - if streams are empty (probably, should be handled on the initialization)
    //     - if at least one of the streams has reached the data limit (duration or size)

    return PlaybackEngineObject::canDoNextStep() && !isAtEnd() && !m_streams.empty()
            && std::none_of(m_streams.begin(), m_streams.end(), isDataLimitReached);
}

void Demuxer::ensureSeeked()
{
    if (std::exchange(m_seeked, true))
        return;

    if ((m_context->ctx_flags & AVFMTCTX_UNSEEKABLE) == 0) {
        const qint64 seekPos = m_posWithOffset.pos * AV_TIME_BASE / 1000000;
        auto err = av_seek_frame(m_context, -1, seekPos, AVSEEK_FLAG_BACKWARD);

        if (err < 0) {
            qCWarning(qLcDemuxer) << "Failed to seek, pos" << seekPos;

            // Drop an error of seeking to initial position of streams with undefined duration.
            // This needs improvements.
            if (seekPos != 0 || m_context->duration > 0)
                emit error(QMediaPlayer::ResourceError,
                           QLatin1StringView("Failed to seek: ") + err2str(err));
        }
    }

    setAtEnd(false);
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

void Demuxer::setLoops(int loopsCount)
{
    qCDebug(qLcDemuxer) << "setLoops to demuxer" << loopsCount;
    m_loops.storeRelease(loopsCount);
}

void Demuxer::updateStreamDataLimitFlag(StreamData &streamData)
{
    const auto packetsPosDiff = streamData.maxSentPacketsPos - streamData.maxProcessedPacketPos;
    streamData.isDataLimitReached =
           streamData.bufferedDuration >= MaxBufferedDurationUs
        || (streamData.bufferedDuration == 0 && packetsPosDiff >= MaxBufferedDurationUs)
        || streamData.bufferedSize >= MaxBufferedSize;
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegdemuxer_p.cpp"
