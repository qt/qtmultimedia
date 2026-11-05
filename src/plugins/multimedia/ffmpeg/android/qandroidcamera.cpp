// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qandroidcamera_p.h>

#include <jni.h>

#include <atomic>
#include <future>
#include <limits>
#include <memory>
#include <optional>

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qpermissions.h>
#include <QtCore/private/qandroidextras_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>

#include <QtFFmpegMediaPluginImpl/private/qandroidvideoframefactory_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegmediacapturesession_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>

#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>

extern "C" {
#include "libavutil/hwcontext.h"
}

QT_BEGIN_NAMESPACE
Q_STATIC_LOGGING_CATEGORY(qLCAndroidCamera, "qt.multimedia.ffmpeg.androidCamera");

Q_DECLARE_JNI_CLASS(QtCameraActivation,
                    "org/qtproject/qt/android/multimedia/qffmpeg/QtCameraActivation")

using namespace Qt::Literals::StringLiterals;

namespace {

class QAndroidCameraActivation
{
public:
    using Result = q23::expected<void, QString>;

    [[nodiscard]] std::future<Result> getFuture()
    {
        return m_promise.get_future();
    }

    void setResult(Result result)
    {
        [[maybe_unused]] bool wasSet = m_resultSet.exchange(true);
        Q_ASSERT_X(!wasSet, "QAndroidCameraActivation::setResult",
                   "The result of a camera activation must only be set once");
        m_promise.set_value(std::move(result));
    }

private:
    std::promise<Result> m_promise;
    std::atomic<bool> m_resultSet = false;
};

// The Java object owns a heap-allocated reference, released by onActivationFinished().
[[nodiscard]] QtJniTypes::QtCameraActivation
createJavaActivation(const std::shared_ptr<QAndroidCameraActivation> &activation)
{
    auto *activationRef = new std::shared_ptr<QAndroidCameraActivation>(activation);
    QtJniTypes::QtCameraActivation javaActivation(
        static_cast<jlong>(reinterpret_cast<uintptr_t>(activationRef)));
    if (!javaActivation.isValid())
        delete activationRef;
    return javaActivation;
}

// TODO: Unify format selection with cross-platform layer
QCameraFormat getDefaultCameraFormat(const QCameraDevice & cameraDevice)
{
    // default settings
    const auto defaultFrameFormat = QVideoFrameFormat::Format_YUV420P;
    const auto defaultResolution = QSize(1920, 1080);
    QCameraFormatPrivate *defaultFormat = new QCameraFormatPrivate{
        .pixelFormat = defaultFrameFormat,
        .resolution = defaultResolution,
        .minFrameRate = 12,
        .maxFrameRate = 30,
    };

    QCameraFormat resultFormat = defaultFormat->create();
    const auto &supportedFormats = cameraDevice.videoFormats();

    if (supportedFormats.empty() || supportedFormats.contains(resultFormat))
        return resultFormat;

    auto pixelCount = [](const QSize& resolution) {
        Q_ASSERT(resolution.isValid());
        return resolution.width() * resolution.height();
    };

    const int defaultPixelCount = pixelCount(defaultResolution);

    // The lower the score, the better the format suits
    int differenceScore = std::numeric_limits<int>::max();

    auto calcDifferenceScore = [defaultPixelCount, pixelCount](const QCameraFormat& format) {
        const int pixelDifference = pixelCount(format.resolution()) - defaultPixelCount;
        // prefer:
        // 1. 'pixels count >= default' over 'pixels count < default'
        // 2. lower abs(pixelDifference)
        return pixelDifference < 0
                  ? -pixelDifference
                  : std::numeric_limits<int>::min() + pixelDifference;
    };

    for (const auto &supportedFormat : supportedFormats) {
        if (supportedFormat.pixelFormat() == defaultFrameFormat) {
            if (supportedFormat.resolution() == defaultResolution)
                return supportedFormat;

            const int currentDifferenceScore = calcDifferenceScore(supportedFormat);

            if (currentDifferenceScore < differenceScore) {
                differenceScore = currentDifferenceScore;
                resultFormat = supportedFormat;
            }
        }
    }

    return resultFormat;
}

bool checkCameraPermission()
{
    QCameraPermission permission;

    const bool granted = qApp->checkPermission(permission) == Qt::PermissionStatus::Granted;
    if (!granted)
        qCWarning(qLCAndroidCamera) << "Access to camera not granted!";

    return granted;
}

int sensorOrientation(QString cameraId)
{
    QJniObject deviceManager(QtJniTypes::Traits<QtJniTypes::QtVideoDeviceManager>::className(),
                             QNativeInterface::QAndroidApplication::context());

    if (!deviceManager.isValid()) {
        qCWarning(qLCAndroidCamera) << "Failed to connect to Qt Video Device Manager.";
        return 0;
    }

    return deviceManager.callMethod<jint>("getSensorOrientation",
                                          QJniObject::fromString(cameraId).object<jstring>());
}

// Returns the FocusModes that are available on the physical device, for which we also have
// an implementation.
[[nodiscard]] static QList<QCamera::FocusMode> qGetSupportedFocusModesFromAndroidCamera(
    const QJniObject &deviceManager,
    const QCameraDevice &cameraDevice)
{
    QList<QCamera::FocusMode> returnValue;

    const QStringList focusModeStrings = deviceManager.callMethod<QStringList>(
            "getSupportedQCameraFocusModesAsStrings",
            QJniObject::fromString(QString::fromUtf8(cameraDevice.id())).object<jstring>());

    // Translate the strings into enums if possible.
    for (const QString &focusModeString : focusModeStrings) {
        bool ok = false;
        const auto focusMode = static_cast<QCamera::FocusMode>(
            QMetaEnum::fromType<QCamera::FocusMode>()
                .keyToValue(
                    focusModeString.toLatin1().data(),
                    &ok));
        if (ok)
            returnValue.push_back(focusMode);
        else
            qCDebug(qLCAndroidCamera) <<
                "received a QCamera::FocusMode string from Android "
                "QtVideoDeviceManager.java that was not recognized.";
    }

    return returnValue;
}

} // namespace

