// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegdemuxer_p.h"
#include <qloggingcategory.h>
#include <chrono>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

// 4 sec for buffering. TODO: maybe move to env var customization
static constexpr TrackDuration MaxBufferedDurationUs{ 4'000'000 };

// around 4 sec of hdr video
static constexpr qint64 MaxBufferedSize = 32 * 1024 * 1024;

Q_STATIC_LOGGING_CATEGORY(qLcDemuxer, "qt.multimedia.ffmpeg.demuxer");

static TrackPosition packetEndPos(const Packet &packet, const AVStream *stream,
                                  const AVFormatContext *context)
{
    const AVPacket &avPacket = *packet.avPacket();
    return packet.loopOffset().loopStartTimeUs.asDuration()
            + toTrackPosition(AVStreamPosition(avPacket.pts + avPacket.duration), stream, context);
}

static bool isPacketWithinStreamDuration(const AVFormatContext *context, const Packet &packet)
{
    const AVPacket &avPacket = *packet.avPacket();
    const AVStream &avStream = *context->streams[avPacket.stream_index];
    const AVStreamDuration streamDuration(avStream.duration);
    if (streamDuration.get() <= 0
        || context->duration_estimation_method != AVFMT_DURATION_FROM_STREAM)
        return true; // Stream duration shouldn't or doesn't need to be compared to pts

    if (avPacket.pts == AV_NOPTS_VALUE) { // Unexpected situation
        qWarning() << "QFFmpeg::Demuxer received AVPacket with pts == AV_NOPTS_VALUE";
        return true;
    }

    if (avStream.start_time != AV_NOPTS_VALUE)
        return AVStreamDuration(avPacket.pts - avStream.start_time) <= streamDuration;

    const TrackPosition trackPos = toTrackPosition(AVStreamPosition(avPacket.pts), &avStream, context);
    const TrackPosition trackPosOfStreamEnd = toTrackDuration(streamDuration, &avStream).asTimePoint();
    return trackPos <= trackPosOfStreamEnd;

    // TODO: If there is a packet that starts before the canonical end of the stream but has a
    // malformed duration, rework doNextStep to check for eof after that packet.
}

