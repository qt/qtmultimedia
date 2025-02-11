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

    updateRotationTracking();
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
}

AVCaptureDevice *QAVFCamera::device() const
{
    return m_videoInput ? m_videoInput.device : nullptr;
}

void QAVFCamera::onActiveChanged(bool active)
{
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

    // If the camera becomes active, we want to start tracking the rotation of the camera
    updateRotationTracking();
}

void QAVFCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    m_session = session ? session->captureSession() : nullptr;
}

void QAVFCamera::onCameraDeviceChanged(const QCameraDevice &device)
{
    if (device.isNull() || !checkCameraPermission())
        return;

    // updateVideoInput must be called before setCameraFormat.
    updateVideoInput();

    // When we change camera, we need to clear up the existing
    // rotation tracker state and set up the new one.
    updateRotationTracking();
}

bool QAVFCamera::tryApplyCameraFormat(const QCameraFormat &format)
{
    // TODO: In the future, we should be able to return false if we failed
    // to apply the format.
    updateCameraFormat(format);
    return true;
}

void QAVFCamera::updateCameraFormat(const QCameraFormat &newFormat)
{
    m_framePixelFormat = QVideoFrameFormat::Format_Invalid;
    m_cvPixelFormat = CvPixelFormatInvalid;

    AVCaptureDevice *captureDevice = device();
    if (!captureDevice)
        return;

    AVCaptureDeviceFormat *newDeviceFormat = qt_convert_to_capture_device_format(
        captureDevice,
        newFormat,
        &QFFmpeg::isCVFormatSupported);

    // If we can't find a AVCaptureDeviceFormat supported by FFmpeg,
    // fall back to one not supported by FFmpeg.
    if (!newDeviceFormat)
        newDeviceFormat = qt_convert_to_capture_device_format(captureDevice, newFormat);

    if (newDeviceFormat) {
        qt_set_active_format(captureDevice, newDeviceFormat, false);
        const auto captureDeviceCVFormat =
                CMVideoFormatDescriptionGetCodecType(newDeviceFormat.formatDescription);
        setPixelFormat(newFormat.pixelFormat(), captureDeviceCVFormat);
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
                             << "Camera pix format:" << newFormat.pixelFormat();
    } else {
        hwAccel = HWAccel::create(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
        qCDebug(qLcCamera) << "Create VIDEOTOOLBOX hw context" << hwAccel.get() << "for camera";
    }

    if (hwAccel) {
        hwAccel->createFramesContext(avPixelFormat, adjustedResolution(newFormat));
        m_hwPixelFormat = hwAccel->hwFormat();
    } else {
        m_hwPixelFormat = AV_PIX_FMT_NONE;
    }

    [m_sampleBufferDelegate setHWAccel:std::move(hwAccel)];
    [m_sampleBufferDelegate setVideoFormatFrameRate:newFormat.maxFrameRate()];
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

QSize QAVFCamera::adjustedResolution(const QCameraFormat& newFormat) const
{
#ifdef Q_OS_MACOS
    return newFormat.resolution();
#else
    // Check, that we have matching dimesnions.
    QSize resolution = newFormat.resolution();
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

    int captureAngle = getCurrentRotationAngleDegrees();

    // In some situations, AVFoundation can set the AVCaptureConnection.videoRotationAgngle
    // implicity and start rotating the pixel buffer before handing it back
    // to us. In this case we want to account for this during preview and capture.
    //
    // This code assumes that AVCaptureConnection.videoRotationAngle returns degrees
    // that are divisible by 90. This has been the case during testing.
    int connectionAngle = 0;
    const AVCaptureConnection *connection = m_videoDataOutput ?
        [m_videoDataOutput connectionWithMediaType:AVMediaTypeVideo] :
        nullptr;
    if (connection) {
        if (@available(macOS 14.0, iOS 17.0, *))
            connectionAngle = static_cast<int>(std::round(connection.videoRotationAngle));

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

// Clears or sets up rotation tracking based on isActive()
void QAVFCamera::updateRotationTracking()
{
    // If the camera is active, it should have either a RotationCoordinator
    // or start listening for UIDeviceOrientation changes.
    if (isActive()) {
        // Use RotationCoordinator if we can.
        if (@available(macOS 14.0, iOS 17.0, *)) {
            if (m_rotationCoordinator)
                [m_rotationCoordinator release];
            m_rotationCoordinator = nullptr;

            AVCaptureDevice* captureDevice = device();
            if (!captureDevice) {
                qCDebug(qLcCamera) << "attempted to setup AVCaptureDeviceRotationCoordinator without any AVCaptureDevice";
            } else {
                m_rotationCoordinator = [[AVCaptureDeviceRotationCoordinator alloc]
                    initWithDevice:captureDevice
                    previewLayer:nil];
            }
        }
#ifdef Q_OS_IOS
        else {
            // If we're running iOS 16 or older, we need to register for UIDeviceOrientation changes.
            if (!m_receivingUiDeviceOrientationNotifications)
                [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
            m_receivingUiDeviceOrientationNotifications = true;
        }
#endif
    } else {
        if (@available(macOS 14.0, iOS 17.0, *)) {
            if (m_rotationCoordinator)
                [m_rotationCoordinator release];
            m_rotationCoordinator = nullptr;
        }

#ifdef Q_OS_IOS
        if (m_receivingUiDeviceOrientationNotifications)
            [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
        m_receivingUiDeviceOrientationNotifications = false;
#endif
    }
}

// Gets the current rotationfor this QAVFCamera.
// Returns the result in degrees, 0 to 360.
// Will always return a result that is divisible by 90.
int QAVFCamera::getCurrentRotationAngleDegrees() const
{
#ifdef Q_OS_IOS
    if (!m_rotationCoordinator && m_receivingUiDeviceOrientationNotifications)
        return 0;
#else
    if (!m_rotationCoordinator)
        return 0;
#endif

    if (@available(macOS 14.0, iOS 17.0, *)) {
        // This code assumes that AVCaptureDeviceRotationCoordinator.videoRotationAngleForHorizonLevelCapture
        // returns degrees that are divisible by 90. This has been the case during testing.
        //
        // TODO: Some rotations are not valid for preview on some devices (such as
        // iPhones not being allowed to have an upside-down window). This usage of the
        // rotation coordinator will still return it as a valid preview rotation, and
        // might cause bugs on iPhone previews.
        if (m_rotationCoordinator)
            return static_cast<int>(std::round(
                m_rotationCoordinator.videoRotationAngleForHorizonLevelCapture));
    }
#ifdef Q_OS_IOS
    else {
        AVCaptureDevice *captureDevice = device();
        if (captureDevice && m_receivingUiDeviceOrientationNotifications) {
            // TODO: The new orientation can be FlatFaceDown or FlatFaceUp, neither of
            // which should trigger a camera re-orientation. We can't store the previously
            // valid orientation because this method has to be const. Currently
            // this means orientation of the camera might be incorrect when laying the device
            // down flat. Ideally we might want to store this orientation as a global
            // variable somewhere.
            const UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];

            const AVCaptureDevicePosition captureDevicePosition = captureDevice.position;

            // If the position is set to PositionUnspecified, it's a good indication that
            // this is an external webcam. In which case, don't apply any rotation.
            if (captureDevicePosition == AVCaptureDevicePositionBack)
                return qt_ui_device_orientation_to_rotation_angle_degrees(orientation);
            else if (captureDevicePosition == AVCaptureDevicePositionFront)
                return 360 - qt_ui_device_orientation_to_rotation_angle_degrees(orientation);
        }
    }
#endif

    return 0;
}

QT_END_NAMESPACE

#include "moc_qavfcamera_p.cpp"