namespace QFFmpeg {

// QAndroidCamera

QAndroidCamera::QAndroidCamera(QCamera *camera)
    : QPlatformCamera(camera)
    , m_jniCamera(
        static_cast<jlong>(reinterpret_cast<uintptr_t>(this)),
        QNativeInterface::QAndroidApplication::context())
{

    m_hwAccel = QFFmpeg::HWAccel::create(AVHWDeviceType::AV_HWDEVICE_TYPE_MEDIACODEC);
    if (camera) {
        m_cameraDevice = camera->cameraDevice();
        m_cameraFormat = !camera->cameraFormat().isNull() ? camera->cameraFormat()
                                                          : getDefaultCameraFormat(m_cameraDevice);
        updateCameraCharacteristics();
    }

    if (qApp) {
        connect(qApp, &QGuiApplication::applicationStateChanged,
                this, &QAndroidCamera::onApplicationStateChanged);
    }
};

QAndroidCamera::~QAndroidCamera()
{
    // Flush queued events / frames and clear the reference back to this
    // this instance. Blocking call.
    m_jniCamera.callMethod<void>("destroy");
    setState(State::Closed);
}

void QAndroidCamera::setCamera(const QCameraDevice &camera)
{
    const bool oldActive = isActive();
    if (oldActive)
        setActive(false);

    // Reset all our control-members on the Java-side to default
    // values. Then populate them again during updateCameraCharacteristics()
    m_jniCamera.callMethod<void>("resetControlProperties");

    m_cameraDevice = camera;
    updateCameraCharacteristics();
    m_cameraFormat = getDefaultCameraFormat(camera);

    if (oldActive)
        setActive(true);
}

QVideoFrameFormat QAndroidCamera::frameFormat() const
{
    // Camera2 delivers YUV_420_888, a flexible format whose memory layout (YUV420P, NV12 or NV21)
    // is chosen by the camera HAL. The layout can't be queried ahead of time, so we can't know
    // the frame format until we receive the actual frame. We leave the format invalid, so that
    // the recorder won't set anything up until it receives the first frame.
    return {};
}

// Called by the Java-side processing background thread.
void QAndroidCamera::frameAvailable(QJniObject image, bool takePhoto)
{
    // Frames may arrive while activating, before activeChanged(true). The capture session
    // is connected ahead of time by onSourceActivating(), so they are not lost.
    if (m_state == State::Closed || m_frameFactory == nullptr) {
        image.callMethod<void>("close");
        return;
    }

    QVideoFrame videoFrame = m_frameFactory->createVideoFrame(image, rotation());
    if (!videoFrame.isValid())
        return;

    videoFrame.setMirrored(m_cameraDevice.position() == QCameraDevice::Position::FrontFace);

    if (!takePhoto)
        Q_EMIT newVideoFrame(videoFrame);
    else
        Q_EMIT onStillPhotoCaptured(videoFrame);
}

QtVideo::Rotation QAndroidCamera::rotation() const
{
    // If the camera is not attached to the main display, we don't want to apply rotation
    // based on primary screen. We assume we are an external camera.
    const QCameraDevice::Position cameraPosition = m_cameraDevice.position();
    const bool isExternalCamera = cameraPosition == QCameraDevice::Position::UnspecifiedPosition;
    if (isExternalCamera)
        return QtVideo::Rotation::None;

    auto screen = QGuiApplication::primaryScreen();
    auto screenOrientation = screen->orientation();
    if (screenOrientation == Qt::PrimaryOrientation)
        screenOrientation = screen->primaryOrientation();

    // Display rotation is the opposite direction of the physical device rotation. We need the
    // device rotation, that's why Landscape is 270 and InvertedLandscape is 90
    int deviceOrientation = 0;
    switch (screenOrientation) {
    case Qt::PrimaryOrientation:
    case Qt::PortraitOrientation:
        break;
    case Qt::LandscapeOrientation:
        deviceOrientation = 270;
        break;
    case Qt::InvertedPortraitOrientation:
        deviceOrientation = 180;
        break;
    case Qt::InvertedLandscapeOrientation:
        deviceOrientation = 90;
        break;
    }

    const int sign = (cameraPosition == QCameraDevice::Position::FrontFace) ? 1 : -1;
    int rotation = (sensorOrientation(QString::fromUtf8(m_cameraDevice.id()))
            - deviceOrientation * sign + 360) % 360;

    return QtVideo::Rotation(rotation);
}

void QAndroidCamera::setActive(bool active)
{
    Q_ASSERT(thread()->isCurrentThread());
    if (isActive() == active)
        return;

    if (!m_jniCamera.isValid()) {
        updateError(QCamera::CameraError, QStringLiteral("No connection to Android Camera2 API"));
        return;
    }

    if (!active || !checkCameraPermission()) {
        m_jniCamera.callMethod<void>("stopAndClose");
        m_jniCamera.callMethod<void>("clearSurfaces");
        setState(State::Closed);
        return;
    }

    // Connect the capture session ahead of time, so frames delivered while activating
    // are not dropped.
    if (m_session)
        m_session->onSourceActivating(*this);

    q23::expected<void, QString> result = tryActivate();
    if (!result) {
        m_jniCamera.callMethod<void>("stopAndClose");
        setState(State::Closed);
        if (m_session)
            m_session->onSourceActivationFailure(*this);
        updateError(QCamera::CameraError, result.error());
        return;
    }

    setState(State::Started);
}

void QAndroidCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    m_session = session ? qobject_cast<QFFmpegMediaCaptureSession *>(session) : nullptr;
    Q_ASSERT(!session || m_session);
}

