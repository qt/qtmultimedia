// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qavfcamera_p.h>

#include <QtCore/qscopeguard.h>
#include <QtCore/private/qcore_mac_p.h>

#include <QtFFmpegMediaPluginImpl/private/qavfcapturephotooutputdelegate_p.h>
#include <QtFFmpegMediaPluginImpl/private/qavfsamplebufferdelegate_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegdarwinintegrationfactory_p.h>

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

namespace QFFmpeg {

namespace {

[[nodiscard]] AVCaptureFlashMode toAvfFlashMode(QCamera::FlashMode flashMode)
{
    switch (flashMode) {
    case QCamera::FlashMode::FlashOff:
        return AVCaptureFlashModeOff;
    case QCamera::FlashMode::FlashAuto:
        return AVCaptureFlashModeAuto;
    case QCamera::FlashMode::FlashOn:
        return AVCaptureFlashModeOn;
    }
    return AVCaptureFlashModeOff;
}

[[nodiscard]] bool checkAvCapturePhotoFormatSupport(AVCapturePhotoOutput *output, int cvPixelFormat)
{
    Q_ASSERT(output);
    NSArray<NSNumber *> *supportedFormats = output.availablePhotoPixelFormatTypes;
    for (NSNumber *format : supportedFormats) {
        if (format.intValue == cvPixelFormat)
            return true;
    }
    return false;
}

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
    Q_ASSERT(avCaptureVideoDataOutput);

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

} // Anonymous namespace

std::unique_ptr<QPlatformCamera> makeQAvfCamera(QCamera &parent)
{
    return std::make_unique<QAVFCamera>(parent);
}

QAVFCamera::QAVFCamera(QCamera &parent)
    : QAVFCameraBase(&parent)
{
    m_avCaptureSession = AVFScopedPointer{ [[AVCaptureSession alloc] init] };
    m_delegateQueue = AVFScopedPointer<dispatch_queue_t>{
        dispatch_queue_create("qt_camera_queue", DISPATCH_QUEUE_SERIAL) };

    m_avCapturePhotoOutput = AVFScopedPointer{ [AVCapturePhotoOutput new] };

    // TODO: Handle error where we cannot add AVCapturePhotoOutput to session,
    // and report back to QImageCapture that we are unable to take a photo.
    if ([m_avCaptureSession canAddOutput:m_avCapturePhotoOutput])
        [m_avCaptureSession addOutput:m_avCapturePhotoOutput];
}

QAVFCamera::~QAVFCamera()
{
    using namespace Qt::Literals::StringLiterals;

    [m_avCaptureSession stopRunning];

    clearAvCaptureSessionInputDevice();
    // Clearing the output will flush jobs on the dispatch queue running on a worker threadpool.
    clearAvCaptureVideoDataOutput();
    clearRotationTracking();

    // If there is currently an on-going still photo capture, we will
    // automatically discard any future results when this QCamera object
    // is destroyed and the connection to the QAVFStillPhotoNotifier is
    // removed. We emit a signal that still-photo capture failed, so
    // that QImageCapture can cancel any pending still-photo capture jobs.
    if (stillPhotoCaptureInProgress()) {
        emit stillPhotoFailed(
            QImageCapture::Error::ResourceError,
            u"Camera object was destroyed before still photo capture was completed"_s);
    }
}

void QAVFCamera::clearAvCaptureSessionInputDevice()
{
    if (m_avCaptureDeviceVideoInput) {
        [m_avCaptureSession removeInput:m_avCaptureDeviceVideoInput];
        m_avCaptureDeviceVideoInput.reset();
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
    Q_ASSERT(m_avCaptureSession);
    Q_ASSERT(!m_avCaptureDeviceVideoInput);

    using namespace Qt::Literals::StringLiterals;

    QMacAutoReleasePool autoReleasePool;

    NSError* creationError = nullptr;
    AVCaptureDeviceInput *deviceInput = [AVCaptureDeviceInput
        deviceInputWithDevice:avCaptureDevice
                        error:&creationError];
    if (creationError != nullptr)
        return q23::unexpected{ QString::fromNSString(creationError.localizedDescription) };
    Q_ASSERT(deviceInput);

    if (![m_avCaptureSession canAddInput:deviceInput])
        return q23::unexpected{
            u"Cannot attach AVCaptureDeviceInput to AVCaptureSession"_s };

    [m_avCaptureSession addInput:deviceInput];

    m_avCaptureDeviceVideoInput = AVFScopedPointer{ [deviceInput retain] };

    return {};
}

// If there is any current delegate, we block the background thread
// and set the delegate to discard future frames.
void QAVFCamera::clearAvCaptureVideoDataOutput()
{
    if (m_avCaptureVideoDataOutput) {
        [m_avCaptureSession removeOutput:m_avCaptureVideoDataOutput];
        m_avCaptureVideoDataOutput.reset();
    }
    if (m_qAvfSampleBufferDelegate) {
        // Push a blocking job to the background frame thread,
        // so we guarantee future frames are discarded. This
        // causes the frameHandler to be destroyed, and the reference
        // to this QAVFCamera is cleared.
        Q_ASSERT(m_delegateQueue);
        dispatch_sync(
            m_delegateQueue,
            [this]() {
                [m_qAvfSampleBufferDelegate discardFutureSamples];
            });

        m_qAvfSampleBufferDelegate.reset();
    }
}

q23::expected<void, QString> QAVFCamera::setupAvCaptureVideoDataOutput(
    AVCaptureDevice *avCaptureDevice)
{
    Q_ASSERT(avCaptureDevice);

    using namespace Qt::Literals::StringLiterals;

    QMacAutoReleasePool autoReleasePool;

    // Setup the delegate object for which we receive video frames.
    // This is called by the background thread. The frameHandler must
    // be cleared on the Delegate when destroying the QAVFCamera,
    // to avoid any remaining enqueued frame-jobs from reading this QAVFCamera
    // reference.
    auto frameHandler = [this](QVideoFrame frame) {
        dispatch_assert_queue(m_delegateQueue);
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
    m_qAvfSampleBufferDelegate = AVFScopedPointer{ [sampleBufferDelegate retain] };
    m_avCaptureVideoDataOutput = AVFScopedPointer{ [avCaptureVideoDataOutput retain] };

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
    Q_ASSERT(avCaptureDevice);
    Q_ASSERT(avCaptureDeviceFormat);
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

    Q_ASSERT(m_avCaptureVideoDataOutput);
    [m_qAvfSampleBufferDelegate setHWAccel:std::move(hwAccel)];
    [m_qAvfSampleBufferDelegate setVideoFormatFrameRate:newCameraFormat.maxFrameRate()];

    Q_ASSERT(m_avCaptureVideoDataOutput);
    NSDictionary *outputSettings = @{
        (NSString *)kCVPixelBufferPixelFormatTypeKey
            : [NSNumber numberWithUnsignedInt:outputCvPixelFormat],
        (NSString *)kCVPixelBufferMetalCompatibilityKey : @true
    };
    m_avCaptureVideoDataOutput.data().videoSettings = outputSettings;

    qt_set_active_format(avCaptureDevice, avCaptureDeviceFormat, false);

    m_framePixelFormat = QAVFHelpers::fromCVPixelFormat(outputCvPixelFormat);
    m_cvPixelFormat = outputCvPixelFormat;

    return {};
}

void QAVFCamera::clearRotationTracking()
{
    m_qAvfCameraRotationTracker = std::nullopt;
}

void QAVFCamera::setupRotationTracking(AVCaptureDevice *avCaptureDevice)
{
    Q_ASSERT(avCaptureDevice != nullptr);
    m_qAvfCameraRotationTracker = QFFmpeg::AvfCameraRotationTracker(avCaptureDevice);
}

void QAVFCamera::clearCaptureSessionConfiguration()
{
    clearAvCaptureSessionInputDevice();
    clearAvCaptureVideoDataOutput();
    clearRotationTracking();
}

[[nodiscard]] q23::expected<void, QString> QAVFCamera::tryConfigureCaptureSession(
    const QCameraDevice &cameraDevice,
    const QCameraFormat &cameraFormat)
{
    using namespace Qt::Literals::StringLiterals;

    AVCaptureDevice *avCaptureDevice = QAVFCameraBase::tryGetAvCaptureDevice(cameraDevice);
    if (avCaptureDevice == nullptr)
        return q23::unexpected{ u"AVCaptureDevice not available"_s };

    return tryConfigureCaptureSession(
        avCaptureDevice,
        cameraFormat);
}

[[nodiscard]] q23::expected<void, QString> QAVFCamera::tryConfigureCaptureSession(
    AVCaptureDevice *avCaptureDevice,
    const QCameraFormat &cameraFormat)
{
    using namespace Qt::Literals::StringLiterals;

    AVCaptureDeviceFormat *avCaptureDeviceFormat = findSuitableAvCaptureDeviceFormat(
        avCaptureDevice,
        cameraFormat);
    // If we can't find any suitable AVCaptureDeviceFormat,
    // then we cannot apply this QCameraFormat.
    if (avCaptureDeviceFormat == nullptr)
        return q23::unexpected{
            u"Unable to find any suitable AVCaptureDeviceFormat when attempting to "
            "apply QCameraFormat"_s };

    return tryConfigureCaptureSession(
        avCaptureDevice,
        avCaptureDeviceFormat,
        cameraFormat);
}

[[nodiscard]] q23::expected<void, QString> QAVFCamera::tryConfigureCaptureSession(
    AVCaptureDevice *avCaptureDevice,
    AVCaptureDeviceFormat *avCaptureDeviceFormat,
    const QCameraFormat &cameraFormat)
{
    Q_ASSERT(avCaptureDevice);
    Q_ASSERT(avCaptureDeviceFormat);

    q23::expected<void, QString> setupInputResult = setupAvCaptureSessionInputDevice(
        avCaptureDevice);
    if (!setupInputResult)
        return q23::unexpected{ std::move(setupInputResult.error()) };

    q23::expected<void, QString> setupOutputResult = setupAvCaptureVideoDataOutput(
        avCaptureDevice);
    if (!setupOutputResult)
        return q23::unexpected{ std::move(setupOutputResult.error()) };

    q23::expected<void, QString> applyFormatResult = tryApplyFormatToCaptureSession(
        avCaptureDevice,
        avCaptureDeviceFormat,
        cameraFormat);
    if (!applyFormatResult)
        return q23::unexpected{ std::move(applyFormatResult.error()) };

    setupRotationTracking(avCaptureDevice);

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

        AVCaptureDevice *avCaptureDevice = QAVFCameraBase::tryGetAvCaptureDevice(m_cameraDevice);
        if (avCaptureDevice == nullptr) {
            qWarning() << "QAVFCamera::onActiveChanged: Device not available";
            return;
        }

        // The AVCaptureDevice must be locked when we call AVCaptureSession.startRunning,
        // in order to not have the AVCaptureDeviceFormat be overriden by the AVCaptureSession's
        // quality preset. Additionally, we apply the format inside tryConfigureCaptureSession,
        // so it's beneficial to keep the device locked during the entire config stage.
        AVFConfigurationLock avCaptureDeviceLock { avCaptureDevice };
        if (!avCaptureDeviceLock) {
            qWarning() << "QAVFCamera::onActiveChanged: Failed to lock AVCaptureDevice";
            return;
        }

        q23::expected<void, QString> configureResult = tryConfigureCaptureSession(
            avCaptureDevice,
            cameraFormat());
        if (configureResult) {
            [m_avCaptureSession startRunning];
        } else {
            qWarning()
                << "QAVFCamera::onActiveChanged: Error when trying to activate camera:"
                << configureResult.error();
            clearCaptureSessionConfiguration();
        }

    } else {
        [m_avCaptureSession stopRunning];

        clearCaptureSessionConfiguration();
    }
}

void QAVFCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    m_qMediaCaptureSession = session ? session->captureSession() : nullptr;
}

void QAVFCamera::onCameraDeviceChanged(
    const QCameraDevice &newCameraDevice,
    const QCameraFormat &newFormat)
{
    // The incoming format should never be null if the incoming device is not null.
    Q_ASSERT(newCameraDevice.isNull() || !newFormat.isNull());

    // We cannot call AVCaptureSession.stopRunning() inside a
    // AVCaptureSession configuration scope, so we wrap that scope in
    // a lambda and call stopRunning() afterwards if configuration
    // fails for the new QCameraDevice.

    auto tryChangeDeviceFn = [&]() -> q23::expected<void, QString> {
        // Using this configuration transaction, we can clear up
        // resources and establish new ones without having to do slow
        // and synchronous calls to AVCaptureSession.stopRunning and startRunning.
        [m_avCaptureSession beginConfiguration];
        QScopeGuard endConfigGuard{ [&] {
            [m_avCaptureSession commitConfiguration];
        } };

        clearCaptureSessionConfiguration();

        // If the new QCameraDevice does not point to any physical device,
        // make sure we clear resources and shut down the capture-session.
        if (newCameraDevice.isNull() || !checkCameraPermission())
            return {};

        // If we are not currently active, then we can just accept the new property
        // value and return.
        if (![m_avCaptureSession isRunning])
            return {};

        q23::expected<void, QString> configureResult = tryConfigureCaptureSession(
            newCameraDevice,
            newFormat);
        if (!configureResult) {
            clearCaptureSessionConfiguration();
            return configureResult;
        }

        return {};
    };

    q23::expected<void, QString> changeDeviceResult = tryChangeDeviceFn();
    if (!changeDeviceResult) {
        [m_avCaptureSession stopRunning];
        qWarning()
            << "Error when trying to activate new camera-device: "
            << changeDeviceResult.error();
    }
}

bool QAVFCamera::tryApplyCameraFormat(const QCameraFormat &newCameraFormat)
{
    m_framePixelFormat = QVideoFrameFormat::Format_Invalid;
    m_cvPixelFormat = CvPixelFormatInvalid;

    // TODO: It's currently unclear whether we should accept the QCameraFormat
    // if the QCameraDevice is currently not connected.
    AVCaptureDevice *avCaptureDevice = QAVFCameraBase::tryGetAvCaptureDevice(m_cameraDevice);
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
        qWarning() << "QAVFCamera::tryApplyCameraFormat: Failed to lock AVCaptureDevice when "
                      "trying to apply new QCameraFormat.";
        return false;
    }

