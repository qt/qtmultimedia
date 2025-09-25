// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qavfcamera_p.h>

#include <QtCore/qscopeguard.h>
#include <QtCore/private/qcore_mac_p.h>

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

namespace {

[[nodiscard]] QAVFSampleBufferDelegateTransform surfaceTransform(
    const QFFmpeg::AvfCameraRotationTracker *rotationTracker,
    const AVCaptureConnection *connection)
{
    QAVFSampleBufferDelegateTransform transform = {};

    int captureAngle = 0;

    if (rotationTracker != nullptr) {
        captureAngle = rotationTracker->rotationDegrees();

        bool cameraIsFrontFacing =
            rotationTracker->avCaptureDevice() != nullptr
            && rotationTracker->avCaptureDevice().position == AVCaptureDevicePositionFront;
        if (cameraIsFrontFacing)
            transform.presentationTransform.mirroredHorizontallyAfterRotation = true;
    }

    // In some situations, AVFoundation can set the AVCaptureConnection.videoRotationAgngle
    // implicity and start rotating the pixel buffer before handing it back
    // to us. In this case we want to account for this during preview and capture.
    //
    // This code assumes that AVCaptureConnection.videoRotationAngle returns degrees
    // that are divisible by 90. This has been the case during testing.
    int connectionAngle = 0;
    if (connection) {
        if (@available(macOS 14.0, iOS 17.0, *))
            connectionAngle = std::lround(connection.videoRotationAngle);

        if (connection.videoMirrored)
            transform.surfaceTransform.mirroredHorizontallyAfterRotation = true;
    }

    transform.surfaceTransform.rotation = qVideoRotationFromDegrees(captureAngle - connectionAngle);

    return transform;
}

// This function may return a nullptr if no suitable format was found.
// The format may not be supported by FFmpeg.
[[nodiscard]] static AVCaptureDeviceFormat* findSuitableAvCaptureDeviceFormat(
    AVCaptureDevice *avCaptureDevice,
    const QCameraFormat &format)
{
    Q_ASSERT(avCaptureDevice != nullptr);
    Q_ASSERT(!format.isNull());

    // First we try to find a device format equivalent to QCameraFormat
    // that is supported by FFmpeg.
    AVCaptureDeviceFormat *newDeviceFormat = qt_convert_to_capture_device_format(
        avCaptureDevice,
        format,
        &QFFmpeg::isCVFormatSupported);

    // If we can't find a AVCaptureDeviceFormat supported by FFmpeg,
    // fall back to one not supported by FFmpeg.
    if (!newDeviceFormat)
        newDeviceFormat = qt_convert_to_capture_device_format(avCaptureDevice, format);

    return newDeviceFormat;
}

[[nodiscard]] static q23::expected<CvPixelFormat, QString> tryFindVideoDataOutputPixelFormat(
    QVideoFrameFormat::PixelFormat cameraPixelFormat,
    CvPixelFormat inputCvPixFormat,
    AVCaptureVideoDataOutput *avCaptureVideoDataOutput)
{
    Q_ASSERT(cameraPixelFormat != QVideoFrameFormat::PixelFormat::Format_Invalid);
    Q_ASSERT(inputCvPixFormat != CvPixelFormatInvalid);
    Q_ASSERT(avCaptureVideoDataOutput != nullptr);

    using namespace Qt::Literals::StringLiterals;

    if (avCaptureVideoDataOutput.availableVideoCVPixelFormatTypes.count == 0)
        return q23::unexpected{
            u"AVCaptureVideoDataOutput.availableVideoCVPixelFormatTypes is empty"_s };

    auto bestScore = MinAVScore;
    NSNumber *bestFormat = nullptr;
    for (NSNumber *cvPixFmtNumber in avCaptureVideoDataOutput.availableVideoCVPixelFormatTypes) {
        const CvPixelFormat cvPixFmt = [cvPixFmtNumber unsignedIntValue];
        const QVideoFrameFormat::PixelFormat pixFmt = QAVFHelpers::fromCVPixelFormat(cvPixFmt);
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

        if (score > bestScore) {
            bestScore = score;
            bestFormat = cvPixFmtNumber;
        }
    }

    if (bestScore < DefaultAVScore)
        qWarning() << "QAVFCamera::tryFindVideoDataOutputPixelFormat: "
                      "Cannot find hw FFmpeg supported cv pix format";

    return [bestFormat unsignedIntValue];
}

}

