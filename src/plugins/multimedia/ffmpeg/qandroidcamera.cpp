// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidcamera_p.h"

#include <jni.h>
#include <QMediaFormat>
#include <qmediadevices.h>
#include <qguiapplication.h>
#include <qscreen.h>
#include <qloggingcategory.h>
#include <private/qabstractvideobuffer_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qandroidextras_p.h>
#include <private/qmemoryvideobuffer_p.h>
#include <private/qcameradevice_p.h>
#include <QReadWriteLock>

Q_DECLARE_JNI_CLASS(QtCamera2, "org/qtproject/qt/android/multimedia/QtCamera2");
Q_DECLARE_JNI_CLASS(QtVideoDeviceManager,
                    "org/qtproject/qt/android/multimedia/QtVideoDeviceManager");

Q_DECLARE_JNI_TYPE(AndroidImage, "Landroid/media/Image;")
Q_DECLARE_JNI_TYPE(AndroidImagePlaneArray, "[Landroid/media/Image$Plane;")
Q_DECLARE_JNI_TYPE(JavaByteBuffer, "Ljava/nio/ByteBuffer;")

QT_BEGIN_NAMESPACE
static Q_LOGGING_CATEGORY(qLCAndroidCamera, "qt.multimedia.ffmpeg.androidCamera")

typedef QMap<QString, QAndroidCamera *> QAndroidCameraMap;
Q_GLOBAL_STATIC(QAndroidCameraMap, g_qcameras)
Q_GLOBAL_STATIC(QReadWriteLock, rwLock)

class JavaImageVideoBuffer : public QAbstractVideoBuffer
{
public:
    JavaImageVideoBuffer(const QJniObject &image, const QCameraDevice &device)
        : QAbstractVideoBuffer(QVideoFrame::NoHandle, nullptr),
          m_image(generateImage(image, device)){};

    virtual ~JavaImageVideoBuffer() = default;

    QVideoFrame::MapMode mapMode() const override { return m_mapMode; }

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData mapData;
        if (m_mapMode == QVideoFrame::NotMapped && mode != QVideoFrame::NotMapped
            && !m_image.isNull()) {
            m_mapMode = mode;

            mapData.nPlanes = 1;
            mapData.bytesPerLine[0] = m_image.bytesPerLine();
            mapData.data[0] = m_image.bits();
            mapData.size[0] = m_image.sizeInBytes();
        }

        return mapData;
    }

    void unmap() override { m_mapMode = QVideoFrame::NotMapped; }

    QImage generateImage(const QJniObject &image, const QCameraDevice &device)
    {
        if (!image.isValid())
            return {};

        QJniEnvironment jniEnv;

        QJniObject planes = image.callMethod<QtJniTypes::AndroidImagePlaneArray>("getPlanes");
        if (!planes.isValid())
            return {};

        // this assumes that this image is a JPEG - single plane, that is taken care of in Java
        QJniObject plane = jniEnv->GetObjectArrayElement(planes.object<jobjectArray>(), 0);
        if (jniEnv.checkAndClearExceptions() || !plane.isValid())
            return {};

        QJniObject byteBuffer = plane.callMethod<QtJniTypes::JavaByteBuffer>("getBuffer");
        if (!byteBuffer.isValid())
            return {};

        // Uses direct access which is garanteed by android to work with ImageReader bytebuffer
        uchar *data =
                reinterpret_cast<uchar *>(jniEnv->GetDirectBufferAddress(byteBuffer.object()));
        if (jniEnv.checkAndClearExceptions())
            return {};

        QTransform transform;
        if (device.position() == QCameraDevice::Position::FrontFace)
            transform.scale(-1, 1);

        return QImage::fromData(data, byteBuffer.callMethod<jint>("remaining"))
                .transformed(transform);
    }

    const QImage &image() { return m_image; }

private:
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    QImage m_image;
};

