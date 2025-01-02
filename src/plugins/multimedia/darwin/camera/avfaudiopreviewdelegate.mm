// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfcamerasession_p.h"
#include "avfaudiopreviewdelegate_p.h"

QT_USE_NAMESPACE

@implementation AVFAudioPreviewDelegate
{
@private
    AVSampleBufferAudioRenderer *m_audioRenderer;
    AVFCameraSession *m_session;
    AVSampleBufferRenderSynchronizer *m_audioBufferSynchronizer;
    dispatch_queue_t m_audioPreviewQueue;
}

- (id)init
{
    if (self = [super init]) {
        m_session = nil;
        m_audioBufferSynchronizer = [[AVSampleBufferRenderSynchronizer alloc] init];
        m_audioRenderer = [[AVSampleBufferAudioRenderer alloc] init];
        [m_audioBufferSynchronizer addRenderer:m_audioRenderer];
        return self;
    }
    return nil;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection *)connection
{
    Q_UNUSED(connection);
    Q_ASSERT(m_session);

    if (!CMSampleBufferDataIsReady(sampleBuffer)) {
        qWarning() << Q_FUNC_INFO << "sample buffer is not ready, skipping.";
        return;
    }

    CFRetain(sampleBuffer);

    dispatch_async(m_audioPreviewQueue, ^{
        [self renderAudioSampleBuffer:sampleBuffer];
        CFRelease(sampleBuffer);
    });
}

- (void)renderAudioSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    Q_ASSERT(sampleBuffer);
    Q_ASSERT(m_session);

    if (m_audioBufferSynchronizer && m_audioRenderer) {
        [m_audioRenderer enqueueSampleBuffer:sampleBuffer];
        if (m_audioBufferSynchronizer.rate == 0)
            [m_audioBufferSynchronizer setRate:1 time:CMSampleBufferGetPresentationTimeStamp(sampleBuffer)];
    }
}

- (void)resetAudioPreviewDelegate
{
    [m_session->audioOutput() setSampleBufferDelegate:self queue:m_audioPreviewQueue];
}

- (void)setupWithCaptureSession: (AVFCameraSession*)session
        audioOutputDevice: (NSString*)deviceId
{
    m_session = session;

    m_audioPreviewQueue = dispatch_queue_create("audio-preview-queue", nullptr);
    [m_session->audioOutput() setSampleBufferDelegate:self queue:m_audioPreviewQueue];
#ifdef Q_OS_MACOS
    m_audioRenderer.audioOutputDeviceUniqueID = deviceId;
#endif
}

- (void)setVolume: (float)volume
{
    m_audioRenderer.volume = volume;
}

- (void)setMuted: (bool)muted
{
    m_audioRenderer.muted = muted;
}

-(void)dealloc {
    m_session = nil;
    [m_audioRenderer release];
    [m_audioBufferSynchronizer release];
    dispatch_release(m_audioPreviewQueue);

    [super dealloc];
}

@end
