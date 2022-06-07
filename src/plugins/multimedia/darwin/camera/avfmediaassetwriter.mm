// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfmediaencoder_p.h"
#include "avfcamerarenderer_p.h"
#include "avfmediaassetwriter_p.h"
#include "avfcameraservice_p.h"
#include "avfcamerasession_p.h"
#include "avfcameradebug_p.h"
#include <qdarwinformatsinfo_p.h>
#include <avfmetadata_p.h>

#include <QtCore/qmetaobject.h>
#include <QtCore/qatomic.h>

QT_USE_NAMESPACE

namespace {

bool qt_capture_session_isValid(AVFCameraService *service)
{
    if (!service || !service->session())
        return false;

    AVFCameraSession *session = service->session();
    if (!session->captureSession())
        return false;

    if (!session->videoInput() && !session->audioInput())
        return false;

    return true;
}

enum WriterState
{
    WriterStateIdle,
    WriterStateActive,
    WriterStatePaused,
    WriterStateAborted
};

using AVFAtomicInt64 = QAtomicInteger<qint64>;

} // unnamed namespace

@interface QT_MANGLE_NAMESPACE(AVFMediaAssetWriter) (PrivateAPI)
- (bool)addWriterInputs;
- (void)setQueues;
- (void)updateDuration:(CMTime)newTimeStamp;
- (CMSampleBufferRef)adjustTime:(CMSampleBufferRef)sample by:(CMTime)offset;
@end

@implementation QT_MANGLE_NAMESPACE(AVFMediaAssetWriter)
{
@private
    AVFCameraService *m_service;

    AVFScopedPointer<AVAssetWriterInput> m_cameraWriterInput;
    AVFScopedPointer<AVAssetWriterInput> m_audioWriterInput;

    // Queue to write sample buffers:
    AVFScopedPointer<dispatch_queue_t> m_writerQueue;
    // High priority serial queue for video output:
    AVFScopedPointer<dispatch_queue_t> m_videoQueue;
    // Serial queue for audio output:
    AVFScopedPointer<dispatch_queue_t> m_audioQueue;

    AVFScopedPointer<AVAssetWriter> m_assetWriter;

    AVFMediaEncoder *m_delegate;

    bool m_setStartTime;

    QAtomicInt m_state;

    bool m_writeFirstAudioBuffer;

    CMTime m_startTime;
    CMTime m_lastTimeStamp;
    CMTime m_lastVideoTimestamp;
    CMTime m_lastAudioTimestamp;
    CMTime m_timeOffset;
    bool m_adjustTime;

    NSDictionary *m_audioSettings;
    NSDictionary *m_videoSettings;

    AVFAtomicInt64 m_durationInMs;
}

- (id)initWithDelegate:(AVFMediaEncoder *)delegate
{
    Q_ASSERT(delegate);

    if (self = [super init]) {
        m_delegate = delegate;
        m_setStartTime = true;
        m_state.storeRelaxed(WriterStateIdle);
        m_startTime = kCMTimeInvalid;
        m_lastTimeStamp = kCMTimeInvalid;
        m_lastAudioTimestamp = kCMTimeInvalid;
        m_lastVideoTimestamp = kCMTimeInvalid;
        m_timeOffset = kCMTimeInvalid;
        m_adjustTime = false;
        m_durationInMs.storeRelaxed(0);
        m_audioSettings = nil;
        m_videoSettings = nil;
        m_writeFirstAudioBuffer = false;
    }

    return self;
}

- (bool)setupWithFileURL:(NSURL *)fileURL
        cameraService:(AVFCameraService *)service
        audioSettings:(NSDictionary *)audioSettings
        videoSettings:(NSDictionary *)videoSettings
        fileFormat:(QMediaFormat::FileFormat)fileFormat
        transform:(CGAffineTransform)transform
{
    Q_ASSERT(fileURL);

    if (!qt_capture_session_isValid(service)) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "invalid capture session";
        return false;
    }

    m_service = service;
    m_audioSettings = audioSettings;
    m_videoSettings = videoSettings;

    AVFCameraSession *session = m_service->session();

    m_writerQueue.reset(dispatch_queue_create("asset-writer-queue", DISPATCH_QUEUE_SERIAL));
    if (!m_writerQueue) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to create an asset writer's queue";
        return false;
    }

    m_videoQueue.reset();
    if (session->videoInput() && session->videoOutput() && session->videoOutput()->videoDataOutput()) {
        m_videoQueue.reset(dispatch_queue_create("video-output-queue", DISPATCH_QUEUE_SERIAL));
        if (!m_videoQueue) {
            qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to create video queue";
            return false;
        }
        dispatch_set_target_queue(m_videoQueue, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0));
    }

    m_audioQueue.reset();
    if (session->audioInput() && session->audioOutput()) {
        m_audioQueue.reset(dispatch_queue_create("audio-output-queue", DISPATCH_QUEUE_SERIAL));
        if (!m_audioQueue) {
            qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to create audio queue";
            if (!m_videoQueue)
                return false;
            // But we still can write video!
        }
    }

    auto fileType = QDarwinFormatInfo::avFileTypeForContainerFormat(fileFormat);
    m_assetWriter.reset([[AVAssetWriter alloc] initWithURL:fileURL
                                               fileType:fileType
                                               error:nil]);
    if (!m_assetWriter) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to create asset writer";
        return false;
    }

    if (!m_videoQueue)
        m_writeFirstAudioBuffer = true;

    if (![self addWriterInputs]) {
        m_assetWriter.reset();
        return false;
    }

    if (m_cameraWriterInput)
        m_cameraWriterInput.data().transform = transform;

    [self setMetaData:fileType];

    // Ready to start ...
    return true;
}

