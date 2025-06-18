// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidcamera_p.h"

#include <jni.h>

#include <memory>
#include <optional>
#include <utility>

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qpermissions.h>
#include <QtCore/qreadwritelock.h>
#include <QtCore/private/qandroidextras_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>

#include <QtMultimedia/qmediaformat.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#include <QtMultimedia/private/qvideoframeconverter_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <qandroidvideoframefactory_p.h>
#include <qffmpegvideobuffer_p.h>

extern "C" {
#include "libavutil/hwcontext.h"
}

QT_BEGIN_NAMESPACE
Q_STATIC_LOGGING_CATEGORY(qLCAndroidCamera, "qt.multimedia.ffmpeg.androidCamera");

// TODO: Should be reworked to just pass a pointer of the QAndroidCamera object into the QtCamera2
// Java instance.
typedef QMap<QString, QFFmpeg::QAndroidCamera *> QAndroidCameraMap;
Q_GLOBAL_STATIC(QAndroidCameraMap, g_qcameras)
Q_GLOBAL_STATIC(QReadWriteLock, rwLock)

namespace {

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

QAndroidCamera::QAndroidCamera(QCamera *camera) : QPlatformCamera(camera)
{
    m_jniCamera = QJniObject(QtJniTypes::Traits<QtJniTypes::QtCamera2>::className(),
                             QNativeInterface::QAndroidApplication::context());

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
    {
        QWriteLocker locker(rwLock);
        g_qcameras->remove(QString::fromUtf8(m_cameraDevice.id()));

        m_jniCamera.callMethod<void>("stopAndClose");
        setState(State::Closed);
    }

    m_jniCamera.callMethod<void>("stopBackgroundThread");
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

std::optional<int> QAndroidCamera::ffmpegHWPixelFormat() const
{
    // TODO: m_androidFramePixelFormat is continuously being written to by
    // the Java-side capture-processing background thread when receiving frame, while this
    // function is commonly called by the media recording engine on other threads.
    // A potential solution might include a mutex-lock and/or determining
    // the pixelFormat ahead of time by checking what format we request
    // when starting the Android camera capture session.
    return QFFmpegVideoBuffer::toAVPixelFormat(m_androidFramePixelFormat);
}

QVideoFrameFormat QAndroidCamera::frameFormat() const
{
    QVideoFrameFormat result = QPlatformCamera::frameFormat();
    // Apply rotation for surface only
    result.setRotation(rotation());
    return result;
}

// Called by the Java-side processing background thread.
void QAndroidCamera::frameAvailable(QJniObject image, bool takePhoto)
{
    if ((!(m_state == State::WaitingStart || m_state == State::Started) && !m_waitingForFirstFrame)
            || m_frameFactory == nullptr) {
        qCWarning(qLCAndroidCamera) << "Received frame when not active... ignoring";
        qCWarning(qLCAndroidCamera) << "state:" << m_state;
        image.callMethod<void>("close");
        return;
    }

    QVideoFrame videoFrame = m_frameFactory->createVideoFrame(image, rotation());
    if (!videoFrame.isValid())
        return;

    // TODO: m_androidFramePixelFormat is written by the Java-side processing
    // background thread, but read by the QCamera thread during QAndroid::ffmpegHWPixelFormat().
    // This causes a race condition (not severe). We should eventually implement some
    // synchronization strategy.
    m_androidFramePixelFormat = videoFrame.pixelFormat();
    if (m_waitingForFirstFrame) {
        m_waitingForFirstFrame = false;
        setState(State::Started);
    }

    videoFrame.setMirrored(m_cameraDevice.position() == QCameraDevice::Position::FrontFace);

    if (!takePhoto)
        emit newVideoFrame(videoFrame);
    else
        emit onStillPhotoCaptured(videoFrame);
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
    if (isActive() == active)
        return;

    if (!m_jniCamera.isValid()) {
        updateError(QCamera::CameraError, QStringLiteral("No connection to Android Camera2 API"));
        return;
    }

    if (active && checkCameraPermission()) {
        QWriteLocker locker(rwLock);
        int width = m_cameraFormat.resolution().width();
        int height = m_cameraFormat.resolution().height();

        if (width < 0 || height < 0) {
            m_cameraFormat = getDefaultCameraFormat(m_cameraDevice);
            width = m_cameraFormat.resolution().width();
            height = m_cameraFormat.resolution().height();
        }

        width = FFALIGN(width, 16);
        height = FFALIGN(height, 16);

        setState(State::WaitingOpen);
        g_qcameras->insert(QString::fromUtf8(m_cameraDevice.id()), this);

        // Create frameFactory when ImageReader is created;
        m_frameFactory = QAndroidVideoFrameFactory::create();

        // this should use the camera format.
        // but there is only 2 fully supported formats on android - JPG and YUV420P
        // and JPEG is not supported for encoding in FFmpeg, so it's locked for YUV for now.
        const static int imageFormat =
                QJniObject::getStaticField<QtJniTypes::ImageFormat, jint>("YUV_420_888");
        m_jniCamera.callMethod<void>("prepareCamera", jint(width), jint(height),
                                     jint(imageFormat), jint(m_cameraFormat.minFrameRate()),
                                     jint(m_cameraFormat.maxFrameRate()));

        bool canOpen = m_jniCamera.callMethod<jboolean>(
                "open",
                QJniObject::fromString(QString::fromUtf8(m_cameraDevice.id())).object<jstring>());

        if (!canOpen) {
            g_qcameras->remove(QString::fromUtf8(m_cameraDevice.id()));
            setState(State::Closed);
            updateError(QCamera::CameraError,
                        QString(u"Failed to start camera: ").append(m_cameraDevice.description()));
        }
    } else {
        m_jniCamera.callMethod<void>("stopAndClose");
        m_jniCamera.callMethod<void>("clearSurfaces");
        setState(State::Closed);
    }
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

    if (newState == State::Started)
        m_state = State::Started;

    if (m_state == State::Started && newState == State::Closed)
        m_state = State::Closed;

    if ((m_state == State::WaitingOpen || m_state == State::WaitingStart)
        && newState == State::Closed) {

        m_state = State::Closed;

        updateError(QCamera::CameraError,
                    QString(u"Failed to start Camera %1").arg(m_cameraDevice.description()));
    }

    if (m_state == State::Closed && newState == State::WaitingOpen)
        m_state = State::WaitingOpen;

    if (m_state == State::WaitingOpen && newState == State::WaitingStart)
        m_state = State::WaitingStart;

    if (wasActive != isActive())
        emit activeChanged(isActive());
}

bool QAndroidCamera::setCameraFormat(const QCameraFormat &format)
{
    const auto chosenFormat = format.isNull() ? getDefaultCameraFormat(m_cameraDevice) : format;

    if (chosenFormat == m_cameraFormat)
        return true;
    if (!m_cameraDevice.videoFormats().contains(chosenFormat))
        return false;

    m_cameraFormat = chosenFormat;

    if (isActive()) {
        // Restart the camera to set new camera format
        setActive(false);
        setActive(true);
    }

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
        const float newZoomFactor = qBound(zoomFactor(), minZoomFactor(), maxZoomFactor());
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

void QAndroidCamera::setFocusDistance(float distance)
{
    if (qFuzzyCompare(focusDistance(), distance))
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
    setState(canStart ? State::WaitingStart : State::Closed);
}

// Called by Java-side processing background thread.
void QAndroidCamera::onCaptureSessionConfigureFailed()
{
    setState(State::Closed);
}

// Called by Java-side processing background thread.
void QAndroidCamera::onCameraOpened()
{
    bool canStart = m_jniCamera.callMethod<jboolean>("createSession");
    setState(canStart ? State::WaitingStart : State::Closed);
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
void QAndroidCamera::onSessionActive()
{
    m_waitingForFirstFrame = true;
}

// Called by Java-side processing background thread.
void QAndroidCamera::onSessionClosed()
{
    m_waitingForFirstFrame = false;
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
    emit onImageCaptureFailed(
        QImageCapture::Error::ResourceError,
        QStringLiteral("Unknown error"));
}

} // namespace QFFmpeg

// JNI logic
// The following static functions can only be called by the Java-side processing background
// thread.

#define GET_CAMERA(cameraId)                                                          \
  QString key = QJniObject(cameraId).toString();                                      \
  QReadLocker locker(rwLock);                                                         \
  if (!g_qcameras->contains(key)) {                                                   \
    qCWarning(qLCAndroidCamera) << "Calling back a QtCamera2 after being destroyed."; \
    return;                                                                           \
  }                                                                                   \
  QFFmpeg::QAndroidCamera *camera = g_qcameras->find(key).value();

static void onPreviewFrameAvailable(JNIEnv *env, jobject obj, jstring cameraId,
                             QtJniTypes::Image image)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->frameAvailable(QJniObject(image));
}
Q_DECLARE_JNI_NATIVE_METHOD(onPreviewFrameAvailable)

static void onStillPhotoAvailable(JNIEnv *env, jobject obj, jstring cameraId,
                             QtJniTypes::Image image)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->frameAvailable(QJniObject(image), true);
}
Q_DECLARE_JNI_NATIVE_METHOD(onStillPhotoAvailable)


static void onCameraOpened(JNIEnv *env, jobject obj, jstring cameraId)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onCameraOpened();
}
Q_DECLARE_JNI_NATIVE_METHOD(onCameraOpened)

