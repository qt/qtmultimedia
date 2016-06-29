/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "avfaudioinputselectorcontrol.h"
#include "avfmediarecordercontrol_ios.h"
#include "avfcamerarenderercontrol.h"
#include "avfmediaassetwriter.h"
#include "avfcameraservice.h"
#include "avfcamerasession.h"
#include "avfcameradebug.h"
#include "avfmediacontainercontrol.h"

#include <QtCore/qmetaobject.h>
#include <QtCore/qsysinfo.h>

QT_USE_NAMESPACE

namespace {

bool qt_camera_service_isValid(AVFCameraService *service)
{
    if (!service || !service->session())
        return false;

    AVFCameraSession *session = service->session();
    if (!session->captureSession())
        return false;

    if (!session->videoInput())
        return false;

    if (!service->videoOutput()
        || !service->videoOutput()->videoDataOutput()) {
        return false;
    }

    return true;
}

enum WriterState
{
    WriterStateIdle,
    WriterStateActive,
    WriterStateAborted
};

} // unnamed namespace

@interface QT_MANGLE_NAMESPACE(AVFMediaAssetWriter) (PrivateAPI)
- (bool)addAudioCapture;
- (bool)addWriterInputs;
- (void)setQueues;
- (void)updateDuration:(CMTime)newTimeStamp;
@end

@implementation QT_MANGLE_NAMESPACE(AVFMediaAssetWriter)

- (id)initWithQueue:(dispatch_queue_t)writerQueue
      delegate:(AVFMediaRecorderControlIOS *)delegate
{
    Q_ASSERT(writerQueue);
    Q_ASSERT(delegate);

    if (self = [super init]) {
        // "Shared" queue:
        dispatch_retain(writerQueue);
        m_writerQueue.reset(writerQueue);

        m_delegate = delegate;
        m_setStartTime = true;
        m_state.store(WriterStateIdle);
        m_startTime = kCMTimeInvalid;
        m_lastTimeStamp = kCMTimeInvalid;
        m_durationInMs.store(0);
        m_audioSettings = nil;
        m_videoSettings = nil;
    }

    return self;
}

