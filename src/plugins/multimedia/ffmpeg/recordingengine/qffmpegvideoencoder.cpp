// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegvideoencoder_p.h"
#include "qffmpegmuxer_p.h"
#include "qffmpegvideobuffer_p.h"
#include "qffmpegrecordingengine_p.h"
#include "qffmpegvideoframeencoder_p.h"
#include "qffmpegrecordingengineutils_p.h"
#include "private/qvideoframe_p.h"
#include "private/qmultimediautils_p.h"
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QFFmpeg {

Q_STATIC_LOGGING_CATEGORY(qLcFFmpegVideoEncoder, "qt.multimedia.ffmpeg.videoencoder");

VideoEncoder::VideoEncoder(RecordingEngine &recordingEngine, const QMediaEncoderSettings &settings,
                           const QVideoFrameFormat &format, std::optional<AVPixelFormat> hwFormat)
    : EncoderThread(recordingEngine), m_settings(settings)
{
    setObjectName(QLatin1String("VideoEncoder"));

    const AVPixelFormat swFormat = QFFmpegVideoBuffer::toAVPixelFormat(format.pixelFormat());
    if (format.streamFrameRate() > 0.)
        m_fixedSourceFrameRate = format.streamFrameRate();

    m_sourceParams.size = format.frameSize();
    m_sourceParams.format = hwFormat && *hwFormat != AV_PIX_FMT_NONE ? *hwFormat : swFormat;
    // Temporary: check isSwPixelFormat because of android issue (QTBUG-116836)
    // TODO: assign swFormat.
    m_sourceParams.swFormat =
            isSwPixelFormat(m_sourceParams.format) ? m_sourceParams.format : swFormat;
    m_sourceParams.transform = qNormalizedSurfaceTransformation(format);
    m_sourceParams.frameRate = m_fixedSourceFrameRate.value_or(0.f);
    m_sourceParams.colorTransfer = QFFmpeg::toAvColorTransfer(format.colorTransfer());
    m_sourceParams.colorSpace = QFFmpeg::toAvColorSpace(format.colorSpace());
    m_sourceParams.colorRange = QFFmpeg::toAvColorRange(format.colorRange());

    if (!m_settings.videoResolution().isValid())
        m_settings.setVideoResolution(m_sourceParams.size);

    // Use a provisional target before negotiation with codec. This is for first
    // frame, before init() is called
    const qreal provisionalTargetRate = m_settings.videoFrameRate() > 0.
            ? m_settings.videoFrameRate()
            : m_sourceParams.frameRate;
    m_frameRateAdapter.setRates(m_fixedSourceFrameRate, provisionalTargetRate);
}

VideoEncoder::~VideoEncoder() = default;

void VideoEncoder::addFrame(const QVideoFrame &frame)
{
    if (!frame.isValid()) {
        bool hasFlushed = false;
        {
            auto guard = lockLoopData();
            if (isInitialized()) {
                // Don't flush if not initialized, since frame rate used in adapter is only provisional
                // Will be flushed in init()
                std::optional<FrameInfo> flushed = m_frameRateAdapter.flush();
                if (flushed && m_videoFrameQueue.size() < m_maxQueueSize) {
                    m_videoFrameQueue.push(*flushed);
                    hasFlushed = true;
                }
            }
        }
        if (hasFlushed)
            dataReady();
        setEndOfSourceStream();
        return;
    }

    {
        auto guard = lockLoopData();
        resetEndOfSourceStream();

        if (m_paused) {
            m_shouldAdjustTimeBaseForNextFrame = true;
            m_frameRateAdapter.reset();
            m_framesBeforeInit = {};
            return;
        }

        if (!isInitialized()) {
            // m_frameRateAdapter isn't configured with the real codec frame rate yet, so
            // buffer the frame and adapt it once init() has set the real rates. Bounded by
            // m_maxFramesBeforeInit. Sources should use checkIfCanPushFrame.
            if (m_framesBeforeInit.size() >= m_maxFramesBeforeInit) {
                qCWarning(qLcFFmpegVideoEncoder) << "Too many frames received before encoder "
                                                    "initialization. Buffer full, dropping "
                                                    "frame.";
                return;
            }
            m_framesBeforeInit.push(frame);
            return;
        }

        const bool adjustTimeBase = m_shouldAdjustTimeBaseForNextFrame;
        m_shouldAdjustTimeBaseForNextFrame = false;

        if (m_videoFrameQueue.size() >= m_maxQueueSize) {
            qCDebug(qLcFFmpegVideoEncoder)
                    << "RecordingEngine frame queue full. Frame lost. This might mean we're trying "
                       "to encode at too high of a frame rate.";
            return;
        }

        std::vector<FrameInfo> adapted = m_frameRateAdapter.adapt(frame, adjustTimeBase);
        if (adapted.empty())
            return; // Adapter can return no frames if converting to a lower frame rate

        for (FrameInfo &adaptedFrame : adapted)
            m_videoFrameQueue.push(std::move(adaptedFrame));
    }

    dataReady();
}

FrameInfo VideoEncoder::takeFrame()
{
    auto guard = lockLoopData();
    return dequeueIfPossible(m_videoFrameQueue);
}

void VideoEncoder::retrievePackets()
{
    Q_ASSERT(m_frameEncoder);
    while (auto packet = m_frameEncoder->retrievePacket())
        m_recordingEngine.getMuxer()->addPacket(std::move(packet));
}

bool VideoEncoder::init()
{
    m_frameEncoder = VideoFrameEncoder::create(m_settings, m_sourceParams,
                                               m_recordingEngine.avFormatContext());

    qCDebug(qLcFFmpegVideoEncoder) << "VideoEncoder::init started video device thread.";
    if (!m_frameEncoder) {
        emit resolved(false);
        emit m_recordingEngine.sessionError(QMediaRecorder::ResourceError,
                                            u"Could not initialize encoder"_s);
        return false;
    }

    {
        auto guard = lockLoopData();
        m_frameRateAdapter.setRates(m_fixedSourceFrameRate, m_frameEncoder->codecFrameRate());

        while (!m_framesBeforeInit.empty()) {
            const QVideoFrame bufferedFrame = std::move(m_framesBeforeInit.front());
            m_framesBeforeInit.pop();

            std::vector<FrameInfo> adapted = m_frameRateAdapter.adapt(
                    bufferedFrame, std::exchange(m_shouldAdjustTimeBaseForNextFrame, false));
            for (FrameInfo &adaptedFrame : adapted)
                m_videoFrameQueue.push(std::move(adaptedFrame));
        }

        // EOS may have arrived before initialization. Flush now that the buffered
        // frames above have been adapted.
        if (isEndOfSourceStream()) {
            std::optional<FrameInfo> flushed = m_frameRateAdapter.flush();
            if (flushed && m_videoFrameQueue.size() < m_maxQueueSize)
                m_videoFrameQueue.push(std::move(*flushed));
        }
    }

    return EncoderThread::init();
}

void VideoEncoder::cleanup()
{
    Q_ASSERT(m_frameEncoder);

    while (!m_videoFrameQueue.empty())
        processOne();

    while (m_frameEncoder->sendFrame(nullptr) == AVERROR(EAGAIN))
        retrievePackets();
    retrievePackets();
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
    Q_ASSERT(m_frameEncoder);

    retrievePackets();

    FrameInfo frameInfo = takeFrame();
    QVideoFrame &frame = frameInfo.frame;
    Q_ASSERT(frame.isValid());

    //    qCDebug(qLcFFmpegEncoder) << "new video buffer" << frame.startTime();

    AVFrameUPtr avFrame;

    auto *videoBuffer = dynamic_cast<QFFmpegVideoBuffer *>(QVideoFramePrivate::hwBuffer(frame));
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

        // TODO: investigate if we need to set color params to AVFrame.
        //       Setting only codec carameters might be sufficient.
        //       What happens if frame color params are set and not equal codec prms?
        //
        // QVideoFrameFormat format = frame.surfaceFormat();
        // avFrame->color_trc = QFFmpeg::toAvColorTransfer(format.colorTransfer());
        // avFrame->colorspace = QFFmpeg::toAvColorSpace(format.colorSpace());
        // avFrame->color_range = QFFmpeg::toAvColorRange(format.colorRange());

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

    auto [startTime, endTime] = frameTimeStamps(frame);

    if (frameInfo.overriddenPts) {
        startTime = *frameInfo.overriddenPts;
        endTime = startTime + m_frameRateAdapter.frameDuration();
    }

    if (frameInfo.adjustTimeBase) {
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
        qCDebug(qLcFFmpegVideoEncoder) << "error sending frame" << ret << AVError(ret);
        emit m_recordingEngine.sessionError(QMediaRecorder::ResourceError, err2str(ret));
    }
}

bool VideoEncoder::checkIfCanPushFrame() const
{
    if (!isInitialized())
        return m_framesBeforeInit.size() < m_maxFramesBeforeInit;
    if (m_encodingStarted)
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
        // No endTime: advance by an assumed duration. If the frame rate
        // adapter is enabled, these get overridden later
        qreal frameRate = frame.streamFrameRate();
        if (frameRate <= 0.)
            frameRate = DefaultVideoFrameRate;

        Q_ASSERT(frameRate > 0.f);
        endTime = startTime + static_cast<qint64>(std::round(VideoFrameTimeBase / frameRate));
    }

    return { startTime, endTime };
}

} // namespace QFFmpeg

QT_END_NAMESPACE