namespace {

QCameraFormat getDefaultCameraFormat()
{
    // default settings
    QCameraFormatPrivate *defaultFormat = new QCameraFormatPrivate{
        .pixelFormat = QVideoFrameFormat::Format_BGRA8888,
        .resolution = { 1920, 1080 },
        .minFrameRate = 30,
        .maxFrameRate = 60,
    };
    return defaultFormat->create();
}

bool checkAndRequestCameraPermission()
{
    const auto key = QStringLiteral("android.permission.CAMERA");

    if (QtAndroidPrivate::checkPermission(key).result() == QtAndroidPrivate::Authorized)
        return true;

    if (QtAndroidPrivate::requestPermission(key).result() != QtAndroidPrivate::Authorized) {
        qCWarning(qLCAndroidCamera) << "User has denied access to camera";
        return false;
    }

    return true;
}

} // namespace

// QAndroidCamera
QAndroidCamera::QAndroidCamera(QCamera *camera) : QPlatformCamera(camera)
{
    m_cameraDevice = (camera ? camera->cameraDevice() : QCameraDevice());
    m_cameraFormat = getDefaultCameraFormat();

    m_jniCamera = QJniObject(QtJniTypes::className<QtJniTypes::QtCamera2>(),
                             QNativeInterface::QAndroidApplication::context());
};

QAndroidCamera::~QAndroidCamera()
{
    QWriteLocker locker(rwLock);
    g_qcameras->remove(m_cameraDevice.id());

    m_jniCamera.callMethod<void>("stopAndClose");
    m_jniCamera.callMethod<void>("stopBackgroundThread");
    setState(State::Closed);
}

void QAndroidCamera::setCamera(const QCameraDevice &camera)
{
    setActive(false);

    m_cameraDevice = camera;
    m_cameraFormat = getDefaultCameraFormat();

    setActive(true);
}

void QAndroidCamera::onFrameAvailable(QJniObject frame)
{
    if (!frame.isValid())
        return;

    long timestamp = frame.callMethod<jlong>("getTimestamp");
    int width = frame.callMethod<jint>("getWidth");
    int height = frame.callMethod<jint>("getHeight");

    QVideoFrameFormat::PixelFormat pixelFormat =
            QVideoFrameFormat::PixelFormat::Format_BGRA8888_Premultiplied;

    QVideoFrameFormat format({ width, height }, pixelFormat);

    QVideoFrame videoFrame(new JavaImageVideoBuffer(frame, m_cameraDevice), format);

    timestamp = timestamp / 1000000;
    if (lastTimestamp == 0)
        lastTimestamp = timestamp;

    videoFrame.setRotationAngle(QVideoFrame::RotationAngle(orientation()));

    if (m_cameraDevice.position() == QCameraDevice::Position::FrontFace)
        videoFrame.setMirrored(true);

    videoFrame.setStartTime(lastTimestamp);
    videoFrame.setEndTime(timestamp);

    emit newVideoFrame(videoFrame);

    lastTimestamp = timestamp;

    // must call close at the end
    frame.callMethod<void>("close");
}

// based on https://developer.android.com/training/camera2/camera-preview#relative_rotation
int QAndroidCamera::orientation()
{
    QJniObject deviceManager(QtJniTypes::className<QtJniTypes::QtVideoDeviceManager>(),
                             QNativeInterface::QAndroidApplication::context());

    QString cameraId = m_cameraDevice.id();
    int sensorOrientation = deviceManager.callMethod<jint>(
            "getSensorOrientation", QJniObject::fromString(cameraId).object<jstring>());

    int deviceOrientation = 0;

    switch (QGuiApplication::primaryScreen()->orientation()) {
    case Qt::PrimaryOrientation:
    case Qt::PortraitOrientation:
        break;
    case Qt::LandscapeOrientation:
        deviceOrientation = 90;
        break;
    case Qt::InvertedPortraitOrientation:
        deviceOrientation = 180;
        break;
    case Qt::InvertedLandscapeOrientation:
        deviceOrientation = 270;
        break;
    }

    int sign = m_cameraDevice.position() == QCameraDevice::Position::FrontFace ? 1 : -1;

    return (sensorOrientation - deviceOrientation * sign + 360) % 360;
}