static void onCameraDisconnect(JNIEnv *env, jobject obj, jstring cameraId)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onCameraDisconnect();
}
Q_DECLARE_JNI_NATIVE_METHOD(onCameraDisconnect)

static void onCameraError(JNIEnv *env, jobject obj, jstring cameraId, jint error)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onCameraError(error);
}
Q_DECLARE_JNI_NATIVE_METHOD(onCameraError)

static void onCaptureSessionConfigured(JNIEnv *env, jobject obj, jstring cameraId)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onCaptureSessionConfigured();
}
Q_DECLARE_JNI_NATIVE_METHOD(onCaptureSessionConfigured)

static void onCaptureSessionConfigureFailed(JNIEnv *env, jobject obj, jstring cameraId)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onCaptureSessionConfigureFailed();
}
Q_DECLARE_JNI_NATIVE_METHOD(onCaptureSessionConfigureFailed)

static void onSessionActive(JNIEnv *env, jobject obj, jstring cameraId)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onSessionActive();
}
Q_DECLARE_JNI_NATIVE_METHOD(onSessionActive)

static void onSessionClosed(JNIEnv *env, jobject obj, jstring cameraId)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onSessionClosed();
}
Q_DECLARE_JNI_NATIVE_METHOD(onSessionClosed)

static void onCaptureSessionFailed(JNIEnv *env, jobject obj, jstring cameraId, jint reason,
                                   jlong framenumber)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onCaptureSessionFailed(reason, framenumber);
}
Q_DECLARE_JNI_NATIVE_METHOD(onCaptureSessionFailed)

