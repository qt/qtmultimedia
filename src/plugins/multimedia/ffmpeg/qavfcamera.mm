// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <qavfcamera_p.h>
#include <qpointer.h>
#include <qmediacapturesession.h>
#include <private/qplatformmediacapture_p.h>
#include "avfcamerautility_p.h"
#include "qavfhelpers_p.h"
#include "avfcameradebug_p.h"
#include "qavfsamplebufferdelegate_p.h"
#include <qvideosink.h>
#include <rhi/qrhi.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qpermissions.h>
#define AVMediaType XAVMediaType
#include "qffmpegvideobuffer_p.h"
#include "qffmpegvideosink_p.h"
extern "C" {
#include <libavutil/hwcontext_videotoolbox.h>
#include <libavutil/hwcontext.h>
}
#undef AVMediaType

QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

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

bool QAVFCamera::checkCameraPermission()
{
    const QCameraPermission permission;
    const bool granted = qApp->checkPermission(permission) == Qt::PermissionStatus::Granted;
    if (!granted)
        qWarning() << "Access to camera not granted";

    return granted;
}

void QAVFCamera::updateVideoInput()
{
    if (!checkCameraPermission())
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
    if (!checkCameraPermission())
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

    if (checkCameraPermission())
        updateVideoInput();
    setCameraFormat({});
}

bool QAVFCamera::setCameraFormat(const QCameraFormat &format)
{
    if (m_cameraFormat == format && !format.isNull())
        return true;

    if (!QAVFCameraBase::setCameraFormat(format))
        return false;

    updateCameraFormat();
    return true;
}

