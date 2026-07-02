// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmacscreencapturekit_p.h"

#include <QtCore/qmutex.h>

#include <QtFFmpegMediaPluginImpl/private/qcvimagevideobuffer_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegdarwinhwframehelpers_p.h>
#define AVMediaType XAVMediaType
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>
extern "C" {
#include <libavutil/hwcontext_videotoolbox.h>
}
#undef AVMediaType

#include <QtMultimedia/private/qavfcamerautility_p.h>
#include <QtMultimedia/private/qavfhelpers_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include <CoreMedia/CMTime.h>
#include <ScreenCaptureKit/ScreenCaptureKit.h>

#include <chrono>

using namespace Qt::Literals::StringLiterals;

Q_LOGGING_CATEGORY_IMPL(
    QT_PREPEND_NAMESPACE(QFFmpeg::qLcMacScreenCapture),
    "qt.multimedia.screencapture.macscreencapturekit");

namespace {

struct QMacScreenCaptureStreamDelegateHelper : public QObject {
    Q_OBJECT
signals:
    void didStopWithError(int64_t streamId, QString);
};

} // Anonymous namespace

// Events are invoked on system background thread that we don't control.
@implementation QT_MANGLE_NAMESPACE(QMacScreenCaptureStreamDelegate) {
@public
    int64_t m_streamId;
    QMacScreenCaptureStreamDelegateHelper m_helper;
}

- (void)stream:(SCStream *)stream didStopWithError:(NSError *)error
{
    QT_USE_NAMESPACE
    using namespace QFFmpeg;

    emit m_helper.didStopWithError(
        m_streamId,
        QString::fromNSString(error.localizedDescription));
}

@end

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

static void handleFrameOutput(
    QMacScreenCaptureStreamOutput &scStreamOutput,
    CMSampleBufferRef sampleBufferRef);
} // namespace QFFmpeg

QT_END_NAMESPACE

// Invoked on background dispatch-queue.
@implementation QT_MANGLE_NAMESPACE(QMacScreenCaptureStreamOutput) {
@public
    // Assigned at construction. We assume it is safe to never reset it, because
    // we flush the background queue anytime we stop a stream.
    QT_PREPEND_NAMESPACE(QFFmpeg::QMacScreenCaptureKit) *m_qScreenCaptureKit;

    // Used to track when the underlying window size changed, in pixel-coordinates.
    std::optional<QSize> m_previousFrameContentRect;
    std::chrono::microseconds m_startTime;
    std::optional<std::chrono::microseconds> m_baseTime;
    std::unique_ptr<QT_PREPEND_NAMESPACE(QFFmpeg::HWAccel)> m_hwAccel;
}

- (void) stream:(SCStream *) stream
didOutputSampleBuffer:(CMSampleBufferRef) sampleBufferRef
               ofType:(SCStreamOutputType) type
{
    QT_USE_NAMESPACE
    using namespace QFFmpeg;

    // SCStreamOutputTypeScreen implies we are receiving video frames
    // rather than audio samples. It doesn't exclude windows.
    // Our stream is hardcoded to never report audio samples.
    Q_ASSERT(type == SCStreamOutputTypeScreen);

    handleFrameOutput(*self, sampleBufferRef);
}

@end

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

// Useful metadata grabbed from a CMSampleBufferRef.
struct FrameInfo
{
    SCFrameStatus status;

    // The size of the captured content, in pixel-coordinates.
    // Can be used to detect e.g. window size changes.
    QSize contentRect;
};