static void onStillPhotoCaptureFailed(JNIEnv *env, jobject obj, jstring cameraId)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onStillPhotoCaptureFailed();
}
Q_DECLARE_JNI_NATIVE_METHOD(onStillPhotoCaptureFailed)

bool QFFmpeg::QAndroidCamera::registerNativeMethods()
{
    static const bool registered = []() {
        return QJniEnvironment().registerNativeMethods(
            QtJniTypes::Traits<QtJniTypes::QtCamera2>::className(),
            {
                Q_JNI_NATIVE_METHOD(onCameraOpened),
                Q_JNI_NATIVE_METHOD(onCameraDisconnect),
                Q_JNI_NATIVE_METHOD(onCameraError),
                Q_JNI_NATIVE_METHOD(onCaptureSessionConfigured),
                Q_JNI_NATIVE_METHOD(onCaptureSessionConfigureFailed),
                Q_JNI_NATIVE_METHOD(onCaptureSessionFailed),
                Q_JNI_NATIVE_METHOD(onPreviewFrameAvailable),
                Q_JNI_NATIVE_METHOD(onStillPhotoAvailable),
                Q_JNI_NATIVE_METHOD(onSessionActive),
                Q_JNI_NATIVE_METHOD(onSessionClosed),
                Q_JNI_NATIVE_METHOD(onStillPhotoCaptureFailed),
            });
    }();
    return registered;
}

QT_END_NAMESPACE
