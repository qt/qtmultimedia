// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegmuxer_p.h"
#include "qffmpegrecordingengine_p.h"
#include "qffmpegrecordingengineutils_p.h"
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

Q_STATIC_LOGGING_CATEGORY(qLcFFmpegMuxer, "qt.multimedia.ffmpeg.muxer");

Muxer::Muxer(RecordingEngine *encoder) : m_encoder(encoder)
{
    setObjectName(QLatin1String("Muxer"));
}

void Muxer::addPacket(AVPacketUPtr packet)
{
    {
        QMutexLocker locker = lockLoopData();
        m_packetQueue.push(std::move(packet));
    }

    //    qCDebug(qLcFFmpegEncoder) << "Muxer::addPacket" << packet->pts << packet->stream_index;
    dataReady();
}

AVPacketUPtr Muxer::takePacket()
{
    QMutexLocker locker = lockLoopData();
    return dequeueIfPossible(m_packetQueue);
}

bool Muxer::init()
{
    qCDebug(qLcFFmpegMuxer) << "Muxer::init started thread.";
    return true;
}

void Muxer::cleanup()
{
    while (!m_packetQueue.empty())
        processOne();
}

bool QFFmpeg::Muxer::hasData() const
{
    return !m_packetQueue.empty();
}

void Muxer::processOne()
{
    auto packet = takePacket();
    //   qCDebug(qLcFFmpegEncoder) << "writing packet to file" << packet->pts << packet->duration <<
    //   packet->stream_index;

    // from AVPacket Struct Reference:
    // "The semantics of data ownership depends on the buf field. If it is set, the packet data is
    // dynamically allocated and is valid indefinitely until a call to av_packet_unref() reduces the
    // reference count to 0."
    //
    // from av_interleaved_write_frame:
    // "If the packet is reference-counted, this function will take ownership of this reference and
    // unreference it later when it sees fit. The caller must not access the data through this
    // reference after this function returns. If the packet is not reference-counted, libavformat
    // will make a copy."
    //
    // This means that av_interleaved_write_frame takes ownership of the AVBufferRef field, not the
    // whole AVPacket.

    av_interleaved_write_frame(m_encoder->avFormatContext(), packet.get());
    Q_ASSERT(packet->buf == nullptr); // av_interleaved_write_frame took ownership of the buffer
}

} // namespace QFFmpeg

QT_END_NAMESPACE