QAVFCamera::QAVFCamera(QCamera *parent)
    : QAVFCameraBase(parent)
{
    m_avCaptureSession = [[AVCaptureSession alloc] init];

    auto frameHandler = [this](QVideoFrame frame) {
        emit newVideoFrame(frame);
    };

    m_qAvfSampleBufferDelegate = [[QAVFSampleBufferDelegate alloc] initWithFrameHandler:frameHandler];

    [m_qAvfSampleBufferDelegate setTransformationProvider:
        [this](const AVCaptureConnection *connection) {
            const AvfCameraRotationTracker *rotationTracker = nullptr;
            if (m_qAvfCameraRotationTracker.has_value())
                rotationTracker = &m_qAvfCameraRotationTracker.value();

            return surfaceTransform(
                rotationTracker,
                connection);
        }];

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
    clearAvCaptureSessionInputDevice();

    [m_qAvfSampleBufferDelegate release];
    [m_avCaptureVideoDataOutput release];
    [m_avCaptureSession release];
    dispatch_release(m_delegateQueue);

    clearRotationTracking();
}

void QAVFCamera::clearAvCaptureSessionInputDevice()
{
    if (m_avCaptureDeviceVideoInput) {
        [m_avCaptureSession removeInput:m_avCaptureDeviceVideoInput];
        [m_avCaptureDeviceVideoInput release];
        m_avCaptureDeviceVideoInput = nullptr;
    }
}

[[nodiscard]] q23::expected<void, QString> QAVFCamera::setupAvCaptureSessionInputDevice(
    AVCaptureDevice *avCaptureDevice)
{
    // AVCaptureDeviceInput.deviceInputWithDevice will implicitly ask for permission
    // and present a dialogue to the end-user.
    // Permission should only be requested explicitly through QPermission API.
    Q_ASSERT(checkCameraPermission());
    Q_ASSERT(avCaptureDevice != nullptr);
    Q_ASSERT(m_avCaptureSession != nullptr);
    Q_ASSERT(m_avCaptureDeviceVideoInput == nullptr);

    using namespace Qt::Literals::StringLiterals;

    QMacAutoReleasePool autoReleasePool;

    NSError* creationError = nullptr;
    AVCaptureDeviceInput *deviceInput = [AVCaptureDeviceInput
        deviceInputWithDevice:avCaptureDevice
                        error:&creationError];
    if (creationError != nullptr)
        return q23::unexpected(QString::fromNSString(creationError.localizedDescription));

    if (![m_avCaptureSession canAddInput:deviceInput])
        return q23::unexpected{
            u"Cannot attach AVCaptureDeviceInput to AVCaptureSession"_s };

    [deviceInput retain];

    [m_avCaptureSession addInput:deviceInput];

    m_avCaptureDeviceVideoInput = deviceInput;

    return {};
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

        // TODO: Tear down any open resources if we fail to go active,
        // and propagate error upwards so we can properly signal errorOcurred.
        AVCaptureDevice *avCaptureDevice = QAVFCameraBase::tryGetAvCaptureDevice(m_cameraDevice);
        if (avCaptureDevice == nullptr)
            return;

        q23::expected<void, QString> setupInputResult = setupAvCaptureSessionInputDevice(
            avCaptureDevice);
        if (!setupInputResult) {
            qWarning()
                << "QAVFCamera::onActiveChanged: Failed to go active:"
                << setupInputResult.error();
            return;
        }

        // According to the doc, the capture device must be locked before
        // startRunning to prevent the format we set to be overridden by the
        // session preset.
        [m_avCaptureDeviceVideoInput.device lockForConfiguration:nil];
        [m_avCaptureSession startRunning];
        [m_avCaptureDeviceVideoInput.device unlockForConfiguration];
    } else {
        [m_avCaptureSession stopRunning];

        clearAvCaptureSessionInputDevice();
    }

    // If the camera becomes active, we want to start tracking the rotation of the camera
    updateRotationTracking();
}

void QAVFCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    m_qMediaCaptureSession = session ? session->captureSession() : nullptr;
}

void QAVFCamera::onCameraDeviceChanged(const QCameraDevice &newCameraDevice)
{
    // Using this configuration transaction, we can clear up resources and establish new ones
    // without having to do slow and synchronous calls to AVCaptureSession.stopRunning and
    // startRunning.
    [m_avCaptureSession beginConfiguration];
    QScopeGuard endConfigGuard{ [&] {
        [m_avCaptureSession commitConfiguration];
    } };

    clearAvCaptureSessionInputDevice();

    // If the new QCameraDevice does not point to any physical device,
    // make sure we clear resources and shut down the capture-session.
    if (newCameraDevice.isNull() || !checkCameraPermission())
        return;

    if ([m_avCaptureSession isRunning]) {
        // TODO: Tear down any open resources if we fail to go active,
        // and propagate error upwards so we can properly signal errorOcurred.
        // Also shut down the AVCaptureSession.
        AVCaptureDevice *avCaptureDevice = QAVFCameraBase::tryGetAvCaptureDevice(newCameraDevice);
        if (avCaptureDevice == nullptr)
            return;

        q23::expected<void, QString> setupInputResult = setupAvCaptureSessionInputDevice(
            avCaptureDevice);
        if (!setupInputResult) {
            qWarning()
                << "QAVFCamera::onCameraDeviceChanged: Failed to go active:"
                << setupInputResult.error();
            return;
        }
    }

    // When we change camera, we need to clear up the existing
    // rotation tracker state and set up the new one.
    updateRotationTracking();
}

bool QAVFCamera::tryApplyCameraFormat(const QCameraFormat &format)
{
    // TODO: In the future, we should be able to return false if we failed
    // to apply the format.
    //
    // TODO: There is a race condition here where we are writing directly
    // to the sample-buffer-delegate of an on-going AVCaptureSession with no
    // locks. In the future, we should determine if we should accept the format
    // ahead of time. If we are in an-ongoing capture-session, we should
    // restart the current session with the new format.
    updateCameraFormat(format);
    return true;
}