void QAndroidCamera::setActive(bool active)
{
    if (isActive() == active)
        return;

    if (!m_jniCamera.isValid()) {
        emit error(QCamera::CameraError, "No connection to Android Camera2 API");
        return;
    }

    if (active && checkAndRequestCameraPermission()) {
        QWriteLocker locker(rwLock);
        int width = m_cameraFormat.resolution().width();
        int height = m_cameraFormat.resolution().height();

        if (width < 0 || height < 0) {
            m_cameraFormat = getDefaultCameraFormat();
            width = m_cameraFormat.resolution().width();
            height = m_cameraFormat.resolution().height();
        }

        setState(State::WaitingOpen);
        g_qcameras->insert(m_cameraDevice.id(), this);

        bool canOpen = m_jniCamera.callMethod<jboolean>(
                "open", QJniObject::fromString(m_cameraDevice.id()).object<jstring>(), width,
                height);

        if (!canOpen) {
            g_qcameras->remove(m_cameraDevice.id());
            setState(State::Closed);
            emit error(QCamera::CameraError,
                       QString("Failed to start camera: ").append(m_cameraDevice.description()));
        }

    } else {
        m_jniCamera.callMethod<void>("stopAndClose");
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
    bool wasActive = isActive();

    setActive(false);
    m_cameraFormat = format;

    if (wasActive)
        setActive(true);

    return true;
}

void QAndroidCamera::onCaptureSessionConfigured()
{
    bool canStart = m_jniCamera.callMethod<jboolean>("start", 5);
    setState(canStart ? State::WaitingStart : State::Closed);
}

void QAndroidCamera::onCaptureSessionConfigureFailed()
{
    setState(State::Closed);
}

void QAndroidCamera::onCameraOpened()
{
    if (m_state == State::WaitingOpen) {
        emit error(QCamera::CameraError, "Camera Open in incorrect state.");
        setState(State::Closed);
    }

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
    setState(State::Closed);
}

void QAndroidCamera::onCaptureSessionStarted(long timestamp, long frameNumber)
{
    Q_UNUSED(timestamp);
    Q_UNUSED(frameNumber);

    setState(State::Started);
}

void QAndroidCamera::onCaptureSessionCompleted(long frameNumber)
{
    Q_UNUSED(frameNumber);

    setState(State::Closed);
}

void QAndroidCamera::onCaptureSessionFailed(int reason, long frameNumber)
{
    Q_UNUSED(frameNumber);

    emit error(QCamera::CameraError,
               QString("Capture session failure with Camera %1. Camera2 Api error code: %2")
                       .arg(m_cameraDevice.description())
                       .arg(reason));
    setState(State::Closed);
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

    camera->onFrameAvailable(QJniObject(image));
}
Q_DECLARE_JNI_NATIVE_METHOD(onFrameAvailable)

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

static void onCaptureSessionStarted(JNIEnv *env, jobject obj, jstring cameraId, jlong timestamp,
                                    jlong framenumber)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onCaptureSessionStarted(timestamp, framenumber);
}
Q_DECLARE_JNI_NATIVE_METHOD(onCaptureSessionStarted)

static void onCaptureSessionFailed(JNIEnv *env, jobject obj, jstring cameraId, jint reason,
                                   jlong framenumber)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onCaptureSessionFailed(reason, framenumber);
}
Q_DECLARE_JNI_NATIVE_METHOD(onCaptureSessionFailed)

static void onCaptureSessionCompleted(JNIEnv *env, jobject obj, jstring cameraId, jlong framenumber)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    GET_CAMERA(cameraId);

    camera->onCaptureSessionCompleted(framenumber);
}
Q_DECLARE_JNI_NATIVE_METHOD(onCaptureSessionCompleted)

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
                        Q_JNI_NATIVE_METHOD(onCaptureSessionStarted),
                        Q_JNI_NATIVE_METHOD(onCaptureSessionFailed),
                        Q_JNI_NATIVE_METHOD(onCaptureSessionCompleted),
                        Q_JNI_NATIVE_METHOD(onFrameAvailable),

                });
    }();
    return registered;
}

QT_END_NAMESPACE