    [m_avCaptureSession beginConfiguration];
    QScopeGuard endConfigGuard { [this]() {
        [m_avCaptureSession commitConfiguration];
    } };

    clearCaptureSessionConfiguration();

    q23::expected<void, QString> configureResult = tryConfigureCaptureSession(
        avCaptureDevice,
        avCaptureDeviceFormat,
        newCameraFormat);
    if (!configureResult) {
        qWarning()
            << "Error when trying to activate camera with new format: "
            << configureResult.error();

        [m_avCaptureSession stopRunning];
        clearCaptureSessionConfiguration();

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

// Gets the current rotationfor this QAVFCamera.
// Returns the result in degrees, 0 to 360.
// Will always return a result that is divisible by 90.
int QAVFCamera::getCurrentRotationAngleDegrees() const
{
    if (m_qAvfCameraRotationTracker)
        return m_qAvfCameraRotationTracker->rotationDegrees();
    else
        return 0;
}

// The still photo finishing will be invoked on a background thread not
// controlled by us, on the QAvfCapturePhotoOutputDelegate object.
// Without proper synchronization, we can therefore end up in a
// situation where the callback is invoked after the QAVFCamera object
// is destroyed. The current approach is to have a thread-safe call that
// tells the QAvfCameraPhotoOutputDelegate to discard the results.
q23::expected<void, QString> QAVFCamera::requestStillPhotoCapture()
{
    Q_ASSERT(thread()->isCurrentThread());
    Q_ASSERT(isActive());
    Q_ASSERT(!stillPhotoCaptureInProgress());
    Q_ASSERT(m_avCapturePhotoOutput);
    // We must have an AVCaptureDeviceVideoInput hooked up to our AVCaptureSession
    // in order for the AVCapturePhotoOutput to be populated with correct values.
    Q_ASSERT(m_avCaptureDeviceVideoInput);

    using namespace Qt::Literals::StringLiterals;

    // TODO: We can potentially match the current QCameraFormat here,
    // which might help us save some bandwidth with i.e YUV420
    int captureFormat = kCVPixelFormatType_32BGRA;
    if (!checkAvCapturePhotoFormatSupport(m_avCapturePhotoOutput, captureFormat)) {
        qCWarning(qLcCamera) << "Attempted to take a still photo with an AVCapturePhotoOutput that "
                                "does not support output with 32BGRA format.";
        return q23::unexpected{ u"Internal camera configuration error"_s };
    }

    NSDictionary *formatDict =
        [NSDictionary dictionaryWithObject:[NSNumber numberWithUnsignedInt:captureFormat]
                                    forKey:(id)kCVPixelBufferPixelFormatTypeKey];

    // Set the settings for this capture.
    //
    // TODO: In the future we should try to respect the size set by QImageCapture here.
    // For now, we use the same size as whatever the AVCaptureDevice is currently using.
    AVCapturePhotoSettings *settings = [AVCapturePhotoSettings photoSettingsWithFormat:formatDict];
    settings.flashMode = toAvfFlashMode(flashMode());

    AVCaptureDevice *avCaptureDevice = [m_avCaptureDeviceVideoInput device];
    Q_ASSERT(avCaptureDevice);

    auto capturePhotoDelegate = AVFScopedPointer([[QAVFCapturePhotoOutputDelegate alloc]
        init:avCaptureDevice]);

    // If we mistakenly use settings that are not supported, captureWithSettings will
    // throw an exception.
    @try {
        [m_avCapturePhotoOutput capturePhotoWithSettings:settings
                                                delegate:capturePhotoDelegate];
    }
    @catch (NSException *exception) {
        QString errMsg =
            u"Attempted to start still photo capture with "
            "capture-settings that are not supported by AVCapturePhotoOutput: '%1'"_s
            .arg(QString::fromNSString(exception.description));
        qCWarning(qLcCamera) << errMsg;

        return q23::unexpected{ u"Internal camera configuration error"_s };
    }
    @finally {}

    QObject::connect(
        &capturePhotoDelegate.data().notifier,
        &QAVFStillPhotoNotifier::succeeded,
        this,
        &QAVFCamera::onStillPhotoDelegateSucceeded);
    QObject::connect(
        &capturePhotoDelegate.data().notifier,
        &QAVFStillPhotoNotifier::failed,
        this,
        &QAVFCamera::onStillPhotoDelegateFailed);

    m_qAvfCapturePhotoOutputDelegate = std::move(capturePhotoDelegate);

    return {};
}

void QAVFCamera::onStillPhotoDelegateSucceeded(const QVideoFrame &image)
{
    Q_ASSERT(stillPhotoCaptureInProgress());
    m_qAvfCapturePhotoOutputDelegate.reset();
    emit stillPhotoSucceeded(image);
}

void QAVFCamera::onStillPhotoDelegateFailed(QImageCapture::Error errType, const QString &errMsg)
{
    Q_ASSERT(stillPhotoCaptureInProgress());
    m_qAvfCapturePhotoOutputDelegate.reset();
    emit stillPhotoFailed(errType, errMsg);
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qavfcamera_p.cpp"