q23::expected<void, QString> QAndroidCamera::tryActivate()
{
    QString cameraId = QString::fromUtf8(m_cameraDevice.id());
    int width = m_cameraFormat.resolution().width();
    int height = m_cameraFormat.resolution().height();

    if (width < 0 || height < 0) {
        m_cameraFormat = getDefaultCameraFormat(m_cameraDevice);
        width = m_cameraFormat.resolution().width();
        height = m_cameraFormat.resolution().height();
    }

    m_videoResolution = { FFALIGN(width, 16), FFALIGN(height, 16) };
    m_stillReaderSize = stillResolution();

    setState(State::Activating);

    // Create frameFactory when ImageReader is created;
    m_frameFactory = QAndroidVideoFrameFactory::create();

    // Most supported format combination for ImageReaders in a session is YUV_420_888 for
    // preview/video, and JPEG for stills
    // TODO: Set session combinations based on device capabilities:
    // https://developer.android.com/reference/android/hardware/camera2/CameraDevice#regular-capture
    m_jniCamera.callMethod<void>("prepareCamera", static_cast<jint>(m_videoResolution.width()),
                                 static_cast<jint>(m_videoResolution.height()),
                                 static_cast<jint>(m_cameraFormat.minFrameRate()),
                                 static_cast<jint>(m_cameraFormat.maxFrameRate()),
                                 static_cast<jint>(m_stillReaderSize.width()),
                                 static_cast<jint>(m_stillReaderSize.height()));

    auto activation = std::make_shared<QAndroidCameraActivation>();
    std::future<QAndroidCameraActivation::Result> result = activation->getFuture();

    QtJniTypes::QtCameraActivation javaActivation = createJavaActivation(activation);
    if (!javaActivation.isValid()) {
        qCWarning(qLCAndroidCamera) << "Failed to create QtCameraActivation";
        return q23::unexpected{ u"Failed to start camera"_s };
    }

    // Callbacks from the Java-side background thread must never wait for this thread.
    m_jniCamera.callMethod<void>("open", cameraId, javaActivation);

    // Assumes the Java side always reports a result.
    return result.get();
}

