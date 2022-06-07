// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qabstractvideobuffer_p.h"
#include "avfcamerarenderer_p.h"
#include "avfcamerasession_p.h"
#include "avfcameraservice_p.h"
#include "avfcameradebug_p.h"
#include "avfcamera_p.h"
#include <avfvideosink_p.h>
#include <avfvideobuffer_p.h>
#include "qvideosink.h"
#include "qavfhelpers_p.h"

#include <QtGui/private/qrhi_p.h>

#import <AVFoundation/AVFoundation.h>

#ifdef Q_OS_IOS
#include <QtGui/qopengl.h>
#endif

#include <private/qabstractvideobuffer_p.h>

#include <QtMultimedia/qvideoframeformat.h>

QT_USE_NAMESPACE

@interface AVFCaptureFramesDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

- (AVFCaptureFramesDelegate *) initWithRenderer:(AVFCameraRenderer*)renderer;

- (void) captureOutput:(AVCaptureOutput *)captureOutput
         didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
         fromConnection:(AVCaptureConnection *)connection;

@end

@implementation AVFCaptureFramesDelegate
{
@private
    AVFCameraRenderer *m_renderer;
}

- (AVFCaptureFramesDelegate *) initWithRenderer:(AVFCameraRenderer*)renderer
{
    if (!(self = [super init]))
        return nil;

    self->m_renderer = renderer;
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
    AVFVideoBuffer *buffer = new AVFVideoBuffer(m_renderer, imageBuffer);
    auto format = buffer->videoFormat();
    if (!format.isValid()) {
        delete buffer;
        return;
    }

    QVideoFrame frame(buffer, format);
    m_renderer->syncHandleViewfinderFrame(frame);
}

@end

AVFCameraRenderer::AVFCameraRenderer(QObject *parent)
   : QObject(parent)
{
    m_viewfinderFramesDelegate = [[AVFCaptureFramesDelegate alloc] initWithRenderer:this];
    connect(&m_orientationHandler, &QVideoOutputOrientationHandler::orientationChanged,
            this, &AVFCameraRenderer::deviceOrientationChanged);
}

AVFCameraRenderer::~AVFCameraRenderer()
{
    [m_cameraSession->captureSession() removeOutput:m_videoDataOutput];
    [m_viewfinderFramesDelegate release];
    if (m_delegateQueue)
        dispatch_release(m_delegateQueue);
#ifdef Q_OS_IOS
    if (m_textureCache)
        CFRelease(m_textureCache);
#endif
}

void AVFCameraRenderer::reconfigure()
{
    QMutexLocker lock(&m_vfMutex);

    // ### This is a hack, need to use a reliable way to determine the size and not use the preview layer
    if (m_layer)
        m_sink->setNativeSize(QSize(m_layer.bounds.size.width, m_layer.bounds.size.height));
    nativeSizeChanged();
    deviceOrientationChanged();
}

void AVFCameraRenderer::setOutputSettings(NSDictionary *settings)
{
    if (!m_videoDataOutput)
        return;

    m_videoDataOutput.videoSettings = settings;
    AVFVideoSinkInterface::setOutputSettings(settings);
}

void AVFCameraRenderer::configureAVCaptureSession(AVFCameraSession *cameraSession)
{
    m_cameraSession = cameraSession;
    connect(m_cameraSession, SIGNAL(readyToConfigureConnections()),
            this, SLOT(updateCaptureConnection()));

    m_needsHorizontalMirroring = false;

    m_videoDataOutput = [[[AVCaptureVideoDataOutput alloc] init] autorelease];

    // Configure video output
    m_delegateQueue = dispatch_queue_create("vf_queue", nullptr);
    [m_videoDataOutput
            setSampleBufferDelegate:m_viewfinderFramesDelegate
            queue:m_delegateQueue];

    [m_cameraSession->captureSession() addOutput:m_videoDataOutput];
}

void AVFCameraRenderer::updateCaptureConnection()
{
    AVCaptureConnection *connection = [m_videoDataOutput connectionWithMediaType:AVMediaTypeVideo];
    if (connection == nil || !m_cameraSession->videoCaptureDevice())
        return;

    // Frames of front-facing cameras should be mirrored horizontally (it's the default when using
    // AVCaptureVideoPreviewLayer but not with AVCaptureVideoDataOutput)
    if (connection.isVideoMirroringSupported)
        connection.videoMirrored = m_cameraSession->videoCaptureDevice().position == AVCaptureDevicePositionFront;

    // If the connection does't support mirroring, we'll have to do it ourselves
    m_needsHorizontalMirroring = !connection.isVideoMirrored
            && m_cameraSession->videoCaptureDevice().position == AVCaptureDevicePositionFront;

    deviceOrientationChanged();
}

