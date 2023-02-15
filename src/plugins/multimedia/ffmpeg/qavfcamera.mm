// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <qavfcamera_p.h>
#include <qpointer.h>
#include <qmediacapturesession.h>
#include <private/qplatformmediacapture_p.h>
#include "avfcamerautility_p.h"
#include "qavfhelpers_p.h"
#include "qavfsamplebufferdelegate_p.h"
#include <qvideosink.h>
#include <private/qrhi_p.h>
#define AVMediaType XAVMediaType
#include "qffmpegvideobuffer_p.h"
#include "qffmpegvideosink_p.h"
extern "C" {
#include <libavutil/hwcontext_videotoolbox.h>
#include <libavutil/hwcontext.h>
}
#undef AVMediaType

static AVAuthorizationStatus m_cameraAuthorizationStatus = AVAuthorizationStatusNotDetermined;

QT_BEGIN_NAMESPACE

QAVFCamera::QAVFCamera(QCamera *parent)
    : QAVFCameraBase(parent)
{
    m_captureSession = [[AVCaptureSession alloc] init];
    m_sampleBufferDelegate = [[QAVFSampleBufferDelegate alloc]
            initWithFrameHandler:[this](const QVideoFrame &frame) { syncHandleFrame(frame); }];
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
        m_videoDataOutput = [[AVCaptureVideoDataOutput alloc] init];

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

    emit activeChanged(active);
}

void QAVFCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    m_session = session ? session->captureSession() : nullptr;
}

void QAVFCamera::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;

    m_cameraDevice = camera;

    requestCameraPermissionIfNeeded();
    if (m_cameraAuthorizationStatus == AVAuthorizationStatusAuthorized)
        updateVideoInput();
    setCameraFormat({});
}

bool QAVFCamera::setCameraFormat(const QCameraFormat &format)
{
    if (m_cameraFormat == format && !format.isNull())
        return true;

    QAVFCameraBase::setCameraFormat(format);
    updateCameraFormat();
    return true;
}

void QAVFCamera::updateCameraFormat()
{
    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return;

    std::uint32_t cvPixelFormat = 0;
    AVCaptureDeviceFormat *newFormat = qt_convert_to_capture_device_format(captureDevice, m_cameraFormat);
    if (newFormat) {
        qt_set_active_format(captureDevice, newFormat, false);
        cvPixelFormat = setPixelFormat(m_cameraFormat.pixelFormat());
    }

    const AVPixelFormat avPixelFormat = av_map_videotoolbox_format_to_pixfmt(cvPixelFormat);

    std::unique_ptr<QFFmpeg::HWAccel> hwAccel;

    if (avPixelFormat == AV_PIX_FMT_NONE) {
        auto formatDescIt =
                std::make_reverse_iterator(reinterpret_cast<const char *>(&cvPixelFormat));
        qWarning() << "Videotoolbox desn't support cvPixelFormat:" << cvPixelFormat
                   << std::string(formatDescIt - 4, formatDescIt)
                   << " Camera pix format:" << m_cameraFormat.pixelFormat();
    } else {
        hwAccel = QFFmpeg::HWAccel::create(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
    }

    if (hwAccel) {
        hwAccel->createFramesContext(avPixelFormat, m_cameraFormat.resolution());
        hwPixelFormat = hwAccel->hwFormat();
    } else {
        hwPixelFormat = AV_PIX_FMT_NONE;
    }
    [m_sampleBufferDelegate setHWAccel:std::move(hwAccel)];
    [m_sampleBufferDelegate setVideoFormatFrameRate:m_cameraFormat.maxFrameRate()];
}

std::uint32_t QAVFCamera::setPixelFormat(const QVideoFrameFormat::PixelFormat pixelFormat)
{
    // Default to 32BGRA pixel formats on the viewfinder, in case the requested
    // format can't be used (shouldn't happen unless the developers sets a wrong camera
    // format on the camera).
    std::uint32_t cvPixelFormat = kCVPixelFormatType_32BGRA;
    if (!QAVFHelpers::toCVPixelFormat(pixelFormat, cvPixelFormat))
        qWarning() << "QCamera::setCameraFormat: couldn't convert requested pixel format, using ARGB32";

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
            kCVPixelBufferPixelFormatTypeKey : [NSNumber numberWithUnsignedInt:cvPixelFormat],
            (NSString *)kCVPixelBufferMetalCompatibilityKey : @true
        };
        m_videoDataOutput.videoSettings = outputSettings;
    } else {
        qWarning() << "QCamera::setCameraFormat: requested pixel format not supported. Did you use a camera format from another camera?";
    }
    return cvPixelFormat;
}

void QAVFCamera::syncHandleFrame(const QVideoFrame &frame)
{
    Q_EMIT newVideoFrame(frame);
}

QT_END_NAMESPACE

#include "moc_qavfcamera_p.cpp"