- (bool)setupWithFileURL:(NSURL *)fileURL
        cameraService:(AVFCameraService *)service
        audioSettings:(NSDictionary *)audioSettings
        videoSettings:(NSDictionary *)videoSettings
        transform:(CGAffineTransform)transform
{
    Q_ASSERT(fileURL);

    if (!qt_camera_service_isValid(service)) {
        qDebugCamera() << Q_FUNC_INFO << "invalid camera service";
        return false;
    }

    m_service = service;
    m_audioSettings = audioSettings;
    m_videoSettings = videoSettings;

    m_videoQueue.reset(dispatch_queue_create("video-output-queue", DISPATCH_QUEUE_SERIAL));
    if (!m_videoQueue) {
        qDebugCamera() << Q_FUNC_INFO << "failed to create video queue";
        return false;
    }
    dispatch_set_target_queue(m_videoQueue, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
    m_audioQueue.reset(dispatch_queue_create("audio-output-queue", DISPATCH_QUEUE_SERIAL));
    if (!m_audioQueue) {
        qDebugCamera() << Q_FUNC_INFO << "failed to create audio queue";
        // But we still can write video!
    }

    m_assetWriter.reset([[AVAssetWriter alloc] initWithURL:fileURL
                                               fileType:m_service->mediaContainerControl()->fileType()
                                               error:nil]);
    if (!m_assetWriter) {
        qDebugCamera() << Q_FUNC_INFO << "failed to create asset writer";
        return false;
    }

    bool audioCaptureOn = false;

    if (m_audioQueue)
        audioCaptureOn = [self addAudioCapture];

    if (![self addWriterInputs]) {
        if (audioCaptureOn) {
            AVCaptureSession *session = m_service->session()->captureSession();
            [session removeOutput:m_audioOutput];
            [session removeInput:m_audioInput];
            m_audioOutput.reset();
            m_audioInput.reset();
            m_audioCaptureDevice = 0;
        }
        m_assetWriter.reset();
        return false;
    }

    m_cameraWriterInput.data().transform = transform;

    // Ready to start ...
    return true;
}

- (void)start
{
    // 'start' is executed on a special writer's queue.
    // We lock a mutex for the whole function body
    // - render control probably was deleted
    // 'immediately' after submitting 'start'
    // and before 'start' is actually executed ...
    // So if we're starting, render control can not
    // abort us 'in the middle'.
    const QMutexLocker lock(&m_writerMutex);
    if (m_state.load() == WriterStateAborted)
        return;

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
    // To be executed on a writer's queue.
    {
    const QMutexLocker lock(&m_writerMutex);
    if (m_state.load() != WriterStateActive)
        return;
    }

    [m_assetWriter finishWritingWithCompletionHandler:^{
        // This block is async, so by the time it's executed,
        // it's possible that render control was deleted already ...
        // We lock so that nobody can call 'abort' in the middle ...
        const QMutexLocker lock(&m_writerMutex);
        if (m_state.load() != WriterStateActive)
            return;

        AVCaptureSession *session = m_service->session()->captureSession();
        [session stopRunning];
        [session removeOutput:m_audioOutput];
        [session removeInput:m_audioInput];
        m_state.storeRelease(WriterStateIdle);
        QMetaObject::invokeMethod(m_delegate, "assetWriterFinished", Qt::QueuedConnection);
    }];
}

- (void)abort
{
    // To be executed on any thread (presumably, it's the main thread),
    // prevents writer from accessing any shared object.
    const QMutexLocker lock(&m_writerMutex);
    if (m_state.fetchAndStoreRelease(WriterStateAborted) != WriterStateActive) {
        // Not recording, nothing to stop.
        return;
    }

    [m_assetWriter finishWritingWithCompletionHandler:^{
    }];
}

- (void)setStartTimeFrom:(CMSampleBufferRef)sampleBuffer
{
    // Writer's queue only.
    Q_ASSERT(m_setStartTime);
    Q_ASSERT(sampleBuffer);

    const QMutexLocker lock(&m_writerMutex);
    if (m_state.load() != WriterStateActive)
        return;

    QMetaObject::invokeMethod(m_delegate, "assetWriterStarted", Qt::QueuedConnection);

    m_durationInMs.store(0);
    m_startTime = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    m_lastTimeStamp = m_startTime;
    [m_assetWriter startSessionAtSourceTime:m_startTime];
    m_setStartTime = false;
}

- (void)writeVideoSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    Q_ASSERT(sampleBuffer);

    // This code is executed only on a writer's queue.
    if (m_state.loadAcquire() == WriterStateActive) {
        if (m_setStartTime)
            [self setStartTimeFrom:sampleBuffer];

        if (m_cameraWriterInput.data().readyForMoreMediaData) {
            [self updateDuration:CMSampleBufferGetPresentationTimeStamp(sampleBuffer)];
            [m_cameraWriterInput appendSampleBuffer:sampleBuffer];
        }
    }


    CFRelease(sampleBuffer);
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

    CFRelease(sampleBuffer);
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection *)connection
{
    Q_UNUSED(connection)

    // This method can be called on either video or audio queue,
    // never on a writer's queue, it needs access to a shared data, so
    // lock is required.
    if (m_state.loadAcquire() != WriterStateActive)
        return;

    if (!CMSampleBufferDataIsReady(sampleBuffer)) {
        qDebugCamera() << Q_FUNC_INFO << "sample buffer is not ready, skipping.";
        return;
    }

    CFRetain(sampleBuffer);

    if (captureOutput != m_audioOutput.data()) {
        const QMutexLocker lock(&m_writerMutex);
        if (m_state.load() != WriterStateActive) {
            CFRelease(sampleBuffer);
            return;
        }
        // Find renderercontrol's delegate and invoke its method to
        // show updated viewfinder's frame.
        if (m_service && m_service->videoOutput()) {
            NSObject<AVCaptureVideoDataOutputSampleBufferDelegate> *vfDelegate =
                (NSObject<AVCaptureVideoDataOutputSampleBufferDelegate> *)m_service->videoOutput()->captureDelegate();
            if (vfDelegate)
                [vfDelegate captureOutput:nil didOutputSampleBuffer:sampleBuffer fromConnection:nil];
        }

        dispatch_async(m_writerQueue, ^{
            [self writeVideoSampleBuffer:sampleBuffer];
        });
    } else {
        dispatch_async(m_writerQueue, ^{
            [self writeAudioSampleBuffer:sampleBuffer];
        });
    }
}