// TODO: setState is currently being used by the C++ thread owning
// the QCamera object, and the Java-side capture-processing background thread.
// This can lead to race conditions and the m_state ending up in inconsistent states.
// We should have a synchronization strategy in the future.
void QAndroidCamera::setState(QAndroidCamera::State newState)
{
    if (newState == m_state)
        return;

    bool wasActive = isActive();
    m_state = newState;

    if (wasActive != isActive())
        Q_EMIT activeChanged(isActive());
}

bool QAndroidCamera::setCameraFormat(const QCameraFormat &format)
{
    const auto chosenFormat = format.isNull() ? getDefaultCameraFormat(m_cameraDevice) : format;

    if (chosenFormat == m_cameraFormat)
        return true;
    if (!m_cameraDevice.videoFormats().contains(chosenFormat))
        return false;

    m_cameraFormat = chosenFormat;

    restartCamera();

    return true;
}

void QAndroidCamera::updateCameraCharacteristics()
{
    if (m_cameraDevice.id().isEmpty()) {
        cleanCameraCharacteristics();
        return;
    }

    QJniObject deviceManager(QtJniTypes::Traits<QtJniTypes::QtVideoDeviceManager>::className(),
                             QNativeInterface::QAndroidApplication::context());

    if (!deviceManager.isValid()) {
        qCWarning(qLCAndroidCamera) << "Failed to connect to Qt Video Device Manager.";
        cleanCameraCharacteristics();
        return;
    }


    // Gather capabilities.
    QCamera::Features newSupportedFeatures = {};

    float newMaxZoom = 1.f;
    float newMinZoom = 1.f;
    const auto newZoomRange = deviceManager.callMethod<jfloat[]>(
            "getZoomRange",
            QJniObject::fromString(QString::fromUtf8(m_cameraDevice.id())).object<jstring>());
    if (newZoomRange.isValid() && newZoomRange.size() == 2) {
        newMinZoom = newZoomRange[0];
        newMaxZoom = newZoomRange[1];
    } else {
        qCDebug(qLCAndroidCamera) <<
            "received invalid float array when querying zoomRange from Android Camera2. "
            "Likely Qt developer bug";
    }

    m_supportedFlashModes.clear();
    m_supportedFlashModes.append(QCamera::FlashOff);
    const QStringList flashModes = deviceManager.callMethod<QStringList>(
            "getSupportedFlashModes",
            QJniObject::fromString(QString::fromUtf8(m_cameraDevice.id())).object<jstring>());
    for (const auto &flashMode : flashModes) {
        if (flashMode == QLatin1String("auto"))
            m_supportedFlashModes.append(QCamera::FlashAuto);
        else if (flashMode == QLatin1String("on"))
            m_supportedFlashModes.append(QCamera::FlashOn);
    }

    m_TorchModeSupported = deviceManager.callMethod<jboolean>(
            "isTorchModeSupported",
            QJniObject::fromString(QString::fromUtf8(m_cameraDevice.id())).object<jstring>());

    m_supportedFocusModes = qGetSupportedFocusModesFromAndroidCamera(
        deviceManager,
        m_cameraDevice);

    if (m_supportedFocusModes.contains(QCamera::FocusMode::FocusModeManual))
        newSupportedFeatures |= QCamera::Feature::FocusDistance;


    // Signal capability changes
    minimumZoomFactorChanged(newMinZoom);
    maximumZoomFactorChanged(newMaxZoom);
    supportedFeaturesChanged(newSupportedFeatures);


    // Apply properties
    if (minZoomFactor() < maxZoomFactor()) {
        // New device supports zooming. Clamp it and apply it to new camera device.
        const float newZoomFactor = qBound(minZoomFactor(), zoomFactor(), maxZoomFactor());
        zoomTo(newZoomFactor, -1);
    }

    if (isFlashModeSupported(flashMode()))
        setFlashMode(flashMode());

    if (isTorchModeSupported(torchMode()))
        setTorchMode(torchMode());

    if (isFocusModeSupported(focusMode()))
        setFocusMode(focusMode());


    // Reset properties if needed.
    if (minZoomFactor() >= maxZoomFactor())
        zoomFactorChanged(defaultZoomFactor());

    if (!isFlashModeSupported(flashMode()))
        flashModeChanged(defaultFlashMode());

    if (!isTorchModeSupported(torchMode()))
        torchModeChanged(defaultTorchMode());

    if (!isFocusModeSupported(focusMode()))
        focusModeChanged(defaultFocusMode());
}