Demuxer::Demuxer(AVFormatContext *context, TrackPosition initialPosUs, bool seekPending,
                 const LoopOffset &loopOffset, const StreamIndexes &streamIndexes, int loops)
    : m_context(context),
      m_seeked(!seekPending && initialPosUs == TrackPosition{ 0 }), // Don't seek to 0 unless seek requested
      m_posInLoopUs{ initialPosUs },
      m_loopOffset(loopOffset),
      m_loops(loops)
{
    qCDebug(qLcDemuxer) << "Create demuxer."
                        << "pos:" << m_posInLoopUs.get()
                        << "loop offset:" << m_loopOffset.loopStartTimeUs.get()
                        << "loop index:" << m_loopOffset.loopIndex << "loops:" << loops;

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

    Packet packet(m_loopOffset, AVPacketUPtr{ av_packet_alloc() }, id());
    AVPacket &avPacket = *packet.avPacket();

    const int demuxStatus = av_read_frame(m_context, &avPacket);

    const int streamIndex = avPacket.stream_index;
    auto streamIterator = m_streams.find(streamIndex);
    const bool streamIsRelevant = streamIterator != m_streams.end();

    if (demuxStatus == AVERROR_EOF
        || (streamIsRelevant && !isPacketWithinStreamDuration(m_context, packet))) {
        ++m_loopOffset.loopIndex;

        const auto loops = m_loops.loadAcquire();
        if (loops >= 0 && m_loopOffset.loopIndex >= loops) {
            qCDebug(qLcDemuxer) << "finish demuxing";

            if (!std::exchange(m_buffered, true))
                emit packetsBuffered();

            setAtEnd(true);
        } else {
            // start next loop
            m_seeked = false;
            m_posInLoopUs = TrackPosition(0);
            m_loopOffset.loopStartTimeUs = m_maxPacketsEndPos;
            m_maxPacketsEndPos = TrackPosition(0);

            ensureSeeked();

            qCDebug(qLcDemuxer) << "Demuxer loops changed. Index:" << m_loopOffset.loopIndex
                                << "Offset:" << m_loopOffset.loopStartTimeUs.get();

            scheduleNextStep(false);
        }

        return;
    }

    if (demuxStatus < 0) {
        qCWarning(qLcDemuxer) << "Demuxing failed" << demuxStatus << AVError(demuxStatus);

        if (demuxStatus == AVERROR(EAGAIN) && m_demuxerRetryCount != s_maxDemuxerRetries) {
            // When demuxer reports EAGAIN, we can try to recover by calling av_read_frame again.
            // The documentation for av_read_frame does not mention this, but FFmpeg command line
            // tool does this, see input_thread() function in ffmpeg_demux.c. There, the response
            // is to sleep for 10 ms before trying again. NOTE: We do not have any known way of
            // reproducing this in our tests.
            ++m_demuxerRetryCount;

            qCDebug(qLcDemuxer) << "Retrying";
            scheduleNextStep(false);
        } else {
            // av_read_frame reports another error. This could for example happen if network is
            // disconnected while playing a network stream, where av_read_frame may return
            // ETIMEDOUT.
            // TODO: Demuxer errors should likely stop playback in media player examples.
            emit error(QMediaPlayer::ResourceError,
                       QLatin1StringView("Demuxing failed"));
        }

        return;
    }

    m_demuxerRetryCount = 0;

    if (streamIsRelevant) {
        auto &streamData = streamIterator->second;
        const AVStream *stream = m_context->streams[streamIndex];

        const TrackPosition endPos = packetEndPos(packet, stream, m_context);
        m_maxPacketsEndPos = qMax(m_maxPacketsEndPos, endPos);

        // Increase buffered metrics as the packet has been processed.

        streamData.bufferedDuration += toTrackDuration(AVStreamDuration(avPacket.duration), stream);
        streamData.bufferedSize += avPacket.size;
        streamData.maxSentPacketsPos = qMax(streamData.maxSentPacketsPos, endPos);
        updateStreamDataLimitFlag(streamData);

        if (!m_buffered && streamData.isDataLimitReached) {
            m_buffered = true;
            emit packetsBuffered();
        }

        if (!m_firstPacketFound) {
            m_firstPacketFound = true;
            emit firstPacketFound(id(), m_posInLoopUs + m_loopOffset.loopStartTimeUs.asDuration());
        }

        auto signal = signalByTrackType(streamData.trackType);
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

        streamData.bufferedDuration -= toTrackDuration(AVStreamDuration(avPacket.duration), stream);
        streamData.bufferedSize -= avPacket.size;
        streamData.maxProcessedPacketPos =
                qMax(streamData.maxProcessedPacketPos, packetEndPos(packet, stream, m_context));

        Q_ASSERT(it->second.bufferedDuration >= TrackDuration(0));
        Q_ASSERT(it->second.bufferedSize >= 0);

        updateStreamDataLimitFlag(streamData);
    }

    scheduleNextStep();
}

std::chrono::milliseconds Demuxer::timerInterval() const
{
    using namespace std::chrono_literals;
    return m_demuxerRetryCount != 0 ? s_demuxerRetryInterval : PlaybackEngineObject::timerInterval();
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

        // m_posInLoopUs is intended to be the number of microseconds since playback start, and is
        // in the range [0, duration()]. av_seek_frame seeks to a position relative to the start of
        // the media timeline, which may be non-zero. We adjust for this by adding the
        // AVFormatContext's start_time.
        //
        // NOTE: m_posInLoop is not calculated correctly if the start_time is non-zero, but
        // this must be fixed separately.
        const AVContextPosition seekPos = toContextPosition(m_posInLoopUs, m_context);

        qCDebug(qLcDemuxer).nospace()
                << "Seeking to offset " << m_posInLoopUs.get() << "us from media start.";

        auto err = av_seek_frame(m_context, -1, seekPos.get(), AVSEEK_FLAG_BACKWARD);

        if (err < 0) {
            qCWarning(qLcDemuxer) << "Failed to seek, pos" << seekPos.get();

            // Drop an error of seeking to initial position of streams with undefined duration.
            // This needs improvements.
            if (m_posInLoopUs != TrackPosition{ 0 } || m_context->duration > 0)
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
    const TrackDuration packetsPosDiff =
            streamData.maxSentPacketsPos - streamData.maxProcessedPacketPos;
    streamData.isDataLimitReached = streamData.bufferedDuration >= MaxBufferedDurationUs
            || (streamData.bufferedDuration == TrackDuration(0)
                && packetsPosDiff >= MaxBufferedDurationUs)
            || streamData.bufferedSize >= MaxBufferedSize;
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegdemuxer_p.cpp"
