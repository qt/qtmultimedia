// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegdemuxer_p.h"
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

// 4 sec for buffering. TODO: maybe move to env var customization
static constexpr qint64 MaxBufferingTimeUs = 4'000'000;

// Currently, consider only time. TODO: maybe move to env var customization
static constexpr qint64 MaxBufferingSize = std::numeric_limits<qint64>::max();

namespace QFFmpeg {

static Q_LOGGING_CATEGORY(qLcDemuxer, "qt.multimedia.ffmpeg.demuxer");

static qint64 streamTimeToUs(const AVStream *stream, qint64 time)
{
    Q_ASSERT(stream);

    const auto res = mul(time * 1000000, stream->time_base);
    return res ? *res : time;
}

Demuxer::Demuxer(AVFormatContext *context, const PositionWithOffset &posWithOffset,
                 const StreamIndexes &streamIndexes, int loops)
    : m_context(context), m_posWithOffset(posWithOffset), m_loops(loops)
{
    qCDebug(qLcDemuxer) << "Create demuxer."
                        << "pos:" << posWithOffset.pos << "loop offset:" << posWithOffset.offset.pos
                        << "loop index:" << posWithOffset.offset.index << "loops:" << loops;

    Q_ASSERT(m_context);
    Q_ASSERT(loops < 0 || m_posWithOffset.offset.index < loops);

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
            setAtEnd(true);
        } else {
            m_seeked = false;
            m_posWithOffset.pos = 0;
            m_posWithOffset.offset.pos = m_endPts;
            m_endPts = 0;

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
        const auto packetEndPos = streamTimeToUs(stream, avPacket.pts + avPacket.duration);
        m_endPts = std::max(m_endPts, m_posWithOffset.offset.pos + packetEndPos);

        it->second.bufferingTime += streamTimeToUs(stream, avPacket.duration);
        it->second.bufferingSize += avPacket.size;

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
    auto it = m_streams.find(streamIndex);

    if (it != m_streams.end()) {
        it->second.bufferingTime -= streamTimeToUs(m_context->streams[streamIndex], avPacket.duration);
        it->second.bufferingSize -= avPacket.size;

        Q_ASSERT(it->second.bufferingTime >= 0);
        Q_ASSERT(it->second.bufferingSize >= 0);
    }

    scheduleNextStep();
}

bool Demuxer::canDoNextStep() const
{
    if (!PlaybackEngineObject::canDoNextStep() || isAtEnd() || m_streams.empty())
        return false;

    auto checkBufferingTime = [](const auto &streamIndexToData) {
        return streamIndexToData.second.bufferingTime < MaxBufferingTimeUs &&
               streamIndexToData.second.bufferingSize < MaxBufferingSize;
    };

    return std::all_of(m_streams.begin(), m_streams.end(), checkBufferingTime);
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

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegdemuxer_p.cpp"
