// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidcamera_p.h"

#include <jni.h>
#include <QMediaFormat>
#include <memory>
#include <optional>
#include <qmediadevices.h>
#include <qguiapplication.h>
#include <qscreen.h>
#include <QDebug>
#include <qloggingcategory.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qpermissions.h>
#include <QtCore/private/qandroidextras_p.h>
#include <private/qcameradevice_p.h>
#include <QReadWriteLock>
#include <private/qvideoframeconverter_p.h>
#include <private/qvideotexturehelper_p.h>
#include <qffmpegvideobuffer_p.h>

#include <qandroidcameraframe_p.h>
#include <utility>

extern "C" {
#include "libavutil/hwcontext.h"
#include "libavutil/pixfmt.h"
}

Q_DECLARE_JNI_CLASS(QtCamera2, "org/qtproject/qt/android/multimedia/QtCamera2");
Q_DECLARE_JNI_CLASS(QtVideoDeviceManager,
                    "org/qtproject/qt/android/multimedia/QtVideoDeviceManager");

Q_DECLARE_JNI_CLASS(AndroidImageFormat, "android/graphics/ImageFormat");

Q_DECLARE_JNI_TYPE(AndroidImage, "Landroid/media/Image;")
Q_DECLARE_JNI_TYPE(AndroidImagePlaneArray, "[Landroid/media/Image$Plane;")
Q_DECLARE_JNI_TYPE(JavaByteBuffer, "Ljava/nio/ByteBuffer;")
Q_DECLARE_JNI_TYPE(StringArray, "[Ljava/lang/String;")

QT_BEGIN_NAMESPACE
static Q_LOGGING_CATEGORY(qLCAndroidCamera, "qt.multimedia.ffmpeg.androidCamera");

typedef QMap<QString, QAndroidCamera *> QAndroidCameraMap;
Q_GLOBAL_STATIC(QAndroidCameraMap, g_qcameras)
Q_GLOBAL_STATIC(QReadWriteLock, rwLock)

namespace {

QCameraFormat getDefaultCameraFormat()
{
    // default settings
    QCameraFormatPrivate *defaultFormat = new QCameraFormatPrivate{
        .pixelFormat = QVideoFrameFormat::Format_YUV420P,
        .resolution = { 1920, 1080 },
        .minFrameRate = 30,
        .maxFrameRate = 60,
    };
    return defaultFormat->create();
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
    QJniObject deviceManager(QtJniTypes::className<QtJniTypes::QtVideoDeviceManager>(),
                             QNativeInterface::QAndroidApplication::context());

    if (!deviceManager.isValid()) {
        qCWarning(qLCAndroidCamera) << "Failed to connect to Qt Video Device Manager.";
        return 0;
    }

    return deviceManager.callMethod<jint>("getSensorOrientation",
                                          QJniObject::fromString(cameraId).object<jstring>());
}
} // namespace

// QAndroidCamera

QAndroidCamera::QAndroidCamera(QCamera *camera) : QPlatformCamera(camera)
{
    m_jniCamera = QJniObject(QtJniTypes::className<QtJniTypes::QtCamera2>(),
                             QNativeInterface::QAndroidApplication::context());

    m_hwAccel = QFFmpeg::HWAccel::create(AVHWDeviceType::AV_HWDEVICE_TYPE_MEDIACODEC);
    if (camera) {
        m_cameraDevice = camera->cameraDevice();
        m_cameraFormat = !camera->cameraFormat().isNull() ? camera->cameraFormat()
                                                          : getDefaultCameraFormat();
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
        g_qcameras->remove(m_cameraDevice.id());

        m_jniCamera.callMethod<void>("stopAndClose");
        setState(State::Closed);
    }

    m_jniCamera.callMethod<void>("stopBackgroundThread");
}

void QAndroidCamera::setCamera(const QCameraDevice &camera)
{
    const bool active = isActive();
    if (active)
        setActive(false);

    m_cameraDevice = camera;
    updateCameraCharacteristics();
    m_cameraFormat = getDefaultCameraFormat();

    if (active)
        setActive(true);
}