void AVFCameraRenderer::deviceOrientationChanged(int angle)
{
    AVCaptureConnection *connection = [m_videoDataOutput connectionWithMediaType:AVMediaTypeVideo];
    if (connection == nil || !m_cameraSession->videoCaptureDevice())
        return;

    if (!connection.supportsVideoOrientation)
        return;

    if (angle < 0)
        angle = m_orientationHandler.currentOrientation();

    AVCaptureVideoOrientation orientation = AVCaptureVideoOrientationPortrait;
    switch (angle) {
    default:
        break;
    case 90:
        orientation = AVCaptureVideoOrientationLandscapeRight;
        break;
    case 180:
        // this keeps the last orientation, don't do anything
        return;
    case 270:
        orientation = AVCaptureVideoOrientationLandscapeLeft;
        break;
    }

    connection.videoOrientation = orientation;
}

//can be called from non main thread
void AVFCameraRenderer::syncHandleViewfinderFrame(const QVideoFrame &frame)
{
    Q_EMIT newViewfinderFrame(frame);

    QMutexLocker lock(&m_vfMutex);

    if (!m_lastViewfinderFrame.isValid()) {
        static QMetaMethod handleViewfinderFrameSlot = metaObject()->method(
                    metaObject()->indexOfMethod("handleViewfinderFrame()"));

        handleViewfinderFrameSlot.invoke(this, Qt::QueuedConnection);
    }

    m_lastViewfinderFrame = frame;
}

AVCaptureVideoDataOutput *AVFCameraRenderer::videoDataOutput() const
{
    return m_videoDataOutput;
}

AVFCaptureFramesDelegate *AVFCameraRenderer::captureDelegate() const
{
    return m_viewfinderFramesDelegate;
}

void AVFCameraRenderer::resetCaptureDelegate() const
{
    [m_videoDataOutput setSampleBufferDelegate:m_viewfinderFramesDelegate queue:m_delegateQueue];
}

void AVFCameraRenderer::handleViewfinderFrame()
{
    QVideoFrame frame;
    {
        QMutexLocker lock(&m_vfMutex);
        frame = m_lastViewfinderFrame;
        m_lastViewfinderFrame = QVideoFrame();
    }

    if (m_sink && frame.isValid()) {
        // ### pass format to surface
        QVideoFrameFormat format = frame.surfaceFormat();
        if (m_needsHorizontalMirroring)
            format.setMirrored(true);

        m_sink->setVideoFrame(frame);
    }
}

void AVFCameraRenderer::setPixelFormat(const QVideoFrameFormat::PixelFormat pixelFormat)
{
    if (rhi() && rhi()->backend() == QRhi::OpenGLES2) {
        if (pixelFormat != QVideoFrameFormat::Format_BGRA8888)
            qWarning() << "OpenGL rhi backend only supports 32BGRA pixel format.";
        return;
    }

    // Default to 32BGRA pixel formats on the viewfinder, in case the requested
    // format can't be used (shouldn't happen unless the developers sets a wrong camera
    // format on the camera).
    unsigned avPixelFormat = kCVPixelFormatType_32BGRA;
    if (!QAVFHelpers::toCVPixelFormat(pixelFormat, avPixelFormat))
        qWarning() << "QCamera::setCameraFormat: couldn't convert requested pixel format, using ARGB32";

    bool isSupported = false;
    NSArray *supportedPixelFormats = m_videoDataOutput.availableVideoCVPixelFormatTypes;
    for (NSNumber *currentPixelFormat in supportedPixelFormats)
    {
        if ([currentPixelFormat unsignedIntValue] == avPixelFormat) {
            isSupported = true;
            break;
        }
    }

    if (isSupported) {
        NSDictionary* outputSettings = @{
            (NSString *)kCVPixelBufferPixelFormatTypeKey: [NSNumber numberWithUnsignedInt:avPixelFormat],
            (NSString *)kCVPixelBufferMetalCompatibilityKey: @true
        };
        setOutputSettings(outputSettings);
    } else {
        qWarning() << "QCamera::setCameraFormat: requested pixel format not supported. Did you use a camera format from another camera?";
    }
}

#include "moc_avfcamerarenderer_p.cpp"

