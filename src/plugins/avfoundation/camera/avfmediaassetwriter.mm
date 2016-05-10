/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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

//#include <QtCore/qmutexlocker.h>
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

} // unnamed namespace

@interface QT_MANGLE_NAMESPACE(AVFMediaAssetWriter) (PrivateAPI)
- (bool)addAudioCapture;
- (bool)addWriterInputs;
- (void)setQueues;
- (NSDictionary *)videoSettings;
- (NSDictionary *)audioSettings;
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
        m_stopped.store(true);
        m_aborted.store(false);
        m_startTime = kCMTimeInvalid;
        m_lastTimeStamp = kCMTimeInvalid;
        m_durationInMs.store(0);
    }

    return self;
}

- (bool)setupWithFileURL:(NSURL *)fileURL
        cameraService:(AVFCameraService *)service
{
    Q_ASSERT(fileURL);

    if (!qt_camera_service_isValid(service)) {
        qDebugCamera() << Q_FUNC_INFO << "invalid camera service";
        return false;
    }

    m_service = service;

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

    m_assetWriter.reset([[AVAssetWriter alloc] initWithURL:fileURL fileType:AVFileTypeQuickTimeMovie error:nil]);
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
        }
        m_assetWriter.reset();
        return false;
    }
    // Ready to start ...
    return true;
}

- (void)start
{
    // To be executed on a writer's queue.
    const QMutexLocker lock(&m_writerMutex);
    if (m_aborted.load())
        return;

    [self setQueues];

    m_setStartTime = true;
    m_stopped.store(false);
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
    if (m_aborted.load())
        return;

    if (m_stopped.load())
        return;

    m_stopped.store(true);
    }

    [m_assetWriter finishWritingWithCompletionHandler:^{
        // This block is async, so by the time it's executed,
        // it's possible that render control was deleted already ...
        const QMutexLocker lock(&m_writerMutex);
        if (m_aborted.load())
            return;

        AVCaptureSession *session = m_service->session()->captureSession();
        [session stopRunning];
        [session removeOutput:m_audioOutput];
        [session removeInput:m_audioInput];
        QMetaObject::invokeMethod(m_delegate, "assetWriterFinished", Qt::QueuedConnection);
    }];
}

- (void)abort
{
    // To be executed on any thread (presumably, it's the main thread),
    // prevents writer from accessing any shared object.
    const QMutexLocker lock(&m_writerMutex);
    m_aborted.store(true);
    if (m_stopped.load())
        return;

    [m_assetWriter finishWritingWithCompletionHandler:^{
    }];
}

- (void)setStartTimeFrom:(CMSampleBufferRef)sampleBuffer
{
    // Writer's queue only.
    Q_ASSERT(m_setStartTime);
    Q_ASSERT(sampleBuffer);

    const QMutexLocker lock(&m_writerMutex);
    if (m_aborted.load() || m_stopped.load())
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
    if (!m_aborted.load() && !m_stopped.load()) {
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
    // This code is executed only on a writer's queue.
    // it does not touch any shared/external data.
    Q_ASSERT(sampleBuffer);

    if (!m_aborted.load() && !m_stopped.load()) {
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
    if (m_stopped.load())
        return;

    if (!CMSampleBufferDataIsReady(sampleBuffer)) {
        qDebugCamera() << Q_FUNC_INFO << "sample buffer is not ready, skipping.";
        return;
    }

    CFRetain(sampleBuffer);

    if (captureOutput != m_audioOutput.data()) {
        const QMutexLocker lock(&m_writerMutex);
        if (m_aborted.load() || m_stopped.load()) {
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

    AVCaptureDevice *audioDevice = m_service->audioInputSelectorControl()->createCaptureDevice();
    if (!audioDevice) {
        qWarning() << Q_FUNC_INFO << "no audio input device available";
        return false;
    } else {
        NSError *error = nil;
        m_audioInput.reset([[AVCaptureDeviceInput deviceInputWithDevice:audioDevice error:&error] retain]);

        if (!m_audioInput || error) {
            qWarning() << Q_FUNC_INFO << "failed to create audio device input";
            m_audioInput.reset();
            return false;
        } else if (![captureSession canAddInput:m_audioInput]) {
            qWarning() << Q_FUNC_INFO << "could not connect the audio input";
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

    m_cameraWriterInput.reset([[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:[self videoSettings]]);
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
        m_audioWriterInput.reset([[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeAudio outputSettings:[self audioSettings]]);
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


- (NSDictionary *)videoSettings
{
    // TODO: these settings should be taken from
    // the video encoding settings control.
    // For now we either take recommended (iOS >= 7.0)
    // or some hardcoded values - they are still better than nothing (nil).
#if QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__IPHONE_7_0)
    AVCaptureVideoDataOutput *videoOutput = m_service->videoOutput()->videoDataOutput();
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_IOS_7_0 && videoOutput)
        return [videoOutput recommendedVideoSettingsForAssetWriterWithOutputFileType:AVFileTypeQuickTimeMovie];
#endif
    NSDictionary *videoSettings = [NSDictionary dictionaryWithObjectsAndKeys:AVVideoCodecH264, AVVideoCodecKey,
                                   [NSNumber numberWithInt:1280], AVVideoWidthKey,
                                   [NSNumber numberWithInt:720], AVVideoHeightKey, nil];

    return videoSettings;
}

- (NSDictionary *)audioSettings
{
    // TODO: these settings should be taken from
    // the video/audio encoder settings control.
    // For now we either take recommended (iOS >= 7.0)
    // or nil - this seems to be good enough.
#if QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__IPHONE_7_0)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_IOS_7_0 && m_audioOutput)
        return [m_audioOutput recommendedAudioSettingsForAssetWriterWithOutputFileType:AVFileTypeQuickTimeMovie];
#endif

    return nil;
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
