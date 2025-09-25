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
    m_delegateQueue = dispatch_queue_create("qt_camera_queue", nullptr);
}

QAVFCamera::~QAVFCamera()
{
    clearAvCaptureSessionInputDevice();
    clearAvCaptureVideoDataOutput();
    clearRotationTracking();

    [m_avCaptureSession release];
    dispatch_release(m_delegateQueue);
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

void QAVFCamera::clearAvCaptureVideoDataOutput()
{
    if (m_avCaptureVideoDataOutput != nullptr) {
        [m_avCaptureSession removeOutput:m_avCaptureVideoDataOutput];
        [m_avCaptureVideoDataOutput release];
        m_avCaptureVideoDataOutput = nullptr;
    }
    if (m_qAvfSampleBufferDelegate != nullptr) {
        [m_qAvfSampleBufferDelegate release];
        m_qAvfSampleBufferDelegate = nullptr;
    }
}

q23::expected<void, QString> QAVFCamera::setupAvCaptureVideoDataOutput(
    AVCaptureDevice *avCaptureDevice)
{
    Q_ASSERT(avCaptureDevice);

    using namespace Qt::Literals::StringLiterals;

    QMacAutoReleasePool autoReleasePool;

    // Setup the delegate object for which we receive video frames.
    auto frameHandler = [this](QVideoFrame frame) {
        emit newVideoFrame(frame);
    };

    QAVFSampleBufferDelegate *sampleBufferDelegate = [[[QAVFSampleBufferDelegate alloc]
        initWithFrameHandler:frameHandler]
        autorelease];
    // The transformProvider callable needs to be copyable, so we use a shared-ptr here.
    auto rotationTracker = std::make_shared<QFFmpeg::AvfCameraRotationTracker>(avCaptureDevice);
    [sampleBufferDelegate setTransformationProvider:
        [rotationTracker](const AVCaptureConnection *connection) {
            return surfaceTransform(
                rotationTracker.get(),
                connection);
        }];

    // Create the AVCaptureOutput object with our delegate object and background-thread.
    AVCaptureVideoDataOutput *avCaptureVideoDataOutput = [[[AVCaptureVideoDataOutput alloc]
        init]
        autorelease];
    [avCaptureVideoDataOutput setSampleBufferDelegate:sampleBufferDelegate
                                                queue:m_delegateQueue];

    if (![m_avCaptureSession canAddOutput:avCaptureVideoDataOutput])
        return q23::unexpected{
            u"Unable to connect AVCaptureVideoDataOutput to AVCaptureSession"_s };

    [m_avCaptureSession addOutput:avCaptureVideoDataOutput];
    m_qAvfSampleBufferDelegate = [sampleBufferDelegate retain];
    m_avCaptureVideoDataOutput = [avCaptureVideoDataOutput retain];

    return {};
}

// This function writes to the AVCaptureVideoDataOutput and QAVFSampleBufferDelegate
// objects directly. Don't use this function if these objects are already
// connected to a running AVCaptureSession.
q23::expected<void, QString> QAVFCamera::tryApplyFormatToCaptureSession(
    AVCaptureDevice *avCaptureDevice,
    AVCaptureDeviceFormat *avCaptureDeviceFormat,
    const QCameraFormat &newCameraFormat)
{
    Q_ASSERT(avCaptureDevice != nullptr);
    Q_ASSERT(avCaptureDeviceFormat != nullptr);
    Q_ASSERT(!newCameraFormat.isNull());

    const CvPixelFormat captureDeviceCvFormat = CMVideoFormatDescriptionGetCodecType(
        avCaptureDeviceFormat.formatDescription);

    // We cannot always use the AVCaptureDeviceFormat directly,
    // so we look for a pixel format that we can use for the output.
    // The AVFoundation internals will take care of converting the
    // pixel formats to what we require.
    q23::expected<CvPixelFormat, QString> outputPixelFormatResult =
        tryFindVideoDataOutputPixelFormat(
            newCameraFormat.pixelFormat(),
            captureDeviceCvFormat,
            m_avCaptureVideoDataOutput);
    if (!outputPixelFormatResult)
        return q23::unexpected{ std::move(outputPixelFormatResult.error()) };

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
                             << "Camera pix format:" << newCameraFormat.pixelFormat();
    } else {
        hwAccel = HWAccel::create(AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
        qCDebug(qLcCamera) << "Create VIDEOTOOLBOX hw context" << hwAccel.get() << "for camera";
    }

    // Apply the format to our capture session and QAVFCamera.

    if (hwAccel) {
        hwAccel->createFramesContext(avPixelFormat, adjustedResolution(newCameraFormat));
        m_hwPixelFormat = hwAccel->hwFormat();
    } else {
        m_hwPixelFormat = AV_PIX_FMT_NONE;
    }

    Q_ASSERT(m_avCaptureVideoDataOutput != nullptr);
    [m_qAvfSampleBufferDelegate setHWAccel:std::move(hwAccel)];
    [m_qAvfSampleBufferDelegate setVideoFormatFrameRate:newCameraFormat.maxFrameRate()];

    Q_ASSERT(m_avCaptureVideoDataOutput != nullptr);
    NSDictionary *outputSettings = @{
        (NSString *)kCVPixelBufferPixelFormatTypeKey
            : [NSNumber numberWithUnsignedInt:outputCvPixelFormat],
        (NSString *)kCVPixelBufferMetalCompatibilityKey : @true
    };
    m_avCaptureVideoDataOutput.videoSettings = outputSettings;

    qt_set_active_format(avCaptureDevice, avCaptureDeviceFormat, false);

    m_framePixelFormat = QAVFHelpers::fromCVPixelFormat(outputCvPixelFormat);
    m_cvPixelFormat = outputCvPixelFormat;

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

        q23::expected<void, QString> setupOutputResult = setupAvCaptureVideoDataOutput(
            avCaptureDevice);
        if (!setupOutputResult) {
            qWarning()
                << "QAVFCamera::onActiveChanged: Failed to go establish output:"
                << setupOutputResult.error();
            return;
        }

        // According to the doc, the capture device must be locked before
        // startRunning to prevent the format we set to be overridden by the
        // session preset.
        AVFConfigurationLock avCaptureDeviceLock { avCaptureDevice };
        if (!avCaptureDeviceLock) {
            qWarning() << "QAVFCamera::onActiveChanged: Failed to lock AVCaptureDevice when trying "
                          "to go active";
            return;
        }

        [m_avCaptureSession startRunning];
    } else {
        [m_avCaptureSession stopRunning];

        clearAvCaptureSessionInputDevice();
        clearAvCaptureVideoDataOutput();
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
    clearAvCaptureVideoDataOutput();

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

        q23::expected<void, QString> setupOutputResult = setupAvCaptureVideoDataOutput(
            avCaptureDevice);
        if (!setupOutputResult) {
            qWarning()
                << "QAVFCamera::onCameraDeviceChanged: Failed to go active:"
                << setupOutputResult.error();
            return;
        }

    }

    // When we change camera, we need to clear up the existing
    // rotation tracker state and set up the new one.
    updateRotationTracking();
}