- (void)setMetaData:(AVFileType)fileType
{
    m_assetWriter.data().metadata = AVFMetaData::toAVMetadataForFormat(m_delegate->metaData(), fileType);
}

- (void)start
{
    [self setQueues];

    m_setStartTime = true;

    m_state.storeRelease(WriterStateActive);

    [m_assetWriter startWriting];
    AVCaptureSession *session = m_service->session()->captureSession();
    if (!session.running)
        [session startRunning];
}

- (void)stop
{
    if (m_state.loadAcquire() != WriterStateActive && m_state.loadAcquire() != WriterStatePaused)
        return;

    if ([m_assetWriter status] != AVAssetWriterStatusWriting
        && [m_assetWriter status] != AVAssetWriterStatusFailed)
        return;

    // Do this here so that -
    // 1. '-abort' should not try calling finishWriting again and
    // 2. async block (see below) will know if recorder control was deleted
    //    before the block's execution:
    m_state.storeRelease(WriterStateIdle);
    // Now, since we have to ensure no sample buffers are
    // appended after a call to finishWriting, we must
    // ensure writer's queue sees this change in m_state
    // _before_ we call finishWriting:
    dispatch_sync(m_writerQueue, ^{});
    // Done, but now we also want to prevent video queue
    // from updating our viewfinder:
    if (m_videoQueue)
        dispatch_sync(m_videoQueue, ^{});

    // Now we're safe to stop:
    [m_assetWriter finishWritingWithCompletionHandler:^{
        // This block is async, so by the time it's executed,
        // it's possible that render control was deleted already ...
        if (m_state.loadAcquire() == WriterStateAborted)
            return;

        AVCaptureSession *session = m_service->session()->captureSession();
        if (session.running)
            [session stopRunning];
        QMetaObject::invokeMethod(m_delegate, "assetWriterFinished", Qt::QueuedConnection);
    }];
}

- (void)abort
{
    // -abort is to be called from recorder control's dtor.

    if (m_state.fetchAndStoreRelease(WriterStateAborted) != WriterStateActive) {
        // Not recording, nothing to stop.
        return;
    }

    // From Apple's docs:
    // "To guarantee that all sample buffers are successfully written,
    //  you must ensure that all calls to appendSampleBuffer: and
    //  appendPixelBuffer:withPresentationTime: have returned before
    //  invoking this method."
    //
    // The only way we can ensure this is:
    dispatch_sync(m_writerQueue, ^{});
    // At this point next block (if any) on the writer's queue
    // will see m_state preventing it from any further processing.
    if (m_videoQueue)
        dispatch_sync(m_videoQueue, ^{});
    // After this point video queue will not try to modify our
    // viewfider, so we're safe to delete now.

    [m_assetWriter finishWritingWithCompletionHandler:^{
    }];
}

- (void)pause
{
    if (m_state.loadAcquire() != WriterStateActive)
        return;
    if ([m_assetWriter status] != AVAssetWriterStatusWriting)
        return;

    m_state.storeRelease(WriterStatePaused);
    m_adjustTime = true;
}

- (void)resume
{
    if (m_state.loadAcquire() != WriterStatePaused)
        return;
    if ([m_assetWriter status] != AVAssetWriterStatusWriting)
        return;

    m_state.storeRelease(WriterStateActive);
}

- (void)setStartTimeFrom:(CMSampleBufferRef)sampleBuffer
{
    // Writer's queue only.
    Q_ASSERT(m_setStartTime);
    Q_ASSERT(sampleBuffer);

    if (m_state.loadAcquire() != WriterStateActive)
        return;

    QMetaObject::invokeMethod(m_delegate, "assetWriterStarted", Qt::QueuedConnection);

    m_durationInMs.storeRelease(0);
    m_startTime = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    m_lastTimeStamp = m_startTime;
    [m_assetWriter startSessionAtSourceTime:m_startTime];
    m_setStartTime = false;
}

