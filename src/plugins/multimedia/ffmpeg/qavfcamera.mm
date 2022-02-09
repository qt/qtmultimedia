/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
#include <qavfcamera_p.h>
#include <qpointer.h>
#include "avfcamerautility_p.h"
#include "qavfhelpers_p.h"
#define AVMediaType XAVMediaType
#include "qffmpegvideobuffer_p.h"
extern "C" {
#include <libavutil/hwcontext_videotoolbox.h>
#include <libavutil/hwcontext.h>
}
#undef AVMediaType



#import <AVFoundation/AVFoundation.h>
#include <CoreVideo/CoreVideo.h>

QT_BEGIN_NAMESPACE

static void releaseHwFrame(void */*opaque*/, uint8_t *data)
{
    CVPixelBufferRelease(CVPixelBufferRef(data));
}

// Make sure this is compatible with the layout used in ffmpeg's hwcontext_videotoolbox
static AVFrame *allocHWFrame(AVBufferRef *hwContext, const CVPixelBufferRef &pixbuf)
{
    AVHWFramesContext *ctx = (AVHWFramesContext*)hwContext->data;
    AVFrame *frame = av_frame_alloc();
    frame->hw_frames_ctx = av_buffer_ref(hwContext);
    frame->extended_data = frame->data;

    frame->buf[0] = av_buffer_create((uint8_t *)pixbuf, 1, releaseHwFrame, NULL, 0);
    frame->data[3] = (uint8_t *)pixbuf;
    CVPixelBufferRetain(pixbuf);
    frame->width  = ctx->width;
    frame->height = ctx->height;
    frame->format = AV_PIX_FMT_VIDEOTOOLBOX;
    Q_ASSERT(frame->width == (int)CVPixelBufferGetWidth(pixbuf));
    Q_ASSERT(frame->height == (int)CVPixelBufferGetHeight(pixbuf));
    return frame;
}

static AVAuthorizationStatus m_cameraAuthorizationStatus = AVAuthorizationStatusNotDetermined;

@interface QAVFSampleBufferDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

- (QAVFSampleBufferDelegate *) initWithCamera:(QAVFCamera *)renderer;

- (void) captureOutput:(AVCaptureOutput *)captureOutput
         didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
         fromConnection:(AVCaptureConnection *)connection;

- (void) setHWContext:(AVBufferRef *)context;

@end

@implementation QAVFSampleBufferDelegate
{
@private
    QAVFCamera *m_camera;
    AVBufferRef *hwFramesContext;
}

- (QAVFSampleBufferDelegate *) initWithCamera:(QAVFCamera *)renderer
{
    if (!(self = [super init]))
        return nil;

    self->m_camera = renderer;
    self->hwFramesContext = nullptr;
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

    int width = CVPixelBufferGetWidth(imageBuffer);
    int height = CVPixelBufferGetHeight(imageBuffer);
    AVFrame *avFrame = allocHWFrame(hwFramesContext, imageBuffer);

    auto *hfc = (AVHWFramesContext *)hwFramesContext->data;
    QFFmpeg::HWAccel accel(hfc->device_ref);
#ifdef USE_SW_FRAMES
    auto *swFrame = av_frame_alloc();
    /* retrieve data from GPU to CPU */
    int ret = av_hwframe_transfer_data(swFrame, avFrame, 0);
    if (ret < 0) {
        qWarning() << "Error transferring the data to system memory\n";
        av_frame_unref(swFrame);
    } else {
        av_frame_unref(avFrame);
        avFrame = swFrame;
    }
#endif

    auto format = QAVFHelpers::fromCVPixelFormat(CVPixelBufferGetPixelFormatType(imageBuffer));
    if (format == QVideoFrameFormat::Format_Invalid) {
        av_frame_unref(avFrame);
        return;
    }

    QFFmpegVideoBuffer *buffer = new QFFmpegVideoBuffer(avFrame, accel);
    QVideoFrame frame(buffer, QVideoFrameFormat(QSize(width, height), format));

    m_camera->syncHandleFrame(frame);
}

- (void) setHWContext:(AVBufferRef *)context
{
    hwFramesContext = context;
    av_buffer_ref(context);
}

@end

QAVFCamera::QAVFCamera(QCamera *parent)
    : QAVFCameraBase(parent)
{
    m_captureSession = [[AVCaptureSession alloc] init];
    m_sampleBufferDelegate = [[QAVFSampleBufferDelegate alloc] initWithCamera:this];
}

QAVFCamera::~QAVFCamera()
{
    [m_sampleBufferDelegate release];
    [m_videoInput release];
    [m_videoDataOutput release];
    [m_captureSession release];
}

void QAVFCamera::requestCameraPermissionIfNeeded()
{
    if (m_cameraAuthorizationStatus == AVAuthorizationStatusAuthorized)
        return;

    switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo])
    {
        case AVAuthorizationStatusAuthorized:
        {
            m_cameraAuthorizationStatus = AVAuthorizationStatusAuthorized;
            break;
        }
        case AVAuthorizationStatusNotDetermined:
        {
            m_cameraAuthorizationStatus = AVAuthorizationStatusNotDetermined;
            QPointer<QAVFCamera> guard(this);
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    if (guard)
                        cameraAuthorizationChanged(granted);
                });
            }];
            break;
        }
        case AVAuthorizationStatusDenied:
        case AVAuthorizationStatusRestricted:
        {
            m_cameraAuthorizationStatus = AVAuthorizationStatusDenied;
            return;
        }
    }
}