// Should only be called when the camera device is set to null.
void QAndroidCamera::cleanCameraCharacteristics()
{
    maximumZoomFactorChanged(1.0);
    if (zoomFactor() != 1.0) {
        zoomTo(1.0, -1.0);
    }
    if (torchMode() != QCamera::TorchOff) {
        setTorchMode(QCamera::TorchOff);
    }
    m_TorchModeSupported = false;

    if (flashMode() != QCamera::FlashOff) {
        setFlashMode(QCamera::FlashOff);
    }
    m_supportedFlashModes.clear();
    m_supportedFlashModes.append(QCamera::FlashOff);


    // Reset focus mode.
    if (focusMode() != QCamera::FocusModeAuto)
        setFocusMode(QCamera::FocusModeAuto);
    m_supportedFocusModes.clear();

    supportedFeaturesChanged({});
}

QSize QAndroidCamera::stillResolution() const
{
    if (m_requestedStillResolution.isEmpty())
        return m_videoResolution; // Not set, will use video resolution

    const QList<QSize> photoResolutions = m_cameraDevice.photoResolutions();

    // Use as-is with empty list, might fail
    if (photoResolutions.isEmpty() || photoResolutions.contains(m_requestedStillResolution))
        return m_requestedStillResolution;

    return qClosestSupportedResolution(m_requestedStillResolution, photoResolutions);
}

// Restart camera now, must be called from owning thread
void QAndroidCamera::restartCamera()
{
    if (!isActive())
        return;

    setActive(false);
    setActive(true);
}

// Queue a camera restart, can be called from a background Java thread
void QAndroidCamera::queueCameraRestart()
{
    if (m_restartPending.exchange(true))
        return;

    // NOTE: Queued on event loop to avoid re-entering onVideoSourceChanged
    QMetaObject::invokeMethod(
            this,
            [this]() {
                m_restartPending = false;
                restartCamera(); // No-op if inactive
            },
            Qt::QueuedConnection);
}

void QAndroidCamera::setFocusDistance(float distance)
{
    if (QtPrivate::fuzzyCompare(focusDistance(), distance))
        return;

    if (!(supportedFeatures() & QCamera::Feature::FocusDistance)) {
        qCWarning(qLCAndroidCamera) <<
            Q_FUNC_INFO <<
            "attmpted to set focus-distance on camera without support for FocusDistance feature";
        return;
    }

    if (distance < 0 || distance > 1) {
        qCWarning(qLCAndroidCamera) <<
            Q_FUNC_INFO <<
            "attempted to set camera focus-distance with out-of-bounds value";
        return;
    }

    // focusDistance should only be applied if the focusMode is currently set to Manual.
    // The Java function will handle this behavior. Either way, the value should be accepted so
    // we can apply it if FocusModeManual is activated
    m_jniCamera.callMethod<void>("setFocusDistance", distance);

    focusDistanceChanged(distance);
}

void QAndroidCamera::setFocusMode(QCamera::FocusMode mode)
{
    if (focusMode() == mode)
        return;

    if (!isFocusModeSupported(mode)) {
        qCWarning(qLCAndroidCamera) <<
            Q_FUNC_INFO <<
            QLatin1String("attempted to set focus-mode '%1' on camera where it is unsupported.")
                .arg(QMetaEnum::fromType<QCamera::FocusMode>().valueToKey(mode));
        return;
    }

    // If the new focus-mode is Manual, then the focusDistance
    // needs to be applied as well. The Java function handles this.
    m_jniCamera.callMethod<void>(
        "setFocusMode",
        static_cast<jint>(mode));

    focusModeChanged(mode);
}