void QAVFCamera::updateCameraFormat()
{
    m_framePixelFormat = QVideoFrameFormat::Format_Invalid;

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return;

    AVCaptureDeviceFormat *newFormat = qt_convert_to_capture_device_format(
            captureDevice, m_cameraFormat, &isCVFormatSupported);

    if (!newFormat)
        newFormat = qt_convert_to_capture_device_format(captureDevice, m_cameraFormat);

    std::uint32_t cvPixelFormat = 0;
    if (newFormat) {
        qt_set_active_format(captureDevice, newFormat, false);
        const auto captureDeviceCVFormat =
                CMVideoFormatDescriptionGetCodecType(newFormat.formatDescription);
        cvPixelFormat = setPixelFormat(m_cameraFormat.pixelFormat(), captureDeviceCVFormat);
        if (captureDeviceCVFormat != cvPixelFormat) {
            qCWarning(qLcCamera) << "Output CV format differs with capture device format!"
                                 << cvPixelFormat << cvFormatToString(cvPixelFormat) << "vs"
                                 << captureDeviceCVFormat
                                 << cvFormatToString(captureDeviceCVFormat);

            m_framePixelFormat = QAVFHelpers::fromCVPixelFormat(cvPixelFormat);
        }
    } else {
        qWarning() << "Cannot find AVCaptureDeviceFormat; Did you use format from another camera?";
    }

    const AVPixelFormat avPixelFormat = av_map_videotoolbox_format_to_pixfmt(cvPixelFormat);

    std::unique_ptr<HWAccel> hwAccel;

    if (avPixelFormat == AV_PIX_FMT_NONE) {
        qCWarning(qLcCamera) << "Videotoolbox doesn't support cvPixelFormat:" << cvPixelFormat
                             << cvFormatToString(cvPixelFormat)
                             << "Camera pix format:" << m_cameraFormat.pixelFormat();
    } else {
        hwAccel = HWAccel::create(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
        qCDebug(qLcCamera) << "Create VIDEOTOOLBOX hw context" << hwAccel.get() << "for camera";
    }

    if (hwAccel) {
        hwAccel->createFramesContext(avPixelFormat, adjustedResolution());
        m_hwPixelFormat = hwAccel->hwFormat();
    } else {
        m_hwPixelFormat = AV_PIX_FMT_NONE;
    }

    [m_sampleBufferDelegate setHWAccel:std::move(hwAccel)];
    [m_sampleBufferDelegate setVideoFormatFrameRate:m_cameraFormat.maxFrameRate()];
}

uint32_t QAVFCamera::setPixelFormat(QVideoFrameFormat::PixelFormat cameraPixelFormat,
                                    uint32_t inputCvPixFormat)
{
    auto bestScore = MinAVScore;
    NSNumber *bestFormat = nullptr;
    for (NSNumber *cvPixFmtNumber in m_videoDataOutput.availableVideoCVPixelFormatTypes) {
        auto cvPixFmt = [cvPixFmtNumber unsignedIntValue];
        const auto pixFmt = QAVFHelpers::fromCVPixelFormat(cvPixFmt);
        if (pixFmt == QVideoFrameFormat::Format_Invalid)
            continue;

        auto score = DefaultAVScore;
        if (cvPixFmt == inputCvPixFormat)
            score += 100;
        if (pixFmt == cameraPixelFormat)
            score += 10;
        // if (cvPixFmt == kCVPixelFormatType_32BGRA)
        //     score += 1;

        // This flag determines priorities of using ffmpeg hw frames or
        // the exact camera format match.
        // Maybe configure more, e.g. by some env var?
        constexpr bool ShouldSuppressNotSupportedByFFmpeg = false;

        if (!isCVFormatSupported(cvPixFmt))
            score -= ShouldSuppressNotSupportedByFFmpeg ? 100000 : 5;

        // qDebug() << "----FMT:" << pixFmt << cvPixFmt << score;

        if (score > bestScore) {
            bestScore = score;
            bestFormat = cvPixFmtNumber;
        }
    }

    if (!bestFormat) {
        qWarning() << "QCamera::setCameraFormat: availableVideoCVPixelFormatTypes empty";
        return 0;
    }

    if (bestScore < DefaultAVScore)
        qWarning() << "QCamera::setCameraFormat: Cannot find hw ffmpeg supported cv pix format";

    NSDictionary *outputSettings = @{
        (NSString *)kCVPixelBufferPixelFormatTypeKey : bestFormat,
        (NSString *)kCVPixelBufferMetalCompatibilityKey : @true
    };
    m_videoDataOutput.videoSettings = outputSettings;

    return [bestFormat unsignedIntValue];
}

QSize QAVFCamera::adjustedResolution() const
{
    // Check, that we have matching dimesnions.
    QSize resolution = m_cameraFormat.resolution();
    AVCaptureConnection *connection = [m_videoDataOutput connectionWithMediaType:AVMediaTypeVideo];
    if (!connection.supportsVideoOrientation)
        return resolution;

    // Either portrait but actually sizes of landscape, or
    // landscape with dimensions of portrait - not what
    // sample delegate will report (it depends on videoOrientation set).
    const bool isPortraitOrientation = connection.videoOrientation == AVCaptureVideoOrientationPortrait;
    const bool isPortraitResolution = resolution.height() > resolution.width();
    if (isPortraitOrientation != isPortraitResolution)
        resolution.transpose();

    return resolution;
}

void QAVFCamera::syncHandleFrame(const QVideoFrame &frame)
{
    Q_EMIT newVideoFrame(frame);
}

std::optional<int> QAVFCamera::ffmpegHWPixelFormat() const
{
    return m_hwPixelFormat == AV_PIX_FMT_NONE ? std::optional<int>{} : m_hwPixelFormat;
}

int QAVFCamera::cameraPixelFormatScore(QVideoFrameFormat::PixelFormat pixelFormat,
                                       QVideoFrameFormat::ColorRange colorRange) const
{
    auto cvFormat = QAVFHelpers::toCVPixelFormat(pixelFormat, colorRange);
    return static_cast<int>(isCVFormatSupported(cvFormat));
}

QT_END_NAMESPACE

#include "moc_qavfcamera_p.cpp"
