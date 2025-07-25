// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qavfcamera_p.h>

#include <QtCore/qscopeguard.h>

#include <QtFFmpegMediaPluginImpl/private/qavfsamplebufferdelegate_p.h>

#include <QtMultimedia/private/qavfcameradebug_p.h>
#include <QtMultimedia/private/qavfcamerautility_p.h>
#include <QtMultimedia/private/qavfhelpers_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtMultimedia/private/qplatformmediacapture_p.h>

#define AVMediaType XAVMediaType
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
    m_avCaptureSession = [[AVCaptureSession alloc] init];

    auto frameHandler = [this](QVideoFrame frame) {
        frame.setMirrored(isFrontCamera()); // presentation mirroring
        emit newVideoFrame(frame);
    };

    m_qAvfSampleBufferDelegate = [[QAVFSampleBufferDelegate alloc] initWithFrameHandler:frameHandler];

    [m_qAvfSampleBufferDelegate setTransformationProvider:[this] { return surfaceTransform(); }];

    // Configure video output
    m_avCaptureVideoDataOutput = [[AVCaptureVideoDataOutput alloc] init];
    m_delegateQueue = dispatch_queue_create("vf_queue", nullptr);
    [m_avCaptureVideoDataOutput setSampleBufferDelegate:m_qAvfSampleBufferDelegate
                                         queue:m_delegateQueue];

    // Hook output object to our capture session.
    [m_avCaptureSession beginConfiguration];
    [m_avCaptureSession addOutput:m_avCaptureVideoDataOutput];
    [m_avCaptureSession commitConfiguration];
}

QAVFCamera::~QAVFCamera()
{
    [m_qAvfSampleBufferDelegate release];
    [m_avCaptureDeviceVideoInput release];
    [m_avCaptureVideoDataOutput release];
    [m_avCaptureSession release];
    dispatch_release(m_delegateQueue);

    clearRotationTracking();
}

void QAVFCamera::refreshAvCaptureSessionInputDevice()
{
    // AVCaptureDeviceInput deviceInputWithDevice will implicitly ask for permission.
    // Only the user should request permissions.
    Q_ASSERT(checkCameraPermission());

    [m_avCaptureSession beginConfiguration];
    const QScopeGuard endConfigGuard{ [&]() {
        [m_avCaptureSession commitConfiguration];
    }};

    if (m_avCaptureDeviceVideoInput) {
        [m_avCaptureSession removeInput:m_avCaptureDeviceVideoInput];
        [m_avCaptureDeviceVideoInput release];
        m_avCaptureDeviceVideoInput = nullptr;
    }

    AVCaptureDevice *videoDevice = device();
    if (!videoDevice)
        return;

    m_avCaptureDeviceVideoInput = [AVCaptureDeviceInput
                    deviceInputWithDevice:videoDevice
                    error:nil];
    if (m_avCaptureDeviceVideoInput && [m_avCaptureSession canAddInput:m_avCaptureDeviceVideoInput]) {
        [m_avCaptureDeviceVideoInput retain];
        [m_avCaptureSession addInput:m_avCaptureDeviceVideoInput];
    } else {
        qWarning() << "Failed to create video device input";
    }
}

void QAVFCamera::onActiveChanged(bool active)
{
    if (active) {
        // We should never try to go active if we don't already have
        // permissions, as refreshAvCaptureSessionInputDevice() will
        // implicitly trigger a user permission request and freeze the
        // program. Permissions should only be requested through
        // QPermissions.
        Q_ASSERT(checkCameraPermission());

        // The AVCaptureSession might not yet have been configured with the
        // AVCaptureDevice, if camera permissions was not granted when applying
        // the device to this QCamera. Set it up now.
        refreshAvCaptureSessionInputDevice();

        // According to the doc, the capture device must be locked before
        // startRunning to prevent the format we set to be overridden by the
        // session preset.
        [m_avCaptureDeviceVideoInput.device lockForConfiguration:nil];
        [m_avCaptureSession startRunning];
        [m_avCaptureDeviceVideoInput.device unlockForConfiguration];
    } else {
        [m_avCaptureSession stopRunning];
    }

    // If the camera becomes active, we want to start tracking the rotation of the camera
    updateRotationTracking();
}

void QAVFCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    m_qMediaCaptureSession = session ? session->captureSession() : nullptr;
}

void QAVFCamera::onCameraDeviceChanged(const QCameraDevice &device)
{
    if (device.isNull() || !checkCameraPermission())
        return;

    refreshAvCaptureSessionInputDevice();

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

    [m_qAvfSampleBufferDelegate setHWAccel:std::move(hwAccel)];
    [m_qAvfSampleBufferDelegate setVideoFormatFrameRate:newFormat.maxFrameRate()];
}

void QAVFCamera::setPixelFormat(QVideoFrameFormat::PixelFormat cameraPixelFormat,
                                uint32_t inputCvPixFormat)
{
    m_cvPixelFormat = CvPixelFormatInvalid;

    auto bestScore = MinAVScore;
    NSNumber *bestFormat = nullptr;
    for (NSNumber *cvPixFmtNumber in m_avCaptureVideoDataOutput.availableVideoCVPixelFormatTypes) {
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
    m_avCaptureVideoDataOutput.videoSettings = outputSettings;

    m_cvPixelFormat = [bestFormat unsignedIntValue];
}

QSize QAVFCamera::adjustedResolution(const QCameraFormat& newFormat) const
{
#ifdef Q_OS_MACOS
    return newFormat.resolution();
#else
    // Check, that we have matching dimesnions.
    QSize resolution = newFormat.resolution();
    AVCaptureConnection *connection = [m_avCaptureVideoDataOutput connectionWithMediaType:AVMediaTypeVideo];
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
    result.setMirrored(transform.mirroredHorizontallyAfterRotation);

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
    const AVCaptureConnection *connection = m_avCaptureVideoDataOutput ?
        [m_avCaptureVideoDataOutput connectionWithMediaType:AVMediaTypeVideo] :
        nullptr;
    if (connection) {
        if (@available(macOS 14.0, iOS 17.0, *))
            connectionAngle = static_cast<int>(std::round(connection.videoRotationAngle));

        transform.mirroredHorizontallyAfterRotation = connection.videoMirrored;
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
            if (m_avRotationCoordinator)
                [m_avRotationCoordinator release];
            m_avRotationCoordinator = nullptr;

            AVCaptureDevice* captureDevice = device();
            if (!captureDevice) {
                qCDebug(qLcCamera) << "attempted to setup AVCaptureDeviceRotationCoordinator without any AVCaptureDevice";
            } else {
                m_avRotationCoordinator = [[AVCaptureDeviceRotationCoordinator alloc]
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
        clearRotationTracking();
    }
}

void QAVFCamera::clearRotationTracking() {
    if (@available(macOS 14.0, iOS 17.0, *)) {
        if (m_avRotationCoordinator)
            [m_avRotationCoordinator release];
        m_avRotationCoordinator = nullptr;
    }

#ifdef Q_OS_IOS
    if (m_receivingUiDeviceOrientationNotifications)
        [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
    m_receivingUiDeviceOrientationNotifications = false;
#endif
}

// Gets the current rotationfor this QAVFCamera.
// Returns the result in degrees, 0 to 360.
// Will always return a result that is divisible by 90.
int QAVFCamera::getCurrentRotationAngleDegrees() const
{
    if (@available(macOS 14.0, iOS 17.0, *)) {
        // This code assumes that AVCaptureDeviceRotationCoordinator.videoRotationAngleForHorizonLevelCapture
        // returns degrees that are divisible by 90. This has been the case during testing.
        //
        // TODO: Some rotations are not valid for preview on some devices (such as
        // iPhones not being allowed to have an upside-down window). This usage of the
        // rotation coordinator will still return it as a valid preview rotation, and
        // might cause bugs on iPhone previews.
        if (m_avRotationCoordinator)
            return static_cast<int>(std::round(
                m_avRotationCoordinator.videoRotationAngleForHorizonLevelCapture));
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