// Reads the frame status and content rect for the given CMSampleBufferRef.
// The content rect usually means the window size at the time a frame was
// outputted. The resolution is in pixel coordinates.
// Error message is not user-facing.
[[nodiscard]] static q23::expected<FrameInfo, QString> readFrameInfo(CMSampleBufferRef sampleBuffer)
{
    CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, false);
    if (!attachments || CFArrayGetCount(attachments) == 0)
        return q23::unexpected{ u"CMSampleBuffer has no attachments array"_s };

    CFDictionaryRef attachment = (CFDictionaryRef)CFArrayGetValueAtIndex(attachments, 0);
    NSDictionary *dict = (__bridge NSDictionary *)attachment;

    NSNumber *statusNumber = dict[(id)SCStreamFrameInfoStatus];
    if (!statusNumber)
        return q23::unexpected{ u"CMSampleBuffer has no frame status"_s };

    FrameInfo info;
    info.status = static_cast<SCFrameStatus>(statusNumber.intValue);

    CGRect contentRect = CGRectZero;
    NSDictionary *frameInfo = dict[(id)SCStreamFrameInfoContentRect];
    if (frameInfo) {
        contentRect = CGRectMakeWithDictionaryRepresentation(
            (__bridge CFDictionaryRef)frameInfo, &contentRect)
                ? contentRect
                : CGRectZero;
    }

    NSNumber *scaleNumber = dict[(id)SCStreamFrameInfoScaleFactor];
    CGFloat scaleFactor = scaleNumber ? scaleNumber.doubleValue : 1.0;

    NSNumber *contentScaleNumber = dict[(id)SCStreamFrameInfoContentScale];
    CGFloat contentScale = contentScaleNumber ? contentScaleNumber.doubleValue : 1.0;
    if (contentScale <= 0.0)
        contentScale = 1.0;

    info.contentRect = QSize{
        static_cast<int>(std::round(contentRect.size.width * scaleFactor / contentScale)),
        static_cast<int>(std::round(contentRect.size.height * scaleFactor / contentScale)), };

    return info;
}

// Invoked on background dispatch-queue.
// Error message is not user-facing.
[[nodiscard]] static q23::expected<QVideoFrame, QString> createQVideoFrame(
    QMacScreenCaptureStreamOutput &scStreamOutput,
    CMSampleBufferRef sampleBufferRef)
{
    CVImageBufferRef imageBufferRef = CMSampleBufferGetImageBuffer(sampleBufferRef);
    if (!imageBufferRef)
        return q23::unexpected(u"Cannot get CVImageBufferRef from CMSampleBufferRef"_s);
    if (CFGetTypeID(imageBufferRef) != CVPixelBufferGetTypeID())
        return q23::unexpected(u"Grabbed CVImageBufferRef that is not of type CVPixelBuffer"_s);

    auto pixelBuffer = QAVFHelpers::QSharedCVPixelBuffer(
        imageBufferRef,
        QAVFHelpers::QSharedCVPixelBuffer::RefMode::NeedsRef);

    // ScreenCaptureKit hands us buffers from its internal pool (see queueDepth),
    // so copy into a free-standing CVPixelBuffer to decouple the frame's
    // lifetime from the stream's pool.
    q23::expected<QAVFHelpers::QSharedCVPixelBuffer, QString> copyResult = deepCopyCvPixelBuffer(
        pixelBuffer.get());
    if (!copyResult)
        return q23::unexpected(u"Failed to copy incoming pixel buffer: "_s + copyResult.error());
    pixelBuffer = std::move(*copyResult);

    // If the new incoming frames have a different size, update the FFmpeg frames context.
    QSize incomingFrameSize {
        static_cast<int>(CVPixelBufferGetWidth(pixelBuffer.get())),
        static_cast<int>(CVPixelBufferGetHeight(pixelBuffer.get())) };
    Q_ASSERT(!incomingFrameSize.isEmpty());
    CvPixelFormat incomingCvPixelFormat = CVPixelBufferGetPixelFormatType(pixelBuffer.get());
    Q_ASSERT(scStreamOutput.m_hwAccel);
    scStreamOutput.m_hwAccel->updateFramesContext(
        av_map_videotoolbox_format_to_pixfmt(incomingCvPixelFormat),
        incomingFrameSize);

    // TODO: We can extract these values specifically with ScreenCaptureKit.
    std::chrono::microseconds frameTime =
        QAVFHelpers::CMTimeToMicroseconds(CMSampleBufferGetPresentationTimeStamp(sampleBufferRef));
    if (!scStreamOutput.m_baseTime) {
        scStreamOutput.m_baseTime = frameTime;
        scStreamOutput.m_startTime = frameTime;
    }

    QVideoFrameFormat format = QAVFHelpers::videoFormatForImageBuffer(pixelBuffer.get());
    if (!format.isValid())
        return q23::unexpected(u"Cannot get get video format for image buffer"_s);

    format.setColorSpace(QMacScreenCaptureKit::colorSpace);
    format.setColorRange(QMacScreenCaptureKit::colorRange);
    format.setColorTransfer(QMacScreenCaptureKit::colorTransfer);

    Q_ASSERT(scStreamOutput.m_hwAccel);
    QVideoFrame frame;
    q23::expected<QVideoFrame, QString> frameResult = QFFmpeg::qVideoFrameFromCvPixelBuffer(
        *scStreamOutput.m_hwAccel,
        scStreamOutput.m_startTime - *scStreamOutput.m_baseTime,
        pixelBuffer,
        format);
    if (!frameResult)
        qCWarning(qLcMacScreenCapture) << frameResult.error();
    else
        frame = *frameResult;

    if (!frame.isValid()) {
        frame = QVideoFramePrivate::createFrame(
            std::make_unique<QFFmpeg::CVImageVideoBuffer>(std::move(pixelBuffer)),
            std::move(format));
    }

    frame.setStartTime((scStreamOutput.m_startTime - *scStreamOutput.m_baseTime).count());
    frame.setEndTime((frameTime - *scStreamOutput.m_baseTime).count());
    scStreamOutput.m_startTime = frameTime;

    return frame;
}