bool QAVFCamera::tryApplyCameraFormat(const QCameraFormat &newCameraFormat)
{
    m_framePixelFormat = QVideoFrameFormat::Format_Invalid;
    m_cvPixelFormat = CvPixelFormatInvalid;

    // TODO: It's currently unclear whether we should accept the QCameraFormat
    // if the QCameraDevice is currently not connected.
    AVCaptureDevice *avCaptureDevice = device();
    if (!avCaptureDevice)
        return false;

    AVCaptureDeviceFormat *avCaptureDeviceFormat = findSuitableAvCaptureDeviceFormat(
        avCaptureDevice,
        newCameraFormat);
    // If we can't find any suitable AVCaptureDeviceFormat,
    // then we cannot apply this QCameraFormat.
    if (!avCaptureDeviceFormat) {
        qWarning() << "QAVFCamera::tryApplyCameraFormat: Unable to find any suitable "
                      "AVCaptureDeviceFormat when attempting to apply QCameraFormat";
        return false;
    }

    // If we are not currently active, we don't need to do anything. We will apply the format
    // to the capture-session when we try to go active later.
    //
    // TODO: Determine if the incoming QCameraFormat resolves to the same formats
    // that we are already using, in which case this function can be a no-op.
    if (![m_avCaptureSession isRunning])
        return true;

    // We are active, so we need to reconfigure the entire capture-session with the
    // new format.
    AVFConfigurationLock avCaptureDeviceLock { avCaptureDevice };
    if (!avCaptureDeviceLock) {
        qWarning() << "Failed to lock AVCaptureDevice when trying to go active.";
        return false;
    }
    [m_avCaptureSession beginConfiguration];
    QScopeGuard endConfigGuard { [this]() {
        [m_avCaptureSession commitConfiguration];
    } };

    clearAvCaptureSessionInputDevice();
    clearAvCaptureVideoDataOutput();

    q23::expected<void, QString> setupInputResult = setupAvCaptureSessionInputDevice(
        avCaptureDevice);
    if (!setupInputResult) {
        qWarning()
            << "Failed to apply QCameraFormat to active AVCaptureSession: "
            << setupInputResult.error();
        return false;
    }

    q23::expected<void, QString> setupOutputResult = setupAvCaptureVideoDataOutput(
        avCaptureDevice);
    if (!setupOutputResult) {
        qWarning()
            << "Failed to apply QCameraFormat to active AVCaptureSession: "
            << setupOutputResult.error();
        return false;
    }

    q23::expected<void, QString> applyFormatResult = tryApplyFormatToCaptureSession(
        avCaptureDevice,
        avCaptureDeviceFormat,
        newCameraFormat);
    if (!applyFormatResult) {
        qWarning()
            << "QAVFCamera::updateCameraFormat: Failed to apply QCameraFormat to "
               "AVCaptureSession:"
            << applyFormatResult.error();
        return false;
    }

    return true;
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
