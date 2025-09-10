// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qabstractvideobuffer.h"
#include "private/qcameradevice_p.h"
#include "private/qvideoframe_p.h"
#include "avfcamerarenderer_p.h"
#include "avfcamerasession_p.h"
#include "avfcameraservice_p.h"
#include <QtMultimedia/private/qavfcameradebug_p.h>
#include "avfcamera_p.h"
#include <avfvideosink_p.h>
#include <avfvideobuffer_p.h>
#include "qvideosink.h"
#include <QtMultimedia/private/qavfhelpers_p.h>

#include <rhi/qrhi.h>

#import <AVFoundation/AVFoundation.h>

#ifdef Q_OS_IOS
#include <QtGui/qopengl.h>
#endif

#include <QtCore/qmetaobject.h>
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

    auto buffer =
            std::make_unique<AVFVideoBuffer>(m_renderer,
                                             QCFType<CVImageBufferRef>::constructFromGet(
                                                     CMSampleBufferGetImageBuffer(sampleBuffer)));
    auto format = buffer->videoFormat();
    if (!format.isValid()) {
        return;
    }

    QVideoFrame frame = QVideoFramePrivate::createFrame(std::move(buffer), format);
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
    [m_videoDataOutput release];

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

void AVFCameraRenderer::setOutputSettings()
{
    if (!m_videoDataOutput)
        return;

    if (m_cameraSession) {
        const auto format = m_cameraSession->cameraFormat();
        if (format.pixelFormat() != QVideoFrameFormat::Format_Invalid)
            setPixelFormat(format.pixelFormat(), QCameraFormatPrivate::getColorRange(format));
    }

    // If no output settings set from above,
    // it's most likely because the rhi is OpenGL
    // and the pixel format is not BGRA.
    // We force this in the base class implementation
    if (!m_outputSettings)
        AVFVideoSinkInterface::setOutputSettings();

    if (m_outputSettings)
        m_videoDataOutput.videoSettings = m_outputSettings;
}

void AVFCameraRenderer::configureAVCaptureSession(AVFCameraSession *cameraSession)
{
    m_cameraSession = cameraSession;
    connect(m_cameraSession, SIGNAL(readyToConfigureConnections()),
            this, SLOT(updateCaptureConnection()));

    m_needsHorizontalMirroring = false;

    m_videoDataOutput = [[AVCaptureVideoDataOutput alloc] init];

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
        // frame.setMirroed(m_needsHorizontalMirroring) ?
        m_sink->setVideoFrame(frame);
    }
}

void AVFCameraRenderer::setPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat,
                                       QVideoFrameFormat::ColorRange colorRange)
{
    if (rhi() && rhi()->backend() == QRhi::OpenGLES2) {
        if (pixelFormat != QVideoFrameFormat::Format_BGRA8888)
            qWarning() << "OpenGL rhi backend only supports 32BGRA pixel format.";
        return;
    }

    // Default to 32BGRA pixel formats on the viewfinder, in case the requested
    // format can't be used (shouldn't happen unless the developers sets a wrong camera
    // format on the camera).
    auto cvPixelFormat = QAVFHelpers::toCVPixelFormat(pixelFormat, colorRange);
    if (cvPixelFormat == CvPixelFormatInvalid) {
        cvPixelFormat = kCVPixelFormatType_32BGRA;
        qWarning() << "QCamera::setCameraFormat: couldn't convert requested pixel format, using ARGB32";
    }

    bool isSupported = false;
    NSArray *supportedPixelFormats = m_videoDataOutput.availableVideoCVPixelFormatTypes;
    for (NSNumber *currentPixelFormat in supportedPixelFormats)
    {
        if ([currentPixelFormat unsignedIntValue] == cvPixelFormat) {
            isSupported = true;
            break;
        }
    }

    if (isSupported) {
        NSDictionary *outputSettings = @{
            (NSString *)
            kCVPixelBufferPixelFormatTypeKey : [NSNumber numberWithUnsignedInt:cvPixelFormat]
#ifndef Q_OS_IOS // On iOS this key generates a warning about 'unsupported key'.
            ,
            (NSString *)kCVPixelBufferMetalCompatibilityKey : @true
#endif // Q_OS_IOS
        };
        if (m_outputSettings)
            [m_outputSettings release];
        m_outputSettings = [[NSDictionary alloc] initWithDictionary:outputSettings];
    } else {
        qWarning() << "QCamera::setCameraFormat: requested pixel format not supported. Did you use a camera format from another camera?";
    }
}

#include "moc_avfcamerarenderer_p.cpp"