// Main frame handler.
// Invoked on background dispatch-queue.
static void handleFrameOutput(
    QFFmpeg::QMacScreenCaptureStreamOutput &streamOutput,
    CMSampleBufferRef sampleBufferRef)
{
    using namespace QFFmpeg;

    Q_ASSERT(streamOutput.m_qScreenCaptureKit);

    q23::expected<FrameInfo, QString> frameInfoResult = readFrameInfo(sampleBufferRef);
    if (!frameInfoResult) {
        qCDebug(qLcMacScreenCapture)
            << "Error while reading frame info of CMSampleBufferRef: "
            << frameInfoResult.error();
        return;
    }

    const FrameInfo &frameInfo = *frameInfoResult;

    // ScreenCaptureKit only hands us a new hardware buffer when the frame status
    // is complete. For other statuses (e.g. idle when the captured content is
    if (frameInfo.status != SCFrameStatusComplete)
        return;

    // The content rect is the updated resolution of the window we are capturing.
    // If the window size is different from our current stream configuration,
    // issue a reconfiguration.
    //
    // If the content rect is empty, it's usually an indication that the window has
    // been minimized while capturing it. We keep the stream unchanged so that it
    // is automatically resumed when the window is restored.
    if (!frameInfo.contentRect.isEmpty()) {
        bool newContentRectIsDifferent =
            streamOutput.m_previousFrameContentRect
            && *streamOutput.m_previousFrameContentRect != frameInfo.contentRect;
        if (newContentRectIsDifferent)
            streamOutput.m_qScreenCaptureKit->updateStream(frameInfo.contentRect);

        streamOutput.m_previousFrameContentRect = frameInfo.contentRect;
    }

    q23::expected<QVideoFrame, QString> videoFrameResult = createQVideoFrame(
        streamOutput,
        sampleBufferRef);
    if (!videoFrameResult) {
        qCWarning(qLcMacScreenCapture)
            << "Failed to create qVideoFrame from CMSampleBufferRef: "
            << videoFrameResult.error();
        return;
    }

    emit streamOutput.m_qScreenCaptureKit->newVideoFrameGenerated(
        streamOutput.m_qScreenCaptureKit->streamId(),
        std::move(*videoFrameResult));
}