void QAndroidCamera::setFlashMode(QCamera::FlashMode mode)
{
    if (!isFlashModeSupported(mode))
        return;

    QString flashMode;
    switch (mode) {
        case QCamera::FlashAuto:
            flashMode = QLatin1String("auto");
            break;
        case QCamera::FlashOn:
            flashMode = QLatin1String("on");
            break;
        case QCamera::FlashOff:
        default:
            flashMode = QLatin1String("off");
            break;
    }

    m_jniCamera.callMethod<void>("setFlashMode", QJniObject::fromString(flashMode).object<jstring>());
    flashModeChanged(mode);
}

bool QAndroidCamera::isFlashModeSupported(QCamera::FlashMode mode) const
{
    return m_supportedFlashModes.contains(mode);
}

bool QAndroidCamera::isFlashReady() const
{
    // Android doesn't have an API for that.
    // Only check if device supports more flash modes than just FlashOff.
    return m_supportedFlashModes.size() > 1;
}

bool QAndroidCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    return QPlatformCamera::isFocusModeSupported(mode) || m_supportedFocusModes.contains(mode);
}

bool QAndroidCamera::isTorchModeSupported(QCamera::TorchMode mode) const
{
    if (mode == QCamera::TorchOff)
        return true;
    else if (mode == QCamera::TorchOn)
        return m_TorchModeSupported;

    return false;
}

void QAndroidCamera::setTorchMode(QCamera::TorchMode mode)
{
    bool torchMode;
    if (mode == QCamera::TorchOff) {
        torchMode = false;
    } else if (mode == QCamera::TorchOn) {
        torchMode = true;
    } else {
        qWarning() << "Unknown Torch mode";
        return;
    }
    m_jniCamera.callMethod<void>("setTorchMode", jboolean(torchMode));
    torchModeChanged(mode);
}

void QAndroidCamera::zoomTo(float factor, float rate)
{
    Q_UNUSED(rate);

    if (!m_cameraDevice.id().isEmpty()) {
        m_jniCamera.callMethod<void>("zoomTo", factor);
    }
    zoomFactorChanged(factor);
}

void QAndroidCamera::onApplicationStateChanged()
{
    switch (QGuiApplication::applicationState()) {
        case Qt::ApplicationInactive:
            if (isActive()) {
                setActive(false);
                m_wasActive = true;
            }
            break;
        case Qt::ApplicationActive:
            if (m_wasActive) {
                setActive(true);
                m_wasActive = false;
            }
            break;
        default:
            break;
    }
}

// Called by Java-side processing background thread.
void QAndroidCamera::onCaptureSessionConfigured()
{
    bool canStart = m_jniCamera.callMethod<jboolean>("start");
    // While activating, Java reports the failure to the pending activation instead.
    if (!canStart && isActive())
        setState(State::Closed);
}

// Called by Java-side processing background thread.
void QAndroidCamera::onCaptureSessionConfigureFailed()
{
    setState(State::Closed);
}

// Called by Java-side processing background thread.
// The session rebuilt for a new still reader size failed. Fall back to a full restart.
void QAndroidCamera::onStillReaderSwapFailed()
{
    queueCameraRestart();
}

// Called by Java-side processing background thread.
void QAndroidCamera::onCameraOpened()
{
    // Failures are reported to the pending activation by Java.
    m_jniCamera.callMethod<jboolean>("createSession");
}

// Called by Java-side processing background thread.
void QAndroidCamera::onCameraDisconnect()
{
    setState(State::Closed);
}

// Called by Java-side processing background thread.
void QAndroidCamera::onCameraError(int reason)
{
    updateError(QCamera::CameraError,
                QString(u"Capture error with Camera %1. Camera2 Api error code: %2")
                        .arg(m_cameraDevice.description())
                        .arg(reason));
}

// Called by Java-side processing background thread.
void QAndroidCamera::onSessionClosed()
{
    setState(State::Closed);
}

void QAndroidCamera::capture()
{
    m_jniCamera.callMethod<void>("beginStillPhotoCapture");
}

