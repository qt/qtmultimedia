// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegstreamdecoder_p.h"
#include "playbackengine/qffmpegmediadataholder_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

StreamDecoder::StreamDecoder(const Codec &codec, qint64 seekPos)
    : m_codec(codec),
      m_seekPos(seekPos),
      m_trackType(MediaDataHolder::trackTypeFromMediaType(codec.context()->codec_type))
{
    Q_ASSERT(m_trackType != QPlatformMediaPlayer::NTrackTypes);
}

StreamDecoder::~StreamDecoder()
{
    avcodec_flush_buffers(m_codec.context());
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
    if (trackType() == QPlatformMediaPlayer::SubtitleStream)
        decodeSubtitle(packet);
    else
        decodeMedia(packet);

    setAtEnd(!packet.isValid());

    if (packet.isValid())
        emit packetProcessed(packet);

    scheduleNextStep(false);
}

QPlatformMediaPlayer::TrackType StreamDecoder::trackType() const
{
    return m_trackType;
}

void StreamDecoder::onFrameProcessed(Frame frame)
{
    if (frame.source() != this)
        return;

    --m_pendingFramesCount;
    Q_ASSERT(m_pendingFramesCount >= 0);

    scheduleNextStep();
}

bool StreamDecoder::canDoNextStep() const
{
    constexpr qint32 maxPendingFramesCount = 3;
    constexpr qint32 maxPendingAudioFramesCount = 9;

    const auto maxCount = m_trackType == QPlatformMediaPlayer::AudioStream
            ? maxPendingAudioFramesCount
            : m_trackType == QPlatformMediaPlayer::SubtitleStream
            ? maxPendingFramesCount * 2 /*main packet and closing packet*/
            : maxPendingFramesCount;

    return !m_packets.empty() && m_pendingFramesCount < maxCount
            && PlaybackEngineObject::canDoNextStep();
}

void StreamDecoder::onFrameFound(Frame frame)
{
    if (frame.isValid() && frame.end() < m_seekPos)
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
            qWarning() << "Unexpected ffmpeg behavior";
    }

    if (sendPacketResult == 0)
        receiveAVFrames();
}

int StreamDecoder::sendAVPacket(Packet packet)
{
    return avcodec_send_packet(m_codec.context(), packet.isValid() ? packet.avPacket() : nullptr);
}

void StreamDecoder::receiveAVFrames()
{
    while (true) {
        auto avFrame = makeAVFrame();

        const auto receiveFrameResult = avcodec_receive_frame(m_codec.context(), avFrame.get());

        if (receiveFrameResult == AVERROR_EOF || receiveFrameResult == AVERROR(EAGAIN))
            break;

        if (receiveFrameResult < 0) {
            emit error(QMediaPlayer::FormatError, err2str(receiveFrameResult));
            break;
        }

        onFrameFound({ std::move(avFrame), m_codec, 0, this });
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
            avcodec_decode_subtitle2(m_codec.context(), &subtitle, &gotSubtitle, packet.avPacket());
    //    qCDebug(qLcDecoder) << "       subtitle got:" << res << gotSubtitle << subtitle.format <<
    //    Qt::hex << (quint64)subtitle.pts;
    if (res < 0 || !gotSubtitle)
        return;

    // apparently the timestamps in the AVSubtitle structure are not always filled in
    // if they are missing, use the packets pts and duration values instead
    qint64 start, end;
    if (subtitle.pts == AV_NOPTS_VALUE) {
        start = m_codec.toUs(packet.avPacket()->pts);
        end = start + m_codec.toUs(packet.avPacket()->duration);
    } else {
        auto pts = timeStampUs(subtitle.pts, AVRational{ 1, AV_TIME_BASE });
        start = *pts + qint64(subtitle.start_display_time) * 1000;
        end = *pts + qint64(subtitle.end_display_time) * 1000;
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

    onFrameFound({ text, start, end - start, this });

    // TODO: maybe optimize
    onFrameFound({ QString(), end, 0, this });
}
} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegstreamdecoder_p.cpp"
