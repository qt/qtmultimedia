// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegvideoencoder_p.h"
#include "qffmpegmuxer_p.h"
#include "qffmpegvideobuffer_p.h"
#include "qffmpegrecordingengine_p.h"
#include "qffmpegvideoframeencoder_p.h"
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

static Q_LOGGING_CATEGORY(qLcFFmpegVideoEncoder, "qt.multimedia.ffmpeg.videoencoder");


VideoEncoder::VideoEncoder(RecordingEngine *encoder, const QMediaEncoderSettings &settings,
                           const QVideoFrameFormat &format, std::optional<AVPixelFormat> hwFormat)
    : EncoderThread(encoder)
{
    setObjectName(QLatin1String("VideoEncoder"));

    AVPixelFormat swFormat = QFFmpegVideoBuffer::toAVPixelFormat(format.pixelFormat());
    AVPixelFormat ffmpegPixelFormat =
            hwFormat && *hwFormat != AV_PIX_FMT_NONE ? *hwFormat : swFormat;
    auto frameRate = format.frameRate();
    if (frameRate <= 0.) {
        qWarning() << "Invalid frameRate" << frameRate << "; Using the default instead";

        // set some default frame rate since ffmpeg has UB if it's 0.
        frameRate = 30.;
    }

    m_frameEncoder =
            VideoFrameEncoder::create(settings, format.frameSize(), frameRate, ffmpegPixelFormat,
                                      swFormat, encoder->avFormatContext());
}

VideoEncoder::~VideoEncoder() = default;

bool VideoEncoder::isValid() const
{
    return m_frameEncoder != nullptr;
}

void VideoEncoder::addFrame(const QVideoFrame &frame)
{
    QMutexLocker locker(&m_queueMutex);

    // Drop frames if encoder can not keep up with the video source data rate
    const bool queueFull = m_videoFrameQueue.size() >= m_maxQueueSize;

    if (queueFull) {
        qCDebug(qLcFFmpegVideoEncoder) << "RecordingEngine frame queue full. Frame lost.";
    } else if (!m_paused.loadRelaxed()) {
        m_videoFrameQueue.push(frame);

        locker.unlock(); // Avoid context switch on wake wake-up

        dataReady();
    }
}

QVideoFrame VideoEncoder::takeFrame()
{
    QMutexLocker locker(&m_queueMutex);
    return dequeueIfPossible(m_videoFrameQueue);
}

void VideoEncoder::retrievePackets()
{
    if (!m_frameEncoder)
        return;
    while (auto packet = m_frameEncoder->retrievePacket())
        m_encoder->getMuxer()->addPacket(std::move(packet));
}

void VideoEncoder::init()
{
    qCDebug(qLcFFmpegVideoEncoder) << "VideoEncoder::init started video device thread.";
    bool ok = m_frameEncoder->open();
    if (!ok)
        emit m_encoder->error(QMediaRecorder::ResourceError, "Could not initialize encoder");
}

void VideoEncoder::cleanup()
{
    while (!m_videoFrameQueue.empty())
        processOne();
    if (m_frameEncoder) {
        while (m_frameEncoder->sendFrame(nullptr) == AVERROR(EAGAIN))
            retrievePackets();
        retrievePackets();
    }
}

bool VideoEncoder::hasData() const
{
    QMutexLocker locker(&m_queueMutex);
    return !m_videoFrameQueue.empty();
}

} // namespace QFFmpeg

QT_END_NAMESPACE
