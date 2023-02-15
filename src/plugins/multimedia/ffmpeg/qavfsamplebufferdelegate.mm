// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qavfsamplebufferdelegate_p.h"

#define AVMediaType XAVMediaType

#include "qffmpeghwaccel_p.h"
#include "qavfhelpers_p.h"
#include "qffmpegvideobuffer_p.h"

#include <optional>

#undef AVMediaType

QT_USE_NAMESPACE

static void releaseHwFrame(void * /*opaque*/, uint8_t *data)
{
    CVPixelBufferRelease(CVPixelBufferRef(data));
}

// Make sure this is compatible with the layout used in ffmpeg's hwcontext_videotoolbox
static QFFmpeg::AVFrameUPtr allocHWFrame(AVBufferRef *hwContext, const CVPixelBufferRef &pixbuf)
{
    AVHWFramesContext *ctx = (AVHWFramesContext *)hwContext->data;
    auto frame = QFFmpeg::makeAVFrame();
    frame->hw_frames_ctx = av_buffer_ref(hwContext);
    frame->extended_data = frame->data;

    frame->buf[0] = av_buffer_create((uint8_t *)pixbuf, 1, releaseHwFrame, NULL, 0);
    frame->data[3] = (uint8_t *)pixbuf;
    CVPixelBufferRetain(pixbuf);
    frame->width = ctx->width;
    frame->height = ctx->height;
    frame->format = AV_PIX_FMT_VIDEOTOOLBOX;
    if (frame->width != (int)CVPixelBufferGetWidth(pixbuf)
        || frame->height != (int)CVPixelBufferGetHeight(pixbuf)) {

        // This can happen while changing camera format
        return nullptr;
    }
    return frame;
}

@implementation QAVFSampleBufferDelegate {
@private
    std::function<void(const QVideoFrame &)> frameHandler;
    AVBufferRef *hwFramesContext;
    std::unique_ptr<QFFmpeg::HWAccel> m_accel;
    qint64 startTime;
    std::optional<qint64> baseTime;
    qreal frameRate;
}

- (instancetype)initWithFrameHandler:(std::function<void(const QVideoFrame &)>)handler
{
    if (!(self = [super init]))
        return nil;

    Q_ASSERT(handler);

    frameHandler = std::move(handler);
    hwFramesContext = nullptr;
    startTime = 0;
    frameRate = 0.;
    return self;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
               fromConnection:(AVCaptureConnection *)connection
{
    Q_UNUSED(connection);
    Q_UNUSED(captureOutput);

    // NB: on iOS captureOutput/connection can be nil (when recording a video -
    // avfmediaassetwriter).

    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);

    const CMTime time = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    const qint64 frameTime = time.timescale ? time.value * 1000000 / time.timescale : 0;
    if (!baseTime) {
        // drop the first frame to get a valid frame start time
        baseTime = frameTime;
        startTime = frameTime;
        return;
    }

    if (!m_accel)
        return;

    auto avFrame = allocHWFrame(m_accel->hwFramesContextAsBuffer(), imageBuffer);
    if (!avFrame)
        return;

#ifdef USE_SW_FRAMES
    {
        auto swFrame = QFFmpeg::makeAVFrame();
        /* retrieve data from GPU to CPU */
        const int ret = av_hwframe_transfer_data(swFrame.get(), avFrame.get(), 0);
        if (ret < 0) {
            qWarning() << "Error transferring the data to system memory:" << ret;
        } else {
            avFrame = std::move(swFrame);
        }
    }
#endif

    QVideoFrameFormat format = QAVFHelpers::videoFormatForImageBuffer(imageBuffer);
    if (!format.isValid()) {
        return;
    }

    format.setFrameRate(frameRate);

    avFrame->pts = startTime - *baseTime;

    QFFmpegVideoBuffer *buffer = new QFFmpegVideoBuffer(std::move(avFrame));
    QVideoFrame frame(buffer, format);
    frame.setStartTime(startTime - *baseTime);
    frame.setEndTime(frameTime - *baseTime);
    startTime = frameTime;

    frameHandler(frame);
}

- (void)setHWAccel:(std::unique_ptr<QFFmpeg::HWAccel> &&)accel
{
    m_accel = std::move(accel);
}

- (void)setVideoFormatFrameRate:(qreal)rate
{
    frameRate = rate;
}

@end