void QAVFCamera::cameraAuthorizationChanged(bool authorized)
{
    if (authorized) {
        m_cameraAuthorizationStatus = AVAuthorizationStatusAuthorized;
    } else {
        m_cameraAuthorizationStatus = AVAuthorizationStatusDenied;
        qWarning() << "User has denied access to camera";
    }
}

void QAVFCamera::updateVideoInput()
{
    requestCameraPermissionIfNeeded();
    if (m_cameraAuthorizationStatus != AVAuthorizationStatusAuthorized)
        return;

    [m_captureSession beginConfiguration];

    attachVideoInputDevice();

    if (!m_videoDataOutput) {
        m_videoDataOutput = [[[AVCaptureVideoDataOutput alloc] init] autorelease];

        // Configure video output
        m_delegateQueue = dispatch_queue_create("vf_queue", nullptr);
        [m_videoDataOutput
                setSampleBufferDelegate:m_sampleBufferDelegate
                queue:m_delegateQueue];

        [m_captureSession addOutput:m_videoDataOutput];
    }
    [m_captureSession commitConfiguration];
    deviceOrientationChanged();
}

void QAVFCamera::deviceOrientationChanged(int angle)
{
    AVCaptureConnection *connection = [m_videoDataOutput connectionWithMediaType:AVMediaTypeVideo];
    if (connection == nil || !m_videoDataOutput)
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
void QAVFCamera::attachVideoInputDevice()
{
    if (m_videoInput) {
        [m_captureSession removeInput:m_videoInput];
        [m_videoInput release];
        m_videoInput = nullptr;
    }

    QByteArray deviceId = m_cameraDevice.id();
    if (deviceId.isEmpty())
        return;

    AVCaptureDevice *videoDevice = [AVCaptureDevice deviceWithUniqueID:
                    [NSString stringWithUTF8String: deviceId.constData()]];

    if (!videoDevice)
        return;

    m_videoInput = [AVCaptureDeviceInput
                    deviceInputWithDevice:videoDevice
                    error:nil];
    if (m_videoInput && [m_captureSession canAddInput:m_videoInput]) {
        [m_videoInput retain];
        [m_captureSession addInput:m_videoInput];
    } else {
        qWarning() << "Failed to create video device input";
    }
}

AVCaptureDevice *QAVFCamera::device() const
{
    return m_videoInput ? m_videoInput.device : nullptr;
}

bool QAVFCamera::isActive() const
{
    return m_active;
}

void QAVFCamera::setActive(bool active)
{
    if (m_active == active)
        return;
    requestCameraPermissionIfNeeded();
    if (m_cameraAuthorizationStatus != AVAuthorizationStatusAuthorized)
        return;

    m_active = active;

    if (active) {
        // According to the doc, the capture device must be locked before
        // startRunning to prevent the format we set to be overridden by the
        // session preset.
        [m_videoInput.device lockForConfiguration:nil];
        [m_captureSession startRunning];
        [m_videoInput.device unlockForConfiguration];
    } else {
        [m_captureSession stopRunning];
    }
}

void QAVFCamera::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;

    m_cameraDevice = camera;

    requestCameraPermissionIfNeeded();
    if (m_cameraAuthorizationStatus == AVAuthorizationStatusAuthorized)
        updateVideoInput();
}

bool QAVFCamera::setCameraFormat(const QCameraFormat &format)
{
    if (m_cameraFormat == format)
        return true;

    updateCameraFormat(format);
    return true;
}

void QAVFCamera::updateCameraFormat(const QCameraFormat &format)
{
    m_cameraFormat = format;

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return;

    uint avPixelFormat = 0;
    AVCaptureDeviceFormat *newFormat = qt_convert_to_capture_device_format(captureDevice, format);
    if (newFormat) {
        qt_set_active_format(captureDevice, newFormat, false);
        avPixelFormat = setPixelFormat(format.pixelFormat());
    }

    auto *hwDevice = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
    auto *hwFramesContext = av_hwframe_ctx_alloc(hwDevice);
    av_buffer_unref(&hwDevice);
    auto *c = (AVHWFramesContext *)hwFramesContext->data;
    c->format = AV_PIX_FMT_VIDEOTOOLBOX;
    c->sw_format = av_map_videotoolbox_format_to_pixfmt(avPixelFormat);
    c->width = format.resolution().width();
    c->height = format.resolution().height();
    int err = av_hwframe_ctx_init(hwFramesContext);
    if (err < 0) {
        char str[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, err);
        qWarning() << "failed to init HW frame context" << err << str;
        return;
    }
    [m_sampleBufferDelegate setHWContext:hwFramesContext];
}

uint QAVFCamera::setPixelFormat(const QVideoFrameFormat::PixelFormat pixelFormat)
{
#if 0
    if (rhi() && rhi()->backend() == QRhi::OpenGLES2) {
        if (pixelFormat != QVideoFrameFormat::Format_BGRA8888)
            qWarning() << "OpenGL rhi backend only supports 32BGRA pixel format.";
        return;
    }
#endif
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
        m_videoDataOutput.videoSettings = outputSettings;
    } else {
        qWarning() << "QCamera::setCameraFormat: requested pixel format not supported. Did you use a camera format from another camera?";
    }
    return avPixelFormat;
}

void QAVFCamera::syncHandleFrame(const QVideoFrame &frame)
{
    Q_EMIT newVideoFrame(frame);
}

QT_END_NAMESPACE

#include "moc_qavfcamera_p.cpp"