- (CMSampleBufferRef)adjustTime:(CMSampleBufferRef)sample by:(CMTime)offset
{
    CMItemCount count;
    CMSampleBufferGetSampleTimingInfoArray(sample, 0, nil, &count);
    CMSampleTimingInfo* timingInfo = (CMSampleTimingInfo*) malloc(sizeof(CMSampleTimingInfo) * count);
    CMSampleBufferGetSampleTimingInfoArray(sample, count, timingInfo, &count);
    for (CMItemCount i = 0; i < count; i++)
    {
        timingInfo[i].decodeTimeStamp = CMTimeSubtract(timingInfo[i].decodeTimeStamp, offset);
        timingInfo[i].presentationTimeStamp = CMTimeSubtract(timingInfo[i].presentationTimeStamp, offset);
    }
    CMSampleBufferRef updatedBuffer;
    CMSampleBufferCreateCopyWithNewTiming(kCFAllocatorDefault, sample, count, timingInfo, &updatedBuffer);
    free(timingInfo);
    return updatedBuffer;
}

- (void)writeVideoSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    // This code is executed only on a writer's queue.
    Q_ASSERT(sampleBuffer);

    if (m_state.loadAcquire() == WriterStateActive) {
        if (m_setStartTime)
            [self setStartTimeFrom:sampleBuffer];

        if (m_cameraWriterInput.data().readyForMoreMediaData) {
            [self updateDuration:CMSampleBufferGetPresentationTimeStamp(sampleBuffer)];
            [m_cameraWriterInput appendSampleBuffer:sampleBuffer];
        }
    }
}

- (void)writeAudioSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    Q_ASSERT(sampleBuffer);

    // This code is executed only on a writer's queue.
    if (m_state.loadAcquire() == WriterStateActive) {
        if (m_setStartTime)
            [self setStartTimeFrom:sampleBuffer];

        if (m_audioWriterInput.data().readyForMoreMediaData) {
            [self updateDuration:CMSampleBufferGetPresentationTimeStamp(sampleBuffer)];
            [m_audioWriterInput appendSampleBuffer:sampleBuffer];
        }
    }
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection *)connection
{
    Q_UNUSED(connection);
    Q_ASSERT(m_service && m_service->session());

    if (m_state.loadAcquire() != WriterStateActive && m_state.loadAcquire() != WriterStatePaused)
        return;

    if ([m_assetWriter status] != AVAssetWriterStatusWriting) {
        if ([m_assetWriter status] == AVAssetWriterStatusFailed) {
            NSError *error = [m_assetWriter error];
            NSString *failureReason = error.localizedFailureReason;
            NSString *suggestion = error.localizedRecoverySuggestion;
            NSString *errorString = suggestion ? [failureReason stringByAppendingString:suggestion] : failureReason;
            QMetaObject::invokeMethod(m_delegate, "assetWriterError",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, QString::fromNSString(errorString)));
        }
        return;
    }

    if (!CMSampleBufferDataIsReady(sampleBuffer)) {
        qWarning() << Q_FUNC_INFO << "sample buffer is not ready, skipping.";
        return;
    }

    CFRetain(sampleBuffer);

    bool isVideoBuffer = true;
    isVideoBuffer = (captureOutput != m_service->session()->audioOutput());
    if (isVideoBuffer) {
        // Find renderercontrol's delegate and invoke its method to
        // show updated viewfinder's frame.
        if (m_service->session()->videoOutput()) {
            NSObject<AVCaptureVideoDataOutputSampleBufferDelegate> *vfDelegate =
                (NSObject<AVCaptureVideoDataOutputSampleBufferDelegate> *)m_service->session()->videoOutput()->captureDelegate();
            if (vfDelegate) {
                AVCaptureOutput *output = nil;
                AVCaptureConnection *connection = nil;
                [vfDelegate captureOutput:output didOutputSampleBuffer:sampleBuffer fromConnection:connection];
            }
        }
    } else {
        if (m_service->session()->audioOutput()) {
            NSObject<AVCaptureAudioDataOutputSampleBufferDelegate> *audioPreviewDelegate =
                (NSObject<AVCaptureAudioDataOutputSampleBufferDelegate> *)m_service->session()->audioPreviewDelegate();
            if (audioPreviewDelegate) {
                AVCaptureOutput *output = nil;
                AVCaptureConnection *connection = nil;
                [audioPreviewDelegate captureOutput:output didOutputSampleBuffer:sampleBuffer fromConnection:connection];
            }
        }
    }

    if (m_state.loadAcquire() != WriterStateActive) {
        CFRelease(sampleBuffer);
        return;
    }

    if (m_adjustTime) {
        CMTime currentTimestamp = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
        CMTime lastTimestamp = isVideoBuffer ? m_lastVideoTimestamp : m_lastAudioTimestamp;

        if (!CMTIME_IS_INVALID(lastTimestamp)) {
            if (!CMTIME_IS_INVALID(m_timeOffset))
                currentTimestamp = CMTimeSubtract(currentTimestamp, m_timeOffset);

            CMTime pauseDuration = CMTimeSubtract(currentTimestamp, lastTimestamp);

            if (m_timeOffset.value == 0)
                m_timeOffset = pauseDuration;
            else
                m_timeOffset = CMTimeAdd(m_timeOffset, pauseDuration);
        }
        m_lastVideoTimestamp = kCMTimeInvalid;
        m_adjustTime = false;
    }

    if (m_timeOffset.value > 0) {
        CFRelease(sampleBuffer);
        sampleBuffer = [self adjustTime:sampleBuffer by:m_timeOffset];
    }

    CMTime currentTimestamp = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    CMTime currentDuration = CMSampleBufferGetDuration(sampleBuffer);
    if (currentDuration.value > 0)
        currentTimestamp = CMTimeAdd(currentTimestamp, currentDuration);

    if (isVideoBuffer)
    {
        m_lastVideoTimestamp = currentTimestamp;
        dispatch_async(m_writerQueue, ^{
            [self writeVideoSampleBuffer:sampleBuffer];
            m_writeFirstAudioBuffer = true;
            CFRelease(sampleBuffer);
        });
    } else if (m_writeFirstAudioBuffer) {
        m_lastAudioTimestamp = currentTimestamp;
        dispatch_async(m_writerQueue, ^{
            [self writeAudioSampleBuffer:sampleBuffer];
            CFRelease(sampleBuffer);
        });
    }
}

