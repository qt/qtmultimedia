// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegstreamdecoder_p.h"
#include "playbackengine/qffmpegmediadataholder_p.h"
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcStreamDecoder, "qt.multimedia.ffmpeg.streamdecoder");

namespace QFFmpeg {

StreamDecoder::StreamDecoder(const CodecContext &codecContext, TrackPosition absSeekPos)
    : m_codecContext(codecContext),
      m_absSeekPos(absSeekPos),
      m_trackType(MediaDataHolder::trackTypeFromMediaType(codecContext.context()->codec_type))
{
    qCDebug(qLcStreamDecoder) << "Create stream decoder, trackType" << m_trackType
                              << "absSeekPos:" << absSeekPos.get();
    Q_ASSERT(m_trackType != QPlatformMediaPlayer::NTrackTypes);
}

StreamDecoder::~StreamDecoder()
{
    avcodec_flush_buffers(m_codecContext.context());
}

void StreamDecoder::onFinalPacketReceived()
{
    decode({});
}

void StreamDecoder::decode(Packet packet)
{
    m_packets.enqueue(packet);

    scheduleNextStep();
}

void StreamDecoder::doNextStep()
{
    auto packet = m_packets.dequeue();

    auto decodePacket = [this](Packet packet) {
        if (trackType() == QPlatformMediaPlayer::SubtitleStream)
            decodeSubtitle(packet);
        else
            decodeMedia(packet);
    };

    if (packet.isValid() && packet.loopOffset().loopIndex != m_offset.loopIndex) {
        decodePacket({});

        qCDebug(qLcStreamDecoder) << "flush buffers due to new loop:"
                                  << packet.loopOffset().loopIndex;

        avcodec_flush_buffers(m_codecContext.context());
        m_offset = packet.loopOffset();
    }

    decodePacket(packet);

    setAtEnd(!packet.isValid());

    if (packet.isValid())
        emit packetProcessed(packet);

    scheduleNextStep(false);
}

QPlatformMediaPlayer::TrackType StreamDecoder::trackType() const
{
    return m_trackType;
}

qint32 StreamDecoder::maxQueueSize(QPlatformMediaPlayer::TrackType type)
{
    switch (type) {

    case QPlatformMediaPlayer::VideoStream:
        return 3;
    case QPlatformMediaPlayer::AudioStream:
        return 9;
    case QPlatformMediaPlayer::SubtitleStream:
        return 6; /*main packet and closing packet*/
    default:
        Q_UNREACHABLE_RETURN(-1);
    }
}

void StreamDecoder::onFrameProcessed(Frame frame)
{
    if (frame.sourceId() != id())
        return;

    --m_pendingFramesCount;
    Q_ASSERT(m_pendingFramesCount >= 0);

    scheduleNextStep();
}

bool StreamDecoder::canDoNextStep() const
{
    const qint32 maxCount = maxQueueSize(m_trackType);

    return !m_packets.empty() && m_pendingFramesCount < maxCount
            && PlaybackEngineObject::canDoNextStep();
}

void StreamDecoder::onFrameFound(Frame frame)
{
    if (frame.isValid() && frame.absoluteEnd() < m_absSeekPos)
        return;

    Q_ASSERT(m_pendingFramesCount >= 0);
    ++m_pendingFramesCount;
    emit requestHandleFrame(frame);
}

void StreamDecoder::decodeMedia(Packet packet)
{
    auto sendPacketResult = sendAVPacket(packet);

    if (sendPacketResult == AVERROR(EAGAIN)) {
        // Doc says:
        //  AVERROR(EAGAIN): input is not accepted in the current state - user
        //                   must read output with avcodec_receive_frame() (once
        //                   all output is read, the packet should be resent, and
        //                   the call will not fail with EAGAIN).
        receiveAVFrames();
        sendPacketResult = sendAVPacket(packet);

        if (sendPacketResult != AVERROR(EAGAIN))
            qWarning() << "Unexpected FFmpeg behavior";
    }

    if (sendPacketResult == 0)
        receiveAVFrames(!packet.isValid());
}

int StreamDecoder::sendAVPacket(Packet packet)
{
    return avcodec_send_packet(m_codecContext.context(), packet.isValid() ? packet.avPacket() : nullptr);
}

void StreamDecoder::receiveAVFrames(bool flushPacket)
{
    while (true) {
        auto avFrame = makeAVFrame();

        const auto receiveFrameResult = avcodec_receive_frame(m_codecContext.context(), avFrame.get());

        if (receiveFrameResult == AVERROR_EOF || receiveFrameResult == AVERROR(EAGAIN)) {
            if (flushPacket && receiveFrameResult == AVERROR(EAGAIN)) {
            // The documentation says that in the EAGAIN state output is not available. The new
            // input must be sent. It does not say that this state can also be returned for
            // Android MediaCodec when the ff_AMediaCodec_dequeueOutputBuffer call times out.
            // The flush packet means it is the end of the stream. No more packets are available,
            // so getting EAGAIN is unexpected here. At this point, the EAGAIN status was probably
            // caused by a timeout in the ffmpeg implementation, not by too few packets. That is
            // why there will be another try of calling avcodec_receive_frame
                qWarning() << "Unexpected FFmpeg behavior: EAGAIN state for avcodec_receive_frame "
                           << "at end of the stream";
                flushPacket = false;
                continue;
            }
            break;
        }

        if (receiveFrameResult < 0) {
            emit error(QMediaPlayer::FormatError, err2str(receiveFrameResult));
            break;
        }


        // Avoid starvation on FFmpeg decoders with fixed size frame pool
        if (m_trackType == QPlatformMediaPlayer::VideoStream)
            avFrame = copyFromHwPool(std::move(avFrame));

        onFrameFound({ m_offset, std::move(avFrame), m_codecContext, id() });
    }
}

void StreamDecoder::decodeSubtitle(Packet packet)
{
    if (!packet.isValid())
        return;
    //    qCDebug(qLcDecoder) << "    decoding subtitle" << "has delay:" <<
    //    (codec->codec->capabilities & AV_CODEC_CAP_DELAY);
    AVSubtitle subtitle;
    memset(&subtitle, 0, sizeof(subtitle));
    int gotSubtitle = 0;

    const int res =
            avcodec_decode_subtitle2(m_codecContext.context(), &subtitle, &gotSubtitle, packet.avPacket());
    //    qCDebug(qLcDecoder) << "       subtitle got:" << res << gotSubtitle << subtitle.format <<
    //    Qt::hex << (quint64)subtitle.pts;
    if (res < 0 || !gotSubtitle)
        return;

    // apparently the timestamps in the AVSubtitle structure are not always filled in
    // if they are missing, use the packets pts and duration values instead
    TrackPosition start = 0, end = 0;
    if (subtitle.pts == AV_NOPTS_VALUE) {
        start = m_codecContext.toTrackPosition(AVStreamPosition(packet.avPacket()->pts));
        end = start + m_codecContext.toTrackDuration(AVStreamDuration(packet.avPacket()->duration));
    } else {
        auto pts = timeStampUs(subtitle.pts, AVRational{ 1, AV_TIME_BASE });
        start = TrackPosition(*pts + qint64(subtitle.start_display_time) * 1000);
        end = TrackPosition(*pts + qint64(subtitle.end_display_time) * 1000);
    }

    if (end <= start) {
        qWarning() << "Invalid subtitle time";
        return;
    }
    //        qCDebug(qLcDecoder) << "    got subtitle (" << start << "--" << end << "):";
    QString text;
    for (uint i = 0; i < subtitle.num_rects; ++i) {
        const auto *r = subtitle.rects[i];
        //            qCDebug(qLcDecoder) << "    subtitletext:" << r->text << "/" << r->ass;
        if (i)
            text += QLatin1Char('\n');
        if (r->text)
            text += QString::fromUtf8(r->text);
        else {
            const char *ass = r->ass;
            int nCommas = 0;
            while (*ass) {
                if (nCommas == 8)
                    break;
                if (*ass == ',')
                    ++nCommas;
                ++ass;
            }
            text += QString::fromUtf8(ass);
        }
    }
    text.replace(QLatin1String("\\N"), QLatin1String("\n"));
    text.replace(QLatin1String("\\n"), QLatin1String("\n"));
    text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    if (text.endsWith(QLatin1Char('\n')))
        text.chop(1);

    onFrameFound({ m_offset, text, start, end - start, id() });

    // TODO: maybe optimize
    onFrameFound({ m_offset, QString(), end, TrackDuration(0), id() });
}
} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegstreamdecoder_p.cpp"