std::optional<int> QAndroidCamera::ffmpegHWPixelFormat() const
{
    return QFFmpegVideoBuffer::toAVPixelFormat(m_androidFramePixelFormat);
}

static void deleteFrame(void *opaque, uint8_t *data)
{
    Q_UNUSED(data);

    auto frame = reinterpret_cast<QAndroidCameraFrame *>(opaque);

    if (frame)
        delete frame;
}

void QAndroidCamera::frameAvailable(QJniObject image, bool takePhoto)
{
    if (!(m_state == State::WaitingStart || m_state == State::Started) && !m_waitingForFirstFrame) {
        qCWarning(qLCAndroidCamera) << "Received frame when not active... ignoring";
        qCWarning(qLCAndroidCamera) << "state:" << m_state;
        image.callMethod<void>("close");
        return;
    }

    auto androidFrame = new QAndroidCameraFrame(image);
    if (!androidFrame->isParsed()) {
        qCWarning(qLCAndroidCamera) << "Failed to parse frame.. dropping frame";
        delete androidFrame;
        return;
    }

    int timestamp = androidFrame->timestamp();
    m_androidFramePixelFormat = androidFrame->format();
    if (m_waitingForFirstFrame) {
        m_waitingForFirstFrame = false;
        setState(State::Started);
    }
    auto avframe = QFFmpeg::makeAVFrame();

    avframe->width = androidFrame->size().width();
    avframe->height = androidFrame->size().height();
    avframe->format = QFFmpegVideoBuffer::toAVPixelFormat(androidFrame->format());

    avframe->extended_data = avframe->data;
    avframe->pts = androidFrame->timestamp();

    for (int planeNumber = 0; planeNumber < androidFrame->numberPlanes(); planeNumber++) {
        QAndroidCameraFrame::Plane plane = androidFrame->plane(planeNumber);
        avframe->linesize[planeNumber] = plane.rowStride;
        avframe->data[planeNumber] = plane.data;
    }

    avframe->data[3] = nullptr;
    avframe->buf[0] = nullptr;

    avframe->opaque_ref = av_buffer_create(NULL, 1, deleteFrame, androidFrame, 0);
    avframe->extended_data = avframe->data;
    avframe->pts = timestamp;

    QVideoFrameFormat format(androidFrame->size(), androidFrame->format());

    QVideoFrame videoFrame(new QFFmpegVideoBuffer(std::move(avframe)), format);

    if (lastTimestamp == 0)
        lastTimestamp = timestamp;

    videoFrame.setRotationAngle(rotation());
    videoFrame.setMirrored(m_cameraDevice.position() == QCameraDevice::Position::FrontFace);

    videoFrame.setStartTime(lastTimestamp);
    videoFrame.setEndTime(timestamp);

    if (!takePhoto)
        emit newVideoFrame(videoFrame);
    else
        emit onCaptured(videoFrame);

    lastTimestamp = timestamp;
}

QVideoFrame::RotationAngle QAndroidCamera::rotation()
{
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

    int sign = (m_cameraDevice.position() == QCameraDevice::Position::FrontFace) ? 1 : -1;
    int rotation = (sensorOrientation(m_cameraDevice.id()) - deviceOrientation * sign + 360) % 360;

    return QVideoFrame::RotationAngle(rotation);
}