static void configureStreamDelegate(
    QMacScreenCaptureStreamDelegate &streamDelegate,
    int64_t streamId,
    const QMacScreenCaptureKit &macScreenCaptureKit)
{
    streamDelegate.m_streamId = streamId;
    QObject::connect(
        &streamDelegate.m_helper,
        &QMacScreenCaptureStreamDelegateHelper::didStopWithError,
        &macScreenCaptureKit,
        &QMacScreenCaptureKit::streamStoppedWithError);
}

[[nodiscard]] static q23::expected<AVFScopedPointer<QMacScreenCaptureStreamOutput>, QString>
createStreamOutput(
    QMacScreenCaptureKit &macScreenCaptureKit,
    uint32_t cvPixelFormat,
    QSize resolution)
{
    auto streamOutput = AVFScopedPointer{ [[QMacScreenCaptureStreamOutput alloc] init] };

    streamOutput.data()->m_qScreenCaptureKit = &macScreenCaptureKit;

    streamOutput.data()->m_hwAccel = HWAccel::create(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
    if (!streamOutput.data()->m_hwAccel)
        return q23::unexpected(
            u"Unable to create FFmpeg HW context when starting ScreenCaptureKit stream"_s);

    streamOutput.data()->m_hwAccel->createFramesContext(
        av_map_videotoolbox_format_to_pixfmt(cvPixelFormat),
        resolution);

    if (!streamOutput.data()->m_hwAccel->hwFramesContextAsBuffer())
        return q23::unexpected(
            u"Unable to create FFmpeg HW context when starting ScreenCaptureKit stream"_s);

    return streamOutput;
}

// The strategy is to flush any remaining jobs on the background thread.
QMacScreenCaptureKit::~QMacScreenCaptureKit()
{
    if (!m_stream)
        return;

    // Issue a blocking stop command.
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    [m_stream.data() stopCaptureWithCompletionHandler:[semaphore](NSError *error) {
        if (error) {
            qCWarning(qLcMacScreenCapture)
                << "Error while stopping ScreenCaptureKit stream during teardown: "
                << QString::fromNSString(error.localizedDescription);
        }
        dispatch_semaphore_signal(semaphore);
    }];
    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    dispatch_release(semaphore);

    // Flush the dispatch_queue. After this we assume it's safe to tear everything down.
    if (m_dispatchQueue)
        dispatch_sync(m_dispatchQueue.data(), []{});
}

// This will commonly fail if we are missing permissions for screen capturing.
// It will also open the "Grant permissions" system dialog if we are missing
// permissions.
//
// Thread-safe.
//
// Error-message is not user-facing
std::future<q23::expected<QMacScreenCaptureKit::CapturableItems, QString>>
QMacScreenCaptureKit::enumerateCapturableItems()
{
    // Block functions can only capture copyable types.
    // Wrap the promise in a shared-ptr.
    auto promise = std::make_shared<std::promise<q23::expected<CapturableItems, QString>>>();

    // This function call will open the permissions system dialog when applicable.
    [SCShareableContent getShareableContentWithCompletionHandler:^(
        SCShareableContent *shareableContent,
        NSError *error)
    {
        if (error != nil) {
            promise->set_value(q23::unexpected(QString::fromNSString(error.localizedDescription)));
            return;
        }

        QMacScreenCaptureKit::CapturableItems output;

        output.displays.reserve(shareableContent.displays.count);
        for (SCDisplay *item in shareableContent.displays)
            output.displays.push_back(AVFScopedPointer{ [item retain] });

        output.windows.reserve(shareableContent.windows.count);
        for (SCWindow *item in shareableContent.windows)
            output.windows.push_back(AVFScopedPointer{ [item retain] });

        promise->set_value(std::move(output));
    }];

    return promise->get_future();
}

// Note that we are using manual memory management here, because Obj-C block functions
// do not support capturing move-only types.
std::future<q23::expected<std::unique_ptr<QMacScreenCaptureKit>, QString>>
QMacScreenCaptureKit::createStreamFromFilter(
    int64_t streamId,
    SCContentFilter *scContentFilter,
    QSize resolutionPx,
    std::optional<qreal> frameRate,
    std::function<void(QMacScreenCaptureKit&)> const &connectionSetup)
{
    using ResultType = q23::expected<std::unique_ptr<QMacScreenCaptureKit>, QString>;

    auto promise = std::make_shared<std::promise<ResultType>>();
    auto future = promise->get_future();

    AVFScopedPointer<SCStreamConfiguration> scStreamConfig =
        QMacScreenCaptureKit::createStreamConfig(resolutionPx, frameRate);

    auto captureKit = std::make_unique<QMacScreenCaptureKit>();
    captureKit->m_streamId = streamId;
    captureKit->m_frameRate = frameRate;
    if (connectionSetup) {
        connectionSetup(*captureKit);
    }

    q23::expected<AVFScopedPointer<QMacScreenCaptureStreamOutput>, QString> streamOutputResult =
        createStreamOutput(
            *captureKit,
            scStreamConfig.data().pixelFormat,
            resolutionPx);
    if (!streamOutputResult) {
        promise->set_value(q23::unexpected(std::move(streamOutputResult.error())));
        return future;
    }
    AVFScopedPointer<QMacScreenCaptureStreamOutput> &streamOutput = *streamOutputResult;

    auto streamDelegate = AVFScopedPointer{ [[QMacScreenCaptureStreamDelegate alloc] init] };
    configureStreamDelegate(*streamDelegate.data(), streamId, *captureKit);

    auto scStream = AVFScopedPointer { [[SCStream alloc]
        initWithFilter:scContentFilter
         configuration:scStreamConfig
              delegate:streamDelegate] };

    auto queue = AVFScopedPointer<dispatch_queue_t>{
        dispatch_queue_create("qt_screencapture", DISPATCH_QUEUE_SERIAL) };

    NSError *addStreamError = nullptr;
    [scStream addStreamOutput:streamOutput
                         type:SCStreamOutputTypeScreen
           sampleHandlerQueue:queue
                        error:&addStreamError];
    if (addStreamError != nil) {
        promise->set_value(q23::unexpected(u"Unable to add stream output to SCStream"_s));
        return future;
    }

    captureKit->m_stream = std::move(scStream);
    captureKit->m_dispatchQueue = std::move(queue);
    captureKit->m_streamDelegate = std::move(streamDelegate);
    captureKit->m_streamOutput = std::move(streamOutput);

    // Block functions for the completion handler require
    // that the callable is copyable. This means we can't capture
    // move-only types. So we temporarily release the unique_ptr here,
    // and switch to manual memory management and then adopt them
    // back into AVFScopedPointer inside the callback.
    // We assume the completion handler is always called, either with success or error.
    [captureKit->m_stream startCaptureWithCompletionHandler: [
        captureKitTemp = captureKit.release(),
        promise = std::move(promise)]
        (NSError *error)
    {
        auto captureKit = std::unique_ptr<QMacScreenCaptureKit>{ captureKitTemp };

        if (error != nil) {
            promise->set_value(q23::unexpected{ u"Error when starting screen capturing stream"_s });
            return;
        }

        promise->set_value(std::move(captureKit));
    }];

    return future;
}

// Frames may arrive on the background thread immediately after the
// creation success event has been emitted, sometimes even out of order.
// This function takes a function that allows us to establish
// the connections on the newly constructed object before
// the stream ever starts, so we never miss any frames.
std::future<q23::expected<std::unique_ptr<QMacScreenCaptureKit>, QString>>
QMacScreenCaptureKit::createStreamFromWindow(
    int64_t streamId,
    SCWindow *scWindow,
    std::optional<qreal> frameRate,
    std::function<void(QMacScreenCaptureKit&)> const &connectionSetup)
{
    Q_ASSERT(scWindow);

    auto scContentFilter = AVFScopedPointer<SCContentFilter>{ [[SCContentFilter alloc]
        initWithDesktopIndependentWindow: scWindow] };

    float pointPixelScale = scContentFilter.data().pointPixelScale;

    // SCWindow.frame is in screen-points, not pixels. Multiply by
    // pointPixelScale.
    return createStreamFromFilter(
        streamId,
        scContentFilter,
        QSize {
            static_cast<int>(std::round(scWindow.frame.size.width * pointPixelScale)),
            static_cast<int>(std::round(scWindow.frame.size.height * pointPixelScale)) },
        frameRate,
        connectionSetup);
}

AVFScopedPointer<SCStreamConfiguration> QMacScreenCaptureKit::createStreamConfig(
    QSize resolutionPx,
    std::optional<qreal> frameRate)
{
    // SCStreamConfiguration defines the output format, having zero resolution makes no sense.
    Q_ASSERT(!resolutionPx.isEmpty());

    // TODO: Possible improvements include specifying pixel format, HDR,
    // capturing system audio...
    auto scStreamConfig = AVFScopedPointer{ [[SCStreamConfiguration alloc] init] };
    scStreamConfig.data().width = resolutionPx.width();
    scStreamConfig.data().height = resolutionPx.height();
    // We make a best-effort to always adjust our video output to match the window/screen size.
    // So we leave scaling off to be pixel-perfect whenever we can.
    scStreamConfig.data().scalesToFit = false;
    scStreamConfig.data().queueDepth = QMacScreenCaptureKit::queueDepth;
    scStreamConfig.data().pixelFormat = QMacScreenCaptureKit::cvPixelFormat;
    scStreamConfig.data().colorSpaceName = QMacScreenCaptureKit::cgColorSpace();
    scStreamConfig.data().captureResolution = SCCaptureResolutionBest;
    if (@available(macOS 15.0, *))
        scStreamConfig.data().captureDynamicRange = SCCaptureDynamicRangeSDR;

    if (frameRate) {
        Q_ASSERT(frameRate > 0);
        scStreamConfig.data().minimumFrameInterval =
            CMTimeMake(1, static_cast<int32_t>(std::round(*frameRate)));
    } else {
        scStreamConfig.data().minimumFrameInterval = kCMTimeZero;
    }

    return scStreamConfig;
}

// Issues a stream configuration update, so that the stream will give us video frames
// of a new resolution.
void QMacScreenCaptureKit::startStreamReconfigure(
    SCStream *scStream,
    QSize resolutionPx,
    std::optional<qreal> frameRate)
{
    Q_ASSERT(scStream);

    AVFScopedPointer<SCStreamConfiguration> scStreamConfig =
        QMacScreenCaptureKit::createStreamConfig(resolutionPx, frameRate);

    [scStream
        updateConfiguration:scStreamConfig.data()
        completionHandler:[](NSError *err) {
            if (err) {
                // TODO: Send potential error back to QMacScreenCaptureKit, but only
                // if the error stops the stream.
                qCWarning(qLcMacScreenCapture)
                    << "Error when reconfiguring ScreenCaptureKit stream: "
                    << QString::fromNSString(err.description);
                return;
            }
        }];
}

// Reconfigures the stream with a new output resolution.
// Does not stop the stream.
// Input resolution is in pixel-coordinates.
// Must be called from background dispatch_queue.
void QMacScreenCaptureKit::updateStream(QSize resolutionPx)
{
    Q_ASSERT(m_dispatchQueue);
    dispatch_assert_queue(m_dispatchQueue.data());

    startStreamReconfigure(m_stream.data(), resolutionPx, m_frameRate);
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qmacscreencapturekit_p.cpp"
#include "qmacscreencapturekit.moc"