void QAVFCamera::updateCameraFormat(const QCameraFormat &newFormat)
{
    m_framePixelFormat = QVideoFrameFormat::Format_Invalid;
    m_cvPixelFormat = CvPixelFormatInvalid;

    AVCaptureDevice *avCaptureDevice = device();
    if (!avCaptureDevice)
        return;

    AVCaptureDeviceFormat *avCaptureDeviceFormat = findSuitableAvCaptureDeviceFormat(
        avCaptureDevice,
        newFormat);
    // If we can't find any suitable AVCaptureDeviceFormat,
    // then we cannot apply this QCameraFormat.
    if (!avCaptureDeviceFormat) {
        qWarning() << "QAVFCamera::updateCameraFormat: Unable to find any suitable "
                      "AVCaptureDeviceFormat when attempting to apply QCameraFormat";
        return;
    }

    const CvPixelFormat captureDeviceCvFormat =
        CMVideoFormatDescriptionGetCodecType(avCaptureDeviceFormat.formatDescription);

    // We cannot always use the AVCaptureDeviceFormat directly,
    // so we look for a pixel format that we can use for the output.
    // The AVFoundation internals will take care of converting the
    // pixel formats to what we require.
    q23::expected<CvPixelFormat, QString> outputPixelFormatResult =
        tryFindVideoDataOutputPixelFormat(
            newFormat.pixelFormat(),
            captureDeviceCvFormat,
            m_avCaptureVideoDataOutput);
    if (!outputPixelFormatResult) {
        qWarning()
            << "QAVFCamera::updateCameraFormat: Unable to find suitable output CvPixelFormat when "
               "applying QCameraFormat:"
            << outputPixelFormatResult.error();
        return;
    }

    const CvPixelFormat outputCvPixelFormat = *outputPixelFormatResult;

    // If the input AVCaptureDevice pixel format does not match
    // the output pixel format, the AVFoundation internals will perform
    // the conversion for us. This likely incurs performance overhead.
    if (captureDeviceCvFormat != outputCvPixelFormat) {
        qCWarning(qLcCamera) << "Output CV format differs with capture device format!"
                             << outputCvPixelFormat << cvFormatToString(outputCvPixelFormat)
                             << "vs"
                             << captureDeviceCvFormat << cvFormatToString(captureDeviceCvFormat);
    }

    const AVPixelFormat avPixelFormat = av_map_videotoolbox_format_to_pixfmt(outputCvPixelFormat);

    HWAccelUPtr hwAccel;

    if (avPixelFormat == AV_PIX_FMT_NONE) {
        qCWarning(qLcCamera) << "Videotoolbox doesn't support cvPixelFormat:" << outputCvPixelFormat
                             << cvFormatToString(outputCvPixelFormat)
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

    // Apply the format to the AVCaptureDevice.
    qt_set_active_format(avCaptureDevice, avCaptureDeviceFormat, false);

    Q_ASSERT(m_avCaptureVideoDataOutput);
    NSDictionary *outputSettings = @{
        (NSString *)kCVPixelBufferPixelFormatTypeKey
            : [NSNumber numberWithUnsignedInteger:outputCvPixelFormat],
        (NSString *)kCVPixelBufferMetalCompatibilityKey : @true
    };
    m_avCaptureVideoDataOutput.videoSettings = outputSettings;

    Q_ASSERT(m_qAvfSampleBufferDelegate);
    [m_qAvfSampleBufferDelegate setHWAccel:std::move(hwAccel)];
    [m_qAvfSampleBufferDelegate setVideoFormatFrameRate:newFormat.maxFrameRate()];

    m_cvPixelFormat = outputCvPixelFormat;
    m_framePixelFormat = QAVFHelpers::fromCVPixelFormat(outputCvPixelFormat);
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

    const AvfCameraRotationTracker *rotationTracker = nullptr;
    if (m_qAvfCameraRotationTracker.has_value())
        rotationTracker = &m_qAvfCameraRotationTracker.value();

    const AVCaptureConnection *connection = m_avCaptureVideoDataOutput ?
        [m_avCaptureVideoDataOutput connectionWithMediaType:AVMediaTypeVideo] :
        nullptr;

    const QAVFSampleBufferDelegateTransform transform = surfaceTransform(
        rotationTracker,
        connection);
    result.setRotation(transform.surfaceTransform.rotation);
    result.setMirrored(transform.surfaceTransform.mirroredHorizontallyAfterRotation);

    result.setColorRange(QAVFHelpers::colorRangeForCVPixelFormat(m_cvPixelFormat));

    return result;
}

// Clears or sets up rotation tracking based on isActive()
void QAVFCamera::updateRotationTracking()
{
    // If the camera is active, it should have either a RotationCoordinator
    // or start listening for UIDeviceOrientation changes.
    if (isActive()) {
        AVCaptureDevice *captureDevice = device();
        if (captureDevice)
            m_qAvfCameraRotationTracker = QFFmpeg::AvfCameraRotationTracker(captureDevice);
        else
            qCDebug(qLcCamera)
                << "Attempted to setup AVCaptureDeviceRotationCoordinator without any "
                   "AVCaptureDevice";
    } else
        clearRotationTracking();
}

void QAVFCamera::clearRotationTracking() {
    m_qAvfCameraRotationTracker = std::nullopt;
}

// Gets the current rotationfor this QAVFCamera.
// Returns the result in degrees, 0 to 360.
// Will always return a result that is divisible by 90.
int QAVFCamera::getCurrentRotationAngleDegrees() const
{
    if (m_qAvfCameraRotationTracker.has_value())
        return m_qAvfCameraRotationTracker.value().rotationDegrees();
    else
        return 0;
}

QT_END_NAMESPACE

#include "moc_qavfcamera_p.cpp"