- (bool)addAudioCapture
{
    Q_ASSERT(m_service && m_service->session() && m_service->session()->captureSession());

    if (!m_service->audioInputSelectorControl())
        return false;

    AVCaptureSession *captureSession = m_service->session()->captureSession();

    m_audioCaptureDevice = m_service->audioInputSelectorControl()->createCaptureDevice();
    if (!m_audioCaptureDevice) {
        qWarning() << Q_FUNC_INFO << "no audio input device available";
        return false;
    } else {
        NSError *error = nil;
        m_audioInput.reset([[AVCaptureDeviceInput deviceInputWithDevice:m_audioCaptureDevice error:&error] retain]);

        if (!m_audioInput || error) {
            qWarning() << Q_FUNC_INFO << "failed to create audio device input";
            m_audioCaptureDevice = 0;
            m_audioInput.reset();
            return false;
        } else if (![captureSession canAddInput:m_audioInput]) {
            qWarning() << Q_FUNC_INFO << "could not connect the audio input";
            m_audioCaptureDevice = 0;
            m_audioInput.reset();
            return false;
        } else {
            [captureSession addInput:m_audioInput];
        }
    }


    m_audioOutput.reset([[AVCaptureAudioDataOutput alloc] init]);
    if (m_audioOutput && [captureSession canAddOutput:m_audioOutput]) {
        [captureSession addOutput:m_audioOutput];
    } else {
        qDebugCamera() << Q_FUNC_INFO << "failed to add audio output";
        [captureSession removeInput:m_audioInput];
        m_audioCaptureDevice = 0;
        m_audioInput.reset();
        m_audioOutput.reset();
        return false;
    }

    return true;
}

- (bool)addWriterInputs
{
    Q_ASSERT(m_service && m_service->videoOutput()
             && m_service->videoOutput()->videoDataOutput());
    Q_ASSERT(m_assetWriter);

    m_cameraWriterInput.reset([[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo
                                                          outputSettings:m_videoSettings
                                                          sourceFormatHint:m_service->session()->videoCaptureDevice().activeFormat.formatDescription]);
    if (!m_cameraWriterInput) {
        qDebugCamera() << Q_FUNC_INFO << "failed to create camera writer input";
        return false;
    }

    if ([m_assetWriter canAddInput:m_cameraWriterInput]) {
        [m_assetWriter addInput:m_cameraWriterInput];
    } else {
        qDebugCamera() << Q_FUNC_INFO << "failed to add camera writer input";
        m_cameraWriterInput.reset();
        return false;
    }

    m_cameraWriterInput.data().expectsMediaDataInRealTime = YES;

    if (m_audioOutput) {
        CMFormatDescriptionRef sourceFormat = m_audioCaptureDevice ? m_audioCaptureDevice.activeFormat.formatDescription : 0;
        m_audioWriterInput.reset([[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeAudio
                                                             outputSettings:m_audioSettings
                                                             sourceFormatHint:sourceFormat]);
        if (!m_audioWriterInput) {
            qDebugCamera() << Q_FUNC_INFO << "failed to create audio writer input";
            // But we still can record video.
        } else if ([m_assetWriter canAddInput:m_audioWriterInput]) {
            [m_assetWriter addInput:m_audioWriterInput];
            m_audioWriterInput.data().expectsMediaDataInRealTime = YES;
        } else {
            qDebugCamera() << Q_FUNC_INFO << "failed to add audio writer input";
            m_audioWriterInput.reset();
            // We can (still) write video though ...
        }
    }

    return true;
}

- (void)setQueues
{
    Q_ASSERT(m_service && m_service->videoOutput() && m_service->videoOutput()->videoDataOutput());
    Q_ASSERT(m_videoQueue);

    [m_service->videoOutput()->videoDataOutput() setSampleBufferDelegate:self queue:m_videoQueue];

    if (m_audioOutput) {
        Q_ASSERT(m_audioQueue);
        [m_audioOutput setSampleBufferDelegate:self queue:m_audioQueue];
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

        m_durationInMs.store(CMTimeGetSeconds(duration) * 1000);
        m_lastTimeStamp = newTimeStamp;
    }
}

@end
