// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegvideoencoder_p.h"
#include "qffmpegmuxer_p.h"
#include "qffmpegvideobuffer_p.h"
#include "qffmpegrecordingengine_p.h"
#include "qffmpegvideoframeencoder_p.h"
#include "qffmpegrecordingengineutils_p.h"
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

static Q_LOGGING_CATEGORY(qLcFFmpegVideoEncoder, "qt.multimedia.ffmpeg.videoencoder");

VideoEncoder::VideoEncoder(RecordingEngine &recordingEngine, const QMediaEncoderSettings &settings,
                           const QVideoFrameFormat &format, std::optional<AVPixelFormat> hwFormat)
    : EncoderThread(recordingEngine)
{
    setObjectName(QLatin1String("VideoEncoder"));

    AVPixelFormat swFormat = QFFmpegVideoBuffer::toAVPixelFormat(format.pixelFormat());
    AVPixelFormat ffmpegPixelFormat =
            hwFormat && *hwFormat != AV_PIX_FMT_NONE ? *hwFormat : swFormat;
    auto frameRate = format.streamFrameRate();
    if (frameRate <= 0.) {
        qWarning() << "Invalid frameRate" << frameRate << "; Using the default instead";

        // set some default frame rate since ffmpeg has UB if it's 0.
        frameRate = 30.;
    }

    m_frameEncoder = VideoFrameEncoder::create(settings,
                                               format.frameSize(),
                                               format.rotation(),
                                               frameRate,
                                               ffmpegPixelFormat,
                                               swFormat,
                                               recordingEngine.avFormatContext());
}

VideoEncoder::~VideoEncoder() = default;

bool VideoEncoder::isValid() const
{
    return m_frameEncoder != nullptr;
}

void VideoEncoder::addFrame(const QVideoFrame &frame)
{
    if (!frame.isValid()) // TODO: report endOfStream
        return;

    {
        auto guard = lockLoopData();

        if (m_paused) {
            m_shouldAdjustTimeBaseForNextFrame = true;
            return;
        }

        // Drop frames if encoder can not keep up with the video source data rate;
        // canPushFrame might be used instead
        const bool queueFull = m_videoFrameQueue.size() >= m_maxQueueSize;

        if (queueFull) {
            qCDebug(qLcFFmpegVideoEncoder) << "RecordingEngine frame queue full. Frame lost.";
            return;
        }

        m_videoFrameQueue.push({ frame, m_shouldAdjustTimeBaseForNextFrame });
        m_shouldAdjustTimeBaseForNextFrame = false;
    }

    dataReady();
}

VideoEncoder::FrameInfo VideoEncoder::takeFrame()
{
    auto guard = lockLoopData();
    return dequeueIfPossible(m_videoFrameQueue);
}

void VideoEncoder::retrievePackets()
{
    if (!m_frameEncoder)
        return;
    while (auto packet = m_frameEncoder->retrievePacket())
        m_recordingEngine.getMuxer()->addPacket(std::move(packet));
}

void VideoEncoder::init()
{
    Q_ASSERT(isValid());

    qCDebug(qLcFFmpegVideoEncoder) << "VideoEncoder::init started video device thread.";
    bool ok = m_frameEncoder->open();
    if (!ok)
        emit m_recordingEngine.sessionError(QMediaRecorder::ResourceError,
                                            "Could not initialize encoder");
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
    return !m_videoFrameQueue.empty();
}

struct QVideoFrameHolder
{
    QVideoFrame f;
    QImage i;
};

static void freeQVideoFrame(void *opaque, uint8_t *)
{
    delete reinterpret_cast<QVideoFrameHolder *>(opaque);
}

void VideoEncoder::processOne()
{
    retrievePackets();

    FrameInfo frameInfo = takeFrame();
    QVideoFrame &frame = frameInfo.frame;
    Q_ASSERT(frame.isValid());

    if (!isValid())
        return;

    //    qCDebug(qLcFFmpegEncoder) << "new video buffer" << frame.startTime();

    AVFrameUPtr avFrame;

    auto *videoBuffer = dynamic_cast<QFFmpegVideoBuffer *>(frame.videoBuffer());
    if (videoBuffer) {
        // ffmpeg video buffer, let's use the native AVFrame stored in there
        auto *hwFrame = videoBuffer->getHWFrame();
        if (hwFrame && hwFrame->format == m_frameEncoder->sourceFormat())
            avFrame.reset(av_frame_clone(hwFrame));
    }

    if (!avFrame) {
        frame.map(QVideoFrame::ReadOnly);
        auto size = frame.size();
        avFrame = makeAVFrame();
        avFrame->format = m_frameEncoder->sourceFormat();
        avFrame->width = size.width();
        avFrame->height = size.height();

        for (int i = 0; i < 4; ++i) {
            avFrame->data[i] = const_cast<uint8_t *>(frame.bits(i));
            avFrame->linesize[i] = frame.bytesPerLine(i);
        }

        QImage img;
        if (frame.pixelFormat() == QVideoFrameFormat::Format_Jpeg) {
            // the QImage is cached inside the video frame, so we can take the pointer to the image
            // data here
            img = frame.toImage();
            avFrame->data[0] = (uint8_t *)img.bits();
            avFrame->linesize[0] = img.bytesPerLine();
        }

        Q_ASSERT(avFrame->data[0]);
        // ensure the video frame and it's data is alive as long as it's being used in the encoder
        avFrame->opaque_ref = av_buffer_create(nullptr, 0, freeQVideoFrame,
                                               new QVideoFrameHolder{ frame, img }, 0);
    }

    const auto [startTime, endTime] = frameTimeStamps(frame);

    if (frameInfo.shouldAdjustTimeBase) {
        m_baseTime += startTime - m_lastFrameTime;
        qCDebug(qLcFFmpegVideoEncoder)
                << ">>>> adjusting base time to" << m_baseTime << startTime << m_lastFrameTime;
    }

    const qint64 time = startTime - m_baseTime;
    m_lastFrameTime = endTime;

    setAVFrameTime(*avFrame, m_frameEncoder->getPts(time), m_frameEncoder->getTimeBase());

    m_recordingEngine.newTimeStamp(time / 1000);

    qCDebug(qLcFFmpegVideoEncoder)
            << ">>> sending frame" << avFrame->pts << time << m_lastFrameTime;
    int ret = m_frameEncoder->sendFrame(std::move(avFrame));
    if (ret < 0) {
        qCDebug(qLcFFmpegVideoEncoder) << "error sending frame" << ret << err2str(ret);
        emit m_recordingEngine.sessionError(QMediaRecorder::ResourceError, err2str(ret));
    }
}

bool VideoEncoder::checkIfCanPushFrame() const
{
    if (isRunning())
        return m_videoFrameQueue.size() < m_maxQueueSize;
    if (!isFinished())
        return m_videoFrameQueue.empty();

    return false;
}

std::pair<qint64, qint64> VideoEncoder::frameTimeStamps(const QVideoFrame &frame) const
{
    qint64 startTime = frame.startTime();
    qint64 endTime = frame.endTime();

    if (startTime == -1) {
        startTime = m_lastFrameTime;
        endTime = -1;
    }

    if (endTime == -1) {
        qreal frameRate = frame.streamFrameRate();
        if (frameRate <= 0.)
            frameRate = m_frameEncoder->settings().videoFrameRate();

        Q_ASSERT(frameRate > 0.f);
        endTime = startTime + static_cast<qint64>(std::round(1000000 / frameRate));
    }

    return { startTime, endTime };
}

} // namespace QFFmpeg

QT_END_NAMESPACE