- (bool)addWriterInputs
{
    Q_ASSERT(m_service && m_service->session());
    Q_ASSERT(m_assetWriter.data());

    AVFCameraSession *session = m_service->session();

    m_cameraWriterInput.reset();
    if (m_videoQueue)
    {
        Q_ASSERT(session->videoCaptureDevice() && session->videoOutput() && session->videoOutput()->videoDataOutput());
        m_cameraWriterInput.reset([[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo
                                                          outputSettings:m_videoSettings
                                                          sourceFormatHint:session->videoCaptureDevice().activeFormat.formatDescription]);

        if (m_cameraWriterInput && [m_assetWriter canAddInput:m_cameraWriterInput]) {
            [m_assetWriter addInput:m_cameraWriterInput];
        } else {
            qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to add camera writer input";
            m_cameraWriterInput.reset();
            return false;
        }

        m_cameraWriterInput.data().expectsMediaDataInRealTime = YES;
    }

    m_audioWriterInput.reset();
    if (m_audioQueue) {
        m_audioWriterInput.reset([[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeAudio
                                                             outputSettings:m_audioSettings]);
        if (!m_audioWriterInput) {
            qWarning() << Q_FUNC_INFO << "failed to create audio writer input";
            // But we still can record video.
            if (!m_cameraWriterInput)
                return false;
        } else if ([m_assetWriter canAddInput:m_audioWriterInput]) {
            [m_assetWriter addInput:m_audioWriterInput];
            m_audioWriterInput.data().expectsMediaDataInRealTime = YES;
        } else {
            qWarning() << Q_FUNC_INFO << "failed to add audio writer input";
            m_audioWriterInput.reset();
            if (!m_cameraWriterInput)
                return false;
            // We can (still) write video though ...
        }
    }

    return true;
}

- (void)setQueues
{
    Q_ASSERT(m_service && m_service->session());
    AVFCameraSession *session = m_service->session();

    if (m_videoQueue) {
        Q_ASSERT(session->videoOutput() && session->videoOutput()->videoDataOutput());
        [session->videoOutput()->videoDataOutput() setSampleBufferDelegate:self queue:m_videoQueue];
    }

    if (m_audioQueue) {
        Q_ASSERT(session->audioOutput());
        [session->audioOutput() setSampleBufferDelegate:self queue:m_audioQueue];
    }
}

- (void)updateDuration:(CMTime)newTimeStamp
{
    Q_ASSERT(CMTimeCompare(m_startTime, kCMTimeInvalid));
    Q_ASSERT(CMTimeCompare(m_lastTimeStamp, kCMTimeInvalid));
    if (CMTimeCompare(newTimeStamp, m_lastTimeStamp) > 0) {

        const CMTime duration = CMTimeSubtract(newTimeStamp, m_startTime);
        if (!CMTimeCompare(duration, kCMTimeInvalid))
            return;

        m_durationInMs.storeRelease(CMTimeGetSeconds(duration) * 1000);
        m_lastTimeStamp = newTimeStamp;

        m_delegate->updateDuration([self durationInMs]);
    }
}

- (qint64)durationInMs
{
    return m_durationInMs.loadAcquire();
}

@end
