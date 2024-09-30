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

    auto frameHandler = [this](QVideoFrame frame) {
        frame.setMirrored(isFrontCamera()); // presentation mirroring
        emit newVideoFrame(frame);
    };

    m_sampleBufferDelegate = [[QAVFSampleBufferDelegate alloc] initWithFrameHandler:frameHandler];

    [m_sampleBufferDelegate setTransformationProvider:[this] { return surfaceTransform(); }];
}

QAVFCamera::~QAVFCamera()
{
    [m_sampleBufferDelegate release];
    [m_videoInput release];
    [m_videoDataOutput release];
    [m_captureSession release];

#if QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(140000, 17000)
    if (@available(macOS 14.0, iOS 17.0, *))
        [m_rotationCoordinator release];
#endif
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

    // Create the rotation-coordinator object for the newly attached
    // capture-device. Rotation coordinator must be initialized after
    // video-input is attached.
#if QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(140000, 170000)
    if (@available(macOS 14.0, iOS 17.0, *)) {
        if (m_rotationCoordinator)
            [m_rotationCoordinator release];
        m_rotationCoordinator = [[AVCaptureDeviceRotationCoordinator alloc]
            initWithDevice:device()
            previewLayer:nil];
    }
#endif
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
    m_cvPixelFormat = CvPixelFormatInvalid;

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return;

    AVCaptureDeviceFormat *newFormat = qt_convert_to_capture_device_format(
            captureDevice, m_cameraFormat, &isCVFormatSupported);

    if (!newFormat)
        newFormat = qt_convert_to_capture_device_format(captureDevice, m_cameraFormat);

    if (newFormat) {
        qt_set_active_format(captureDevice, newFormat, false);
        const auto captureDeviceCVFormat =
                CMVideoFormatDescriptionGetCodecType(newFormat.formatDescription);
        setPixelFormat(m_cameraFormat.pixelFormat(), captureDeviceCVFormat);
        if (captureDeviceCVFormat != m_cvPixelFormat) {
            qCWarning(qLcCamera) << "Output CV format differs with capture device format!"
                                 << m_cvPixelFormat << cvFormatToString(m_cvPixelFormat) << "vs"
                                 << captureDeviceCVFormat
                                 << cvFormatToString(captureDeviceCVFormat);
        }

        m_framePixelFormat = QAVFHelpers::fromCVPixelFormat(m_cvPixelFormat);
    } else {
        qWarning() << "Cannot find AVCaptureDeviceFormat; Did you use format from another camera?";
    }

    const AVPixelFormat avPixelFormat = av_map_videotoolbox_format_to_pixfmt(m_cvPixelFormat);

    HWAccelUPtr hwAccel;

    if (avPixelFormat == AV_PIX_FMT_NONE) {
        qCWarning(qLcCamera) << "Videotoolbox doesn't support cvPixelFormat:" << m_cvPixelFormat
                             << cvFormatToString(m_cvPixelFormat)
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

void QAVFCamera::setPixelFormat(QVideoFrameFormat::PixelFormat cameraPixelFormat,
                                uint32_t inputCvPixFormat)
{
    m_cvPixelFormat = CvPixelFormatInvalid;

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
        return;
    }

    if (bestScore < DefaultAVScore)
        qWarning() << "QCamera::setCameraFormat: Cannot find hw FFmpeg supported cv pix format";

    NSDictionary *outputSettings = @{
        (NSString *)kCVPixelBufferPixelFormatTypeKey : bestFormat,
        (NSString *)kCVPixelBufferMetalCompatibilityKey : @true
    };
    m_videoDataOutput.videoSettings = outputSettings;

    m_cvPixelFormat = [bestFormat unsignedIntValue];
}

QSize QAVFCamera::adjustedResolution() const
{
#ifdef Q_OS_MACOS
    return m_cameraFormat.resolution();
#else
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
#endif // Q_OS_MACOS
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

QVideoFrameFormat QAVFCamera::frameFormat() const
{
    QVideoFrameFormat result = QPlatformCamera::frameFormat();

    const VideoTransformation transform = surfaceTransform();
    result.setRotation(transform.rotation);
    result.setMirrored(transform.mirrorredHorizontallyAfterRotation);

    result.setColorRange(QAVFHelpers::colorRangeForCVPixelFormat(m_cvPixelFormat));

    return result;
}

VideoTransformation QAVFCamera::surfaceTransform() const
{
    VideoTransformation transform;

    // Add the rotation metadata of this AVCaptureDevice.
    //
    // In some situations, AVFoundation can set the connection.videoRotationAgngle
    // implicity and start rotating the pixel buffer before handing it back
    // to us. In this case we want to account for this during preview and capture.
    //
    // This code assumes that m_rotationCoordinator.videoRotationAngleForHorizonLevelCapture
    // and AVCaptureConnection.videoRotationAngle returns degrees
    // that are divisible by 90. This has been the case during testing.
    //
    // TODO: Some rotations are not valid for preview on some devices (such as
    // iPhones not being allowed to have an upside-down window). This usage of the
    // rotation coordinator will still return it as a valid preview rotation, and
    // might cause bugs on iPhone previews.

    int captureAngle = 0;
#if QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(140000, 170000)
    if (@available(macOS 14.0, iOS 17.0, *)) {
        if (m_rotationCoordinator)
            captureAngle = static_cast<int>(std::round(
                m_rotationCoordinator.videoRotationAngleForHorizonLevelCapture));
    }
#endif

    int connectionAngle = 0;
    const AVCaptureConnection *connection = m_videoDataOutput
            ? [m_videoDataOutput connectionWithMediaType:AVMediaTypeVideo]
            : nullptr;

    if (connection) {
#if QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(140000, 170000)
        if (@available(macOS 14.0, iOS 17.0, *))
            connectionAngle = static_cast<int>(std::round(connection.videoRotationAngle));
#endif

        transform.mirrorredHorizontallyAfterRotation = connection.videoMirrored;
    }

    transform.rotation = qVideoRotationFromDegrees(captureAngle - connectionAngle);

    return transform;
}

bool QAVFCamera::isFrontCamera() const
{
    AVCaptureDevice *captureDevice = device();
    return captureDevice && captureDevice.position == AVCaptureDevicePositionFront;
}

QT_END_NAMESPACE

#include "moc_qavfcamera_p.cpp"
