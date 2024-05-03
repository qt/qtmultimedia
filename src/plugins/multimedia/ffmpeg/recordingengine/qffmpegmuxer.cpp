// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegmuxer_p.h"
#include "qffmpegrecordingengine_p.h"
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

static Q_LOGGING_CATEGORY(qLcFFmpegMuxer, "qt.multimedia.ffmpeg.muxer");

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

void Muxer::init()
{
    qCDebug(qLcFFmpegMuxer) << "Muxer::init started thread.";
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

    // the function takes ownership for the packet
    av_interleaved_write_frame(m_encoder->avFormatContext(), packet.release());
}

} // namespace QFFmpeg

QT_END_NAMESPACE