void QAndroidCamera::updateExif(const QString &filename)
{
    m_jniCamera.callMethod<void>("saveExifToFile", QJniObject::fromString(filename).object<jstring>());
}

void QAndroidCamera::setStillCaptureResolution(QSize resolution)
{
    Q_ASSERT(thread()->isCurrentThread());

    if (resolution != m_requestedStillResolution && !resolution.isEmpty()
        && m_cameraDevice.photoResolutions().isEmpty()) {
        qCWarning(qLCAndroidCamera) << "Image capture resolution set to" << resolution
                                    << "while list of valid photo resolutions is empty. Image "
                                       "capture can fail if resolution is not supported by device.";
    }
    m_requestedStillResolution = resolution;
    applyStillResolution();
}

void QAndroidCamera::applyStillResolution()
{
    Q_ASSERT(thread()->isCurrentThread());

    const QSize newSize = stillResolution();
    // Not started: the next start uses the new size or re-runs this, see frameAvailable()
    if (!isActive() || m_stillReaderSize == newSize)
        return;

    if (m_jniCamera.callMethod<jboolean>("updateStillImageReader", newSize.width(),
                                         newSize.height())) {
        m_stillReaderSize = newSize;
        return;
    }

    queueCameraRestart();
}

void QAndroidCamera::onCaptureSessionFailed(int reason, long frameNumber)
{
    Q_UNUSED(frameNumber);

    updateError(QCamera::CameraError,
                QStringLiteral("Capture session failure with Camera %1. Camera2 Api error code: %2")
                        .arg(m_cameraDevice.description())
                        .arg(reason));
}

// Called by Java background thread if the on-going still-photo capture fails.
void QAndroidCamera::onStillPhotoCaptureFailed()
{
    // TODO: Emit a more descriptive error signal. At the time of writing, there is no
    // suitable QImageCapture::Error enumerator for this scenario.
    Q_EMIT onImageCaptureFailed(
        QImageCapture::Error::ResourceError,
        QStringLiteral("Unknown error"));
}

} // namespace QFFmpeg

// JNI logic
// The following static functions can only be called by the Java-side processing background
// thread. They all assume that we are currently holding the mutex protecting the
// QAndroidCamera reference on the Java-side.

namespace {

[[nodiscard]] QFFmpeg::QAndroidCamera& jLongToQAndroidCamera(jlong input)
{
    Q_ASSERT(input != 0);
    return *reinterpret_cast<QFFmpeg::QAndroidCamera *>(static_cast<uintptr_t>(input));
}

} // Anonymous namespace

static void onPreviewFrameAvailable(JNIEnv *, jobject, jlong qAndroidCameraPtr,
                                    QtJniTypes::Image image)
{
    QFFmpeg::QAndroidCamera &camera = jLongToQAndroidCamera(qAndroidCameraPtr);
    camera.frameAvailable(QJniObject(image));
}
Q_DECLARE_JNI_NATIVE_METHOD(onPreviewFrameAvailable)

static void onStillPhotoAvailable(JNIEnv *, jobject, jlong qAndroidCameraPtr,
                                  QtJniTypes::Image image)
{
    QFFmpeg::QAndroidCamera &camera = jLongToQAndroidCamera(qAndroidCameraPtr);
    camera.frameAvailable(QJniObject(image), true);
}
Q_DECLARE_JNI_NATIVE_METHOD(onStillPhotoAvailable)

static void onCameraOpened(JNIEnv *, jobject, jlong qAndroidCameraPtr)
{
    QFFmpeg::QAndroidCamera &camera = jLongToQAndroidCamera(qAndroidCameraPtr);
    camera.onCameraOpened();
}
Q_DECLARE_JNI_NATIVE_METHOD(onCameraOpened)

static void onCameraDisconnect(JNIEnv *, jobject, jlong qAndroidCameraPtr)
{
    QFFmpeg::QAndroidCamera &camera = jLongToQAndroidCamera(qAndroidCameraPtr);
    camera.onCameraDisconnect();
}
Q_DECLARE_JNI_NATIVE_METHOD(onCameraDisconnect)

static void onCameraError(JNIEnv *, jobject, jlong qAndroidCameraPtr, jint error)
{
    QFFmpeg::QAndroidCamera &camera = jLongToQAndroidCamera(qAndroidCameraPtr);
    camera.onCameraError(error);
}
Q_DECLARE_JNI_NATIVE_METHOD(onCameraError)

