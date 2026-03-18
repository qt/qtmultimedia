// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qavfsamplebufferdelegate_p.h>

#define AVMediaType XAVMediaType
extern "C" {
#include <libavutil/hwcontext_videotoolbox.h>
} // extern "C"
#undef AVMediaType

#include <QtCore/qsize.h>

#include <QtMultimedia/private/qavfhelpers_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include <QtFFmpegMediaPluginImpl/private/qcvimagevideobuffer_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegdarwinhwframehelpers_p.h>
#define AVMediaType XAVMediaType
#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#undef AVMediaType

#include <chrono>
#include <optional>

using namespace Qt::StringLiterals;

QT_USE_NAMESPACE

@implementation QT_MANGLE_NAMESPACE(QAVFSampleBufferDelegate) {
@private
    std::function<void(const QVideoFrame &)> frameHandler;
    QFFmpeg::QAVFSampleBufferDelegateTransformProvider transformationProvider;
    std::unique_ptr<QFFmpeg::HWAccel> m_accel;
    std::chrono::microseconds startTime;
    std::optional<std::chrono::microseconds> baseTime;
    qreal frameRate;
}

- (instancetype)initWithFrameHandler:(std::function<void(const QVideoFrame &)>)handler
{
    if (!(self = [super init]))
        return nil;

    Q_ASSERT(handler);

    frameHandler = std::move(handler);
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

    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!imageBuffer || CFGetTypeID(imageBuffer) != CVPixelBufferGetTypeID()) {
        qWarning() << "Cannot get image buffer from sample buffer";
        return;
    }

    auto pixelBuffer = QAVFHelpers::QSharedCVPixelBuffer(
        imageBuffer,
        QAVFHelpers::QSharedCVPixelBuffer::RefMode::NeedsRef);

    QSize incomingFrameSize {
        static_cast<int>(CVPixelBufferGetWidth(pixelBuffer.get())),
        static_cast<int>(CVPixelBufferGetHeight(pixelBuffer.get())) };
    Q_ASSERT(!incomingFrameSize.isEmpty());
    CvPixelFormat incomingCvPixelFormat = CVPixelBufferGetPixelFormatType(pixelBuffer.get());

    Q_ASSERT(m_accel);
    m_accel->updateFramesContext(
        av_map_videotoolbox_format_to_pixfmt(incomingCvPixelFormat),
        incomingFrameSize);

    std::chrono::microseconds frameTime =
        QAVFHelpers::CMTimeToMicroseconds(CMSampleBufferGetPresentationTimeStamp(sampleBuffer));
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

    QVideoFrame frame = QFFmpeg::qVideoFrameFromCvPixelBuffer(
        *m_accel,
        startTime - *baseTime,
        pixelBuffer,
        format);
    if (!frame.isValid())
        frame = QVideoFramePrivate::createFrame(
            std::make_unique<QFFmpeg::CVImageVideoBuffer>(std::move(pixelBuffer)),
            std::move(format));

    if (transform.has_value()) {
        const VideoTransformation &presentationTransform = transform.value().presentationTransform;
        frame.setRotation(presentationTransform.rotation);
        frame.setMirrored(presentationTransform.mirroredHorizontallyAfterRotation);
    }

    frame.setStartTime((startTime - *baseTime).count());
    frame.setEndTime((frameTime - *baseTime).count());
    startTime = frameTime;

    frameHandler(frame);
}

// Sets the initial HWAccel. Should only be called once during
// initialization.
- (void)setHWAccel:(std::unique_ptr<QFFmpeg::HWAccel> &&)accel
{
    m_accel = std::move(accel);
}

- (void)setVideoFormatFrameRate:(qreal)rate
{
    frameRate = rate;
}

@end