void QAndroidCamera::setActive(bool active)
{
    if (isActive() == active)
        return;

    if (!m_jniCamera.isValid()) {
        emit error(QCamera::CameraError, "No connection to Android Camera2 API");
        return;
    }

    if (active && checkCameraPermission()) {
        QWriteLocker locker(rwLock);
        int width = m_cameraFormat.resolution().width();
        int height = m_cameraFormat.resolution().height();

        if (width < 0 || height < 0) {
            m_cameraFormat = getDefaultCameraFormat();
            width = m_cameraFormat.resolution().width();
            height = m_cameraFormat.resolution().height();
        }

        width = FFALIGN(width, 16);
        height = FFALIGN(height, 16);

        setState(State::WaitingOpen);
        g_qcameras->insert(m_cameraDevice.id(), this);

        bool canOpen = m_jniCamera.callMethod<jboolean>(
                "open", QJniObject::fromString(m_cameraDevice.id()).object<jstring>());

        if (!canOpen) {
            g_qcameras->remove(m_cameraDevice.id());
            setState(State::Closed);
            emit error(QCamera::CameraError,
                       QString("Failed to start camera: ").append(m_cameraDevice.description()));
        }

        // this should use the camera format.
        // but there is only 2 fully supported formats on android - JPG and YUV420P
        // and JPEG is not supported for encoding in FFMpeg, so it's locked for YUV for now.
        const static int imageFormat =
                QJniObject::getStaticField<QtJniTypes::AndroidImageFormat, jint>("YUV_420_888");
        m_jniCamera.callMethod<jboolean>("addImageReader", jint(width), jint(height),
                                         jint(imageFormat));

    } else {
        m_jniCamera.callMethod<void>("stopAndClose");
        m_jniCamera.callMethod<void>("clearSurfaces");
        setState(State::Closed);
    }
}

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

        emit error(QCamera::CameraError,
                   QString("Failed to start Camera %1").arg(m_cameraDevice.description()));
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
    if (!format.isNull() && !m_cameraDevice.videoFormats().contains(format))
        return false;

    m_cameraFormat = format.isNull() ? getDefaultCameraFormat() : format;

    return true;
}

void QAndroidCamera::updateCameraCharacteristics()
{
    if (m_cameraDevice.id().isEmpty()) {
        cleanCameraCharacteristics();
        return;
    }

    QJniObject deviceManager(QtJniTypes::className<QtJniTypes::QtVideoDeviceManager>(),
                             QNativeInterface::QAndroidApplication::context());

    if (!deviceManager.isValid()) {
        qCWarning(qLCAndroidCamera) << "Failed to connect to Qt Video Device Manager.";
        cleanCameraCharacteristics();
        return;
    }

    const float maxZoom = deviceManager.callMethod<jfloat>(
                "getMaxZoom", QJniObject::fromString(m_cameraDevice.id()).object<jstring>());
    maximumZoomFactorChanged(maxZoom);

    m_TorchModeSupported = deviceManager.callMethod<jboolean>(
            "isTorchModeSupported", QJniObject::fromString(m_cameraDevice.id()).object<jstring>());

    m_supportedFlashModes.clear();
    m_supportedFlashModes.append(QCamera::FlashOff);
    QJniObject flashModesObj = deviceManager.callMethod<QtJniTypes::StringArray>(
            "getSupportedFlashModes",
            QJniObject::fromString(m_cameraDevice.id()).object<jstring>());
    QJniEnvironment jniEnv;
    jobjectArray flashModes = flashModesObj.object<jobjectArray>();
    int size = jniEnv->GetArrayLength(flashModes);
    for (int i = 0; i < size; ++i) {
        QJniObject flashModeObj = jniEnv->GetObjectArrayElement(flashModes, i);
        QString flashMode = flashModeObj.toString();
        if (flashMode == QLatin1String("auto"))
            m_supportedFlashModes.append(QCamera::FlashAuto);
        else if (flashMode == QLatin1String("on"))
            m_supportedFlashModes.append(QCamera::FlashOn);
    }
}