static void onCaptureSessionConfigured(JNIEnv *, jobject, jlong qAndroidCameraPtr)
{
    QFFmpeg::QAndroidCamera &camera = jLongToQAndroidCamera(qAndroidCameraPtr);
    camera.onCaptureSessionConfigured();
}
Q_DECLARE_JNI_NATIVE_METHOD(onCaptureSessionConfigured)

static void onCaptureSessionConfigureFailed(JNIEnv *, jobject, jlong qAndroidCameraPtr)
{
    QFFmpeg::QAndroidCamera &camera = jLongToQAndroidCamera(qAndroidCameraPtr);
    camera.onCaptureSessionConfigureFailed();
}
Q_DECLARE_JNI_NATIVE_METHOD(onCaptureSessionConfigureFailed)

static void onStillReaderSwapFailed(JNIEnv *, jobject, jlong qAndroidCameraPtr)
{
    QFFmpeg::QAndroidCamera &camera = jLongToQAndroidCamera(qAndroidCameraPtr);
    camera.onStillReaderSwapFailed();
}
Q_DECLARE_JNI_NATIVE_METHOD(onStillReaderSwapFailed)

static void onSessionClosed(JNIEnv *, jobject, jlong qAndroidCameraPtr)
{
    QFFmpeg::QAndroidCamera &camera = jLongToQAndroidCamera(qAndroidCameraPtr);
    camera.onSessionClosed();
}
Q_DECLARE_JNI_NATIVE_METHOD(onSessionClosed)

static void onCaptureSessionFailed(JNIEnv *, jobject, jlong qAndroidCameraPtr, jint reason,
                                   jlong framenumber)
{
    QFFmpeg::QAndroidCamera &camera = jLongToQAndroidCamera(qAndroidCameraPtr);
    camera.onCaptureSessionFailed(reason, framenumber);
}
Q_DECLARE_JNI_NATIVE_METHOD(onCaptureSessionFailed)

static void onStillPhotoCaptureFailed(JNIEnv *, jobject, jlong qAndroidCameraPtr)
{
    QFFmpeg::QAndroidCamera &camera = jLongToQAndroidCamera(qAndroidCameraPtr);
    camera.onStillPhotoCaptureFailed();
}
Q_DECLARE_JNI_NATIVE_METHOD(onStillPhotoCaptureFailed)

// Called once per activation, from any thread.
static void onActivationFinished(JNIEnv *, jobject, jlong activationPtr, jboolean success,
                                 QString errorMessage)
{
    Q_ASSERT(activationPtr != 0);
    std::unique_ptr<std::shared_ptr<QAndroidCameraActivation>> activationRef(
        reinterpret_cast<std::shared_ptr<QAndroidCameraActivation> *>(
            static_cast<uintptr_t>(activationPtr)));

    if (success)
        (*activationRef)->setResult({});
    else
        (*activationRef)->setResult(q23::unexpected{ std::move(errorMessage) });
}
Q_DECLARE_JNI_NATIVE_METHOD(onActivationFinished)

bool QFFmpeg::QAndroidCamera::registerNativeMethods()
{
    static const bool registered = []() {
        bool activationRegistered = QtJniTypes::QtCameraActivation::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(onActivationFinished),
        });
        if (!activationRegistered)
            return false;

        return QtJniCamera2::registerNativeMethods({
            Q_JNI_NATIVE_METHOD(onCameraOpened),
            Q_JNI_NATIVE_METHOD(onCameraDisconnect),
            Q_JNI_NATIVE_METHOD(onCameraError),
            Q_JNI_NATIVE_METHOD(onCaptureSessionConfigured),
            Q_JNI_NATIVE_METHOD(onCaptureSessionConfigureFailed),
            Q_JNI_NATIVE_METHOD(onStillReaderSwapFailed),
            Q_JNI_NATIVE_METHOD(onCaptureSessionFailed),
            Q_JNI_NATIVE_METHOD(onPreviewFrameAvailable),
            Q_JNI_NATIVE_METHOD(onStillPhotoAvailable),
            Q_JNI_NATIVE_METHOD(onSessionClosed),
            Q_JNI_NATIVE_METHOD(onStillPhotoCaptureFailed),
        });
    }();
    return registered;
}

QT_END_NAMESPACE
