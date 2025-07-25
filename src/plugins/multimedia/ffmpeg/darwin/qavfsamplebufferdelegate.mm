// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qavfsamplebufferdelegate_p.h>

#include <QtMultimedia/private/qavfhelpers_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include <QtFFmpegMediaPluginImpl/private/qcvimagevideobuffer_p.h>
#define AVMediaType XAVMediaType
#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#undef AVMediaType

#include <optional>

QT_USE_NAMESPACE

// Make sure this is compatible with the layout used in ffmpeg's hwcontext_videotoolbox
static QFFmpeg::AVFrameUPtr allocHWFrame(
    AVBufferRef *hwContext,
    QAVFHelpers::QSharedCVPixelBuffer sharedPixBuf)
{
    Q_ASSERT(sharedPixBuf);

    AVHWFramesContext *ctx = (AVHWFramesContext *)hwContext->data;
    auto frame = QFFmpeg::makeAVFrame();
    frame->hw_frames_ctx = av_buffer_ref(hwContext);
    frame->extended_data = frame->data;

    CVPixelBufferRef pixbuf = sharedPixBuf.release();
    auto releasePixBufFn = [](void* opaquePtr, uint8_t *) {
        CVPixelBufferRelease(static_cast<CVPixelBufferRef>(opaquePtr));
    };
    frame->buf[0] = av_buffer_create(nullptr, 0, releasePixBufFn, pixbuf, 0);

    // It is convention to use 4th data plane for hardware frames.
    frame->data[3] = (uint8_t *)pixbuf;
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
    QFFmpeg::QAVFSampleBufferDelegateTransformProvider transformationProvider;
    AVBufferRef *hwFramesContext;
    std::unique_ptr<QFFmpeg::HWAccel> m_accel;
    qint64 startTime;
    std::optional<qint64> baseTime;
    qreal frameRate;
}

static QVideoFrame createHwVideoFrame(
    QAVFSampleBufferDelegate &delegate,
    const QAVFHelpers::QSharedCVPixelBuffer &imageBuffer,
    QVideoFrameFormat format)
{
    Q_ASSERT(delegate.baseTime);

    if (!delegate.m_accel)
        return {};

    auto avFrame = allocHWFrame(
        delegate.m_accel->hwFramesContextAsBuffer(),
        imageBuffer);
    if (!avFrame)
        return {};

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

    avFrame->pts = delegate.startTime - *delegate.baseTime;

    return QVideoFramePrivate::createFrame(std::make_unique<QFFmpegVideoBuffer>(std::move(avFrame)),
                                           format);
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

- (void)discardFutureSamples
{
    frameHandler = nullptr;
}

- (void)setTransformationProvider:
    (const QFFmpeg::QAVFSampleBufferDelegateTransformProvider &)provider
{
    transformationProvider = std::move(provider);
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
               fromConnection:(AVCaptureConnection *)connection
{
    Q_UNUSED(captureOutput);

    if (!frameHandler)
        return;

    // NB: on iOS captureOutput/connection can be nil (when recording a video -
    // avfmediaassetwriter).

    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!imageBuffer || CFGetTypeID(imageBuffer) != CVPixelBufferGetTypeID()) {
        qWarning() << "Cannot get image buffer from sample buffer";
        return;
    }

    auto pixelBuffer = QAVFHelpers::QSharedCVPixelBuffer(
        imageBuffer,
        QAVFHelpers::QSharedCVPixelBuffer::RefMode::NeedsRef);

    const CMTime time = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    const qint64 frameTime = time.timescale ? time.value * 1000000 / time.timescale : 0;
    if (!baseTime) {
        baseTime = frameTime;
        startTime = frameTime;
    }

    QVideoFrameFormat format = QAVFHelpers::videoFormatForImageBuffer(pixelBuffer.get());
    if (!format.isValid()) {
        qWarning() << "Cannot get get video format for image buffer"
                   << CVPixelBufferGetWidth(pixelBuffer.get()) << 'x'
                   << CVPixelBufferGetHeight(pixelBuffer.get());
        return;
    }

    std::optional<QFFmpeg::QAVFSampleBufferDelegateTransform> transform;
    if (transformationProvider) {
        transform = transformationProvider(connection);
        const VideoTransformation &surfaceTransform = transform.value().surfaceTransform;
        format.setRotation(surfaceTransform.rotation);
        format.setMirrored(surfaceTransform.mirroredHorizontallyAfterRotation);
    }

    format.setStreamFrameRate(frameRate);

    auto frame = createHwVideoFrame(*self, pixelBuffer, format);
    if (!frame.isValid())
        frame = QVideoFramePrivate::createFrame(
            std::make_unique<QFFmpeg::CVImageVideoBuffer>(std::move(pixelBuffer)),
            std::move(format));

    if (transform.has_value()) {
        const VideoTransformation &presentationTransform = transform.value().presentationTransform;
        frame.setRotation(presentationTransform.rotation);
        frame.setMirrored(presentationTransform.mirroredHorizontallyAfterRotation);
    }

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