void QAndroidCamera::cleanCameraCharacteristics()
{
    maximumZoomFactorChanged(1.0);
    if (torchMode() != QCamera::TorchOff) {
        setTorchMode(QCamera::TorchOff);
    }
    m_TorchModeSupported = false;

    if (flashMode() != QCamera::FlashOff) {
        setFlashMode(QCamera::FlashOff);
    }
    m_supportedFlashModes.clear();
    m_supportedFlashModes.append(QCamera::FlashOff);
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
    m_jniCamera.callMethod<void>("zoomTo", factor);
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

void QAndroidCamera::onCaptureSessionConfigured()
{
    bool canStart = m_jniCamera.callMethod<jboolean>("start", 3);
    setState(canStart ? State::WaitingStart : State::Closed);
}

void QAndroidCamera::onCaptureSessionConfigureFailed()
{
    setState(State::Closed);
}

void QAndroidCamera::onCameraOpened()
{
    bool canStart = m_jniCamera.callMethod<jboolean>("createSession");
    setState(canStart ? State::WaitingStart : State::Closed);
}

void QAndroidCamera::onCameraDisconnect()
{
    setState(State::Closed);
}

void QAndroidCamera::onCameraError(int reason)
{
    emit error(QCamera::CameraError,
               QString("Capture error with Camera %1. Camera2 Api error code: %2")
                       .arg(m_cameraDevice.description())
                       .arg(reason));
}

void QAndroidCamera::onSessionActive()
{
    m_waitingForFirstFrame = true;
}

void QAndroidCamera::onSessionClosed()
{
    m_waitingForFirstFrame = false;
    setState(State::Closed);
}

void QAndroidCamera::capture()
{
    m_jniCamera.callMethod<void>("takePhoto");
}

void QAndroidCamera::updateExif(const QString &filename)
{
    m_jniCamera.callMethod<void>("saveExifToFile", QJniObject::fromString(filename).object<jstring>());
}

void QAndroidCamera::onCaptureSessionFailed(int reason, long frameNumber)
{
    Q_UNUSED(frameNumber);

    emit error(QCamera::CameraError,
               QString("Capture session failure with Camera %1. Camera2 Api error code: %2")
                       .arg(m_cameraDevice.description())
                       .arg(reason));
}

// JNI logic

#define GET_CAMERA(cameraId)                                                          \
  QString key = QJniObject(cameraId).toString();                                      \
  QReadLocker locker(rwLock);                                                         \
  if (!g_qcameras->contains(key)) {                                                   \
    qCWarning(qLCAndroidCamera) << "Calling back a QtCamera2 after being destroyed."; \
    return;                                                                           \
  }                                                                                   \
  QAndroidCamera *camera = g_qcameras->find(key).value();

static void onFrameAvailable(JNIEnv *env, jobject obj, jstring cameraId,
                             QtJniTypes::AndroidImage image)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->frameAvailable(QJniObject(image));
}
Q_DECLARE_JNI_NATIVE_METHOD(onFrameAvailable)

static void onPhotoAvailable(JNIEnv *env, jobject obj, jstring cameraId,
                             QtJniTypes::AndroidImage image)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->frameAvailable(QJniObject(image), true);
}
Q_DECLARE_JNI_NATIVE_METHOD(onPhotoAvailable)


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

bool QAndroidCamera::registerNativeMethods()
{
    static const bool registered = []() {
        return QJniEnvironment().registerNativeMethods(
                QtJniTypes::className<QtJniTypes::QtCamera2>(),
                {
                        Q_JNI_NATIVE_METHOD(onCameraOpened),
                        Q_JNI_NATIVE_METHOD(onCameraDisconnect),
                        Q_JNI_NATIVE_METHOD(onCameraError),
                        Q_JNI_NATIVE_METHOD(onCaptureSessionConfigured),
                        Q_JNI_NATIVE_METHOD(onCaptureSessionConfigureFailed),
                        Q_JNI_NATIVE_METHOD(onCaptureSessionFailed),
                        Q_JNI_NATIVE_METHOD(onFrameAvailable),
                        Q_JNI_NATIVE_METHOD(onPhotoAvailable),
                        Q_JNI_NATIVE_METHOD(onSessionActive),
                        Q_JNI_NATIVE_METHOD(onSessionClosed),
                });
    }();
    return registered;
}

QT_END_NAMESPACE
