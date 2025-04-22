// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Ruslan Baratov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androidcamera_p.h"
#include "androidsurfacetexture_p.h"
#include "androidsurfaceview_p.h"
#include "qandroidmultimediautils_p.h"
#include "qandroidglobal_p.h"

#include <private/qvideoframe_p.h>

#include <qhash.h>
#include <qstringlist.h>
#include <qdebug.h>
#include <QtCore/qthread.h>
#include <QtCore/qreadwritelock.h>
#include <QtCore/qmutex.h>
#include <QtMultimedia/private/qmemoryvideobuffer_p.h>
#include <QtCore/qcoreapplication.h>

#include <mutex>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcAndroidCamera, "qt.multimedia.android.camera");

static const char QtCameraListenerClassName[] = "org/qtproject/qt/android/multimedia/QtCameraListener";

typedef QHash<int, AndroidCamera *> CameraMap;
Q_GLOBAL_STATIC(CameraMap, cameras)
Q_GLOBAL_STATIC(QReadWriteLock, rwLock)

static QRect areaToRect(jobject areaObj)
{
    QJniObject area(areaObj);
    QJniObject rect = area.getObjectField("rect", "Landroid/graphics/Rect;");

    return QRect(rect.getField<jint>("left"),
                 rect.getField<jint>("top"),
                 rect.callMethod<jint>("width"),
                 rect.callMethod<jint>("height"));
}

static QJniObject rectToArea(const QRect &rect)
{
    QJniObject jrect("android/graphics/Rect",
                     "(IIII)V",
                     rect.left(), rect.top(), rect.right(), rect.bottom());

    QJniObject area("android/hardware/Camera$Area",
                    "(Landroid/graphics/Rect;I)V",
                    jrect.object(), 500);

    return area;
}

// native method for QtCameraLisener.java
static void notifyAutoFocusComplete(JNIEnv* , jobject, int id, jboolean success)
{
    QReadLocker locker(rwLock);
    const auto it = cameras->constFind(id);
    if (Q_UNLIKELY(it == cameras->cend()))
        return;

    Q_EMIT (*it)->autoFocusComplete(success);
}

static void notifyPictureExposed(JNIEnv* , jobject, int id)
{
    QReadLocker locker(rwLock);
    const auto it = cameras->constFind(id);
    if (Q_UNLIKELY(it == cameras->cend()))
        return;

    Q_EMIT (*it)->pictureExposed();
}

static void notifyPictureCaptured(JNIEnv *env, jobject, int id, jbyteArray data)
{
    QReadLocker locker(rwLock);
    const auto it = cameras->constFind(id);
    if (Q_UNLIKELY(it == cameras->cend())) {
        qCWarning(lcAndroidCamera) << "Could not obtain camera!";
        return;
    }

    AndroidCamera *camera = (*it);

    const int arrayLength = env->GetArrayLength(data);
    QByteArray bytes(arrayLength, Qt::Uninitialized);
    env->GetByteArrayRegion(data, 0, arrayLength, reinterpret_cast<jbyte *>(bytes.data()));

    auto parameters = camera->getParametersObject();

    QJniObject size =
            parameters.callObjectMethod("getPictureSize", "()Landroid/hardware/Camera$Size;");

    if (!size.isValid()) {
        qCWarning(lcAndroidCamera) << "Picture Size is not valid!";
        return;
    }

    QSize pictureSize(size.getField<jint>("width"), size.getField<jint>("height"));

    auto format = AndroidCamera::ImageFormat(parameters.callMethod<jint>("getPictureFormat"));

    if (format == AndroidCamera::ImageFormat::UnknownImageFormat) {
        qCWarning(lcAndroidCamera) << "Android Camera Image Format is UnknownImageFormat!";
        return;
    }

    int bytesPerLine = 0;

    switch (format) {
    case AndroidCamera::ImageFormat::YV12:
        bytesPerLine = (pictureSize.width() + 15) & ~15;
        break;
    case AndroidCamera::ImageFormat::NV21:
        bytesPerLine = pictureSize.width();
        break;
    case AndroidCamera::ImageFormat::RGB565:
    case AndroidCamera::ImageFormat::YUY2:
        bytesPerLine = pictureSize.width() * 2;
        break;
    default:
        bytesPerLine = -1;
    }

    auto pictureFormat = qt_pixelFormatFromAndroidImageFormat(format);

    emit camera->pictureCaptured(bytes, pictureFormat, pictureSize, bytesPerLine);
}

static void notifyNewPreviewFrame(JNIEnv *env, jobject, int id, jbyteArray data,
                                  int width, int height, int format, int bpl)
{
    QReadLocker locker(rwLock);
    const auto it = cameras->constFind(id);
    if (Q_UNLIKELY(it == cameras->cend()))
        return;

    const int arrayLength = env->GetArrayLength(data);
    if (arrayLength == 0)
        return;

    QByteArray bytes(arrayLength, Qt::Uninitialized);
    env->GetByteArrayRegion(data, 0, arrayLength, (jbyte*)bytes.data());

    QVideoFrameFormat frameFormat(
            QSize(width, height),
            qt_pixelFormatFromAndroidImageFormat(AndroidCamera::ImageFormat(format)));

    QVideoFrame frame = QVideoFramePrivate::createFrame(
            std::make_unique<QMemoryVideoBuffer>(std::move(bytes), bpl), std::move(frameFormat));

    Q_EMIT (*it)->newPreviewFrame(frame);
}

static void notifyFrameAvailable(JNIEnv *, jobject, int id)
{
    QReadLocker locker(rwLock);
    const auto it = cameras->constFind(id);
    if (Q_UNLIKELY(it == cameras->cend()))
        return;

    (*it)->fetchLastPreviewFrame();
}

class AndroidCameraPrivate : public QObject
{
    Q_OBJECT
public:
    AndroidCameraPrivate();
    ~AndroidCameraPrivate();

    Q_INVOKABLE bool init(int cameraId);

    Q_INVOKABLE void release();
    Q_INVOKABLE bool lock();
    Q_INVOKABLE bool unlock();
    Q_INVOKABLE bool reconnect();

    Q_INVOKABLE AndroidCamera::CameraFacing getFacing();
    Q_INVOKABLE int getNativeOrientation();

    Q_INVOKABLE QSize getPreferredPreviewSizeForVideo();
    Q_INVOKABLE QList<QSize> getSupportedPreviewSizes();
    static QList<QSize> getSupportedPreviewSizes(QJniObject &parameters);

    Q_INVOKABLE QList<AndroidCamera::FpsRange> getSupportedPreviewFpsRange();

    Q_INVOKABLE AndroidCamera::FpsRange getPreviewFpsRange();
    static AndroidCamera::FpsRange getPreviewFpsRange(QJniObject &parameters);
    Q_INVOKABLE void setPreviewFpsRange(int min, int max);

    Q_INVOKABLE AndroidCamera::ImageFormat getPreviewFormat();
    Q_INVOKABLE void setPreviewFormat(AndroidCamera::ImageFormat fmt);
    Q_INVOKABLE QList<AndroidCamera::ImageFormat> getSupportedPreviewFormats();
    static QList<AndroidCamera::ImageFormat> getSupportedPreviewFormats(QJniObject &parameters);

    Q_INVOKABLE QSize previewSize() const { return m_previewSize; }
    Q_INVOKABLE QSize getPreviewSize();
    Q_INVOKABLE void updatePreviewSize();
    Q_INVOKABLE bool setPreviewTexture(void *surfaceTexture);
    Q_INVOKABLE bool setPreviewDisplay(void *surfaceHolder);
    Q_INVOKABLE void setDisplayOrientation(int degrees);

    Q_INVOKABLE bool isZoomSupported();
    Q_INVOKABLE int getMaxZoom();
    Q_INVOKABLE QList<int> getZoomRatios();
    Q_INVOKABLE int getZoom();
    Q_INVOKABLE void setZoom(int value);

    Q_INVOKABLE QString getFlashMode();
    Q_INVOKABLE void setFlashMode(const QString &value);

    Q_INVOKABLE QString getFocusMode();
    Q_INVOKABLE void setFocusMode(const QString &value);

    Q_INVOKABLE int getMaxNumFocusAreas();
    Q_INVOKABLE QList<QRect> getFocusAreas();
    Q_INVOKABLE void setFocusAreas(const QList<QRect> &areas);

    Q_INVOKABLE void autoFocus();
    Q_INVOKABLE void cancelAutoFocus();

    Q_INVOKABLE bool isAutoExposureLockSupported();
    Q_INVOKABLE bool getAutoExposureLock();
    Q_INVOKABLE void setAutoExposureLock(bool toggle);

    Q_INVOKABLE bool isAutoWhiteBalanceLockSupported();
    Q_INVOKABLE bool getAutoWhiteBalanceLock();
    Q_INVOKABLE void setAutoWhiteBalanceLock(bool toggle);

    Q_INVOKABLE int getExposureCompensation();
    Q_INVOKABLE void setExposureCompensation(int value);
    Q_INVOKABLE float getExposureCompensationStep();
    Q_INVOKABLE int getMinExposureCompensation();
    Q_INVOKABLE int getMaxExposureCompensation();

    Q_INVOKABLE QString getSceneMode();
    Q_INVOKABLE void setSceneMode(const QString &value);

    Q_INVOKABLE QString getWhiteBalance();
    Q_INVOKABLE void setWhiteBalance(const QString &value);

    Q_INVOKABLE void updateRotation();

    Q_INVOKABLE QList<QSize> getSupportedPictureSizes();
    Q_INVOKABLE QList<QSize> getSupportedVideoSizes();
    Q_INVOKABLE void setPictureSize(const QSize &size);
    Q_INVOKABLE void setJpegQuality(int quality);

    Q_INVOKABLE void startPreview();
    Q_INVOKABLE void stopPreview();

    Q_INVOKABLE void takePicture();

    Q_INVOKABLE void setupPreviewFrameCallback();
    Q_INVOKABLE void notifyNewFrames(bool notify);
    Q_INVOKABLE void fetchLastPreviewFrame();

    Q_INVOKABLE void applyParameters();

    Q_INVOKABLE QStringList callParametersStringListMethod(const QByteArray &methodName);

    int m_cameraId;
    QRecursiveMutex m_parametersMutex;
    QSize m_previewSize;
    int m_rotation;
    QJniObject m_info;
    QJniObject m_parameters;
    QJniObject m_camera;
    QJniObject m_cameraListener;

Q_SIGNALS:
    void previewSizeChanged();
    void previewStarted();
    void previewFailedToStart();
    void previewStopped();

    void autoFocusStarted();

    void whiteBalanceChanged();

    void takePictureFailed();

    void lastPreviewFrameFetched(const QVideoFrame &frame);
};

AndroidCamera::AndroidCamera(AndroidCameraPrivate *d, QThread *worker)
    : QObject(),
      d_ptr(d),
      m_worker(worker)

{
    connect(d, &AndroidCameraPrivate::previewSizeChanged, this, &AndroidCamera::previewSizeChanged);
    connect(d, &AndroidCameraPrivate::previewStarted, this, &AndroidCamera::previewStarted);
    connect(d, &AndroidCameraPrivate::previewFailedToStart, this, &AndroidCamera::previewFailedToStart);
    connect(d, &AndroidCameraPrivate::previewStopped, this, &AndroidCamera::previewStopped);
    connect(d, &AndroidCameraPrivate::autoFocusStarted, this, &AndroidCamera::autoFocusStarted);
    connect(d, &AndroidCameraPrivate::whiteBalanceChanged, this, &AndroidCamera::whiteBalanceChanged);
    connect(d, &AndroidCameraPrivate::takePictureFailed, this, &AndroidCamera::takePictureFailed);
    connect(d, &AndroidCameraPrivate::lastPreviewFrameFetched, this, &AndroidCamera::lastPreviewFrameFetched);
}

AndroidCamera::~AndroidCamera()
{
    Q_D(AndroidCamera);
    if (d->m_camera.isValid()) {
        release();
        QWriteLocker locker(rwLock);
        cameras->remove(cameraId());
    }

    m_worker->exit();
    m_worker->wait(5000);
}

AndroidCamera *AndroidCamera::open(int cameraId)
{
    if (!qt_androidCheckCameraPermission())
        return nullptr;

    AndroidCameraPrivate *d = new AndroidCameraPrivate();
    QThread *worker = new QThread;
    worker->start();
    d->moveToThread(worker);
    connect(worker, &QThread::finished, d, &AndroidCameraPrivate::deleteLater);
    bool ok = true;
    QMetaObject::invokeMethod(d, "init", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, ok), Q_ARG(int, cameraId));
    if (!ok) {
        worker->quit();
        worker->wait(5000);
        delete worker;
        return 0;
    }

    AndroidCamera *q = new AndroidCamera(d, worker);
    QWriteLocker locker(rwLock);
    cameras->insert(cameraId, q);

    return q;
}

int AndroidCamera::cameraId() const
{
    Q_D(const AndroidCamera);
    return d->m_cameraId;
}

bool AndroidCamera::lock()
{
    Q_D(AndroidCamera);
    bool ok = true;
    QMetaObject::invokeMethod(d, "lock", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, ok));
    return ok;
}

bool AndroidCamera::unlock()
{
    Q_D(AndroidCamera);
    bool ok = true;
    QMetaObject::invokeMethod(d, "unlock", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, ok));
    return ok;
}

bool AndroidCamera::reconnect()
{
    Q_D(AndroidCamera);
    bool ok = true;
    QMetaObject::invokeMethod(d, "reconnect", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, ok));
    return ok;
}

void AndroidCamera::release()
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "release", Qt::BlockingQueuedConnection);
}

AndroidCamera::CameraFacing AndroidCamera::getFacing()
{
    Q_D(AndroidCamera);
    return d->getFacing();
}

int AndroidCamera::getNativeOrientation()
{
    Q_D(AndroidCamera);
    return d->getNativeOrientation();
}

QSize AndroidCamera::getPreferredPreviewSizeForVideo()
{
    Q_D(AndroidCamera);
    return d->getPreferredPreviewSizeForVideo();
}

QList<QSize> AndroidCamera::getSupportedPreviewSizes()
{
    Q_D(AndroidCamera);
    return d->getSupportedPreviewSizes();
}

QList<AndroidCamera::FpsRange> AndroidCamera::getSupportedPreviewFpsRange()
{
    Q_D(AndroidCamera);
    return d->getSupportedPreviewFpsRange();
}

AndroidCamera::FpsRange AndroidCamera::getPreviewFpsRange()
{
    Q_D(AndroidCamera);
    return d->getPreviewFpsRange();
}

void AndroidCamera::setPreviewFpsRange(FpsRange range)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setPreviewFpsRange", Q_ARG(int, range.min), Q_ARG(int, range.max));
}

AndroidCamera::ImageFormat AndroidCamera::getPreviewFormat()
{
    Q_D(AndroidCamera);
    return d->getPreviewFormat();
}

void AndroidCamera::setPreviewFormat(ImageFormat fmt)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setPreviewFormat", Q_ARG(AndroidCamera::ImageFormat, fmt));
}

QList<AndroidCamera::ImageFormat> AndroidCamera::getSupportedPreviewFormats()
{
    Q_D(AndroidCamera);
    return d->getSupportedPreviewFormats();
}

QSize AndroidCamera::previewSize() const
{
    Q_D(const AndroidCamera);
    return d->m_previewSize;
}

QSize AndroidCamera::actualPreviewSize()
{
    Q_D(AndroidCamera);
    return d->getPreviewSize();
}

void AndroidCamera::setPreviewSize(const QSize &size)
{
    Q_D(AndroidCamera);
    d->m_parametersMutex.lock();
    bool areParametersValid = d->m_parameters.isValid();
    d->m_parametersMutex.unlock();
    if (!areParametersValid)
        return;

    d->m_previewSize = size;
    QMetaObject::invokeMethod(d, "updatePreviewSize");
}

bool AndroidCamera::setPreviewTexture(AndroidSurfaceTexture *surfaceTexture)
{
    Q_D(AndroidCamera);
    bool ok = true;
    QMetaObject::invokeMethod(d,
                              "setPreviewTexture",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, ok),
                              Q_ARG(void *, surfaceTexture ? surfaceTexture->surfaceTexture() : 0));
    return ok;
}

bool AndroidCamera::setPreviewDisplay(AndroidSurfaceHolder *surfaceHolder)
{
    Q_D(AndroidCamera);
    bool ok = true;
    QMetaObject::invokeMethod(d,
                              "setPreviewDisplay",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, ok),
                              Q_ARG(void *, surfaceHolder ? surfaceHolder->surfaceHolder() : 0));
    return ok;
}

void AndroidCamera::setDisplayOrientation(int degrees)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setDisplayOrientation", Qt::QueuedConnection, Q_ARG(int, degrees));
}

bool AndroidCamera::isZoomSupported()
{
    Q_D(AndroidCamera);
    return d->isZoomSupported();
}

int AndroidCamera::getMaxZoom()
{
    Q_D(AndroidCamera);
    return d->getMaxZoom();
}

QList<int> AndroidCamera::getZoomRatios()
{
    Q_D(AndroidCamera);
    return d->getZoomRatios();
}

int AndroidCamera::getZoom()
{
    Q_D(AndroidCamera);
    return d->getZoom();
}

void AndroidCamera::setZoom(int value)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setZoom", Q_ARG(int, value));
}

QStringList AndroidCamera::getSupportedFlashModes()
{
    Q_D(AndroidCamera);
    return d->callParametersStringListMethod("getSupportedFlashModes");
}

QString AndroidCamera::getFlashMode()
{
    Q_D(AndroidCamera);
    return d->getFlashMode();
}

void AndroidCamera::setFlashMode(const QString &value)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setFlashMode", Q_ARG(QString, value));
}

QStringList AndroidCamera::getSupportedFocusModes()
{
    Q_D(AndroidCamera);
    return d->callParametersStringListMethod("getSupportedFocusModes");
}

QString AndroidCamera::getFocusMode()
{
    Q_D(AndroidCamera);
    return d->getFocusMode();
}

void AndroidCamera::setFocusMode(const QString &value)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setFocusMode", Q_ARG(QString, value));
}

int AndroidCamera::getMaxNumFocusAreas()
{
    Q_D(AndroidCamera);
    return d->getMaxNumFocusAreas();
}

QList<QRect> AndroidCamera::getFocusAreas()
{
    Q_D(AndroidCamera);
    return d->getFocusAreas();
}

void AndroidCamera::setFocusAreas(const QList<QRect> &areas)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setFocusAreas", Q_ARG(QList<QRect>, areas));
}

void AndroidCamera::autoFocus()
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "autoFocus");
}

void AndroidCamera::cancelAutoFocus()
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "cancelAutoFocus", Qt::QueuedConnection);
}

bool AndroidCamera::isAutoExposureLockSupported()
{
    Q_D(AndroidCamera);
    return d->isAutoExposureLockSupported();
}

bool AndroidCamera::getAutoExposureLock()
{
    Q_D(AndroidCamera);
    return d->getAutoExposureLock();
}

void AndroidCamera::setAutoExposureLock(bool toggle)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setAutoExposureLock", Q_ARG(bool, toggle));
}

bool AndroidCamera::isAutoWhiteBalanceLockSupported()
{
    Q_D(AndroidCamera);
    return d->isAutoWhiteBalanceLockSupported();
}

bool AndroidCamera::getAutoWhiteBalanceLock()
{
    Q_D(AndroidCamera);
    return d->getAutoWhiteBalanceLock();
}

void AndroidCamera::setAutoWhiteBalanceLock(bool toggle)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setAutoWhiteBalanceLock", Q_ARG(bool, toggle));
}

int AndroidCamera::getExposureCompensation()
{
    Q_D(AndroidCamera);
    return d->getExposureCompensation();
}

void AndroidCamera::setExposureCompensation(int value)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setExposureCompensation", Q_ARG(int, value));
}

float AndroidCamera::getExposureCompensationStep()
{
    Q_D(AndroidCamera);
    return d->getExposureCompensationStep();
}

int AndroidCamera::getMinExposureCompensation()
{
    Q_D(AndroidCamera);
    return d->getMinExposureCompensation();
}

int AndroidCamera::getMaxExposureCompensation()
{
    Q_D(AndroidCamera);
    return d->getMaxExposureCompensation();
}

QStringList AndroidCamera::getSupportedSceneModes()
{
    Q_D(AndroidCamera);
    return d->callParametersStringListMethod("getSupportedSceneModes");
}

QString AndroidCamera::getSceneMode()
{
    Q_D(AndroidCamera);
    return d->getSceneMode();
}

void AndroidCamera::setSceneMode(const QString &value)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setSceneMode", Q_ARG(QString, value));
}

QStringList AndroidCamera::getSupportedWhiteBalance()
{
    Q_D(AndroidCamera);
    return d->callParametersStringListMethod("getSupportedWhiteBalance");
}

QString AndroidCamera::getWhiteBalance()
{
    Q_D(AndroidCamera);
    return d->getWhiteBalance();
}

void AndroidCamera::setWhiteBalance(const QString &value)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setWhiteBalance", Q_ARG(QString, value));
}

void AndroidCamera::setRotation(int rotation)
{
    Q_D(AndroidCamera);
    //We need to do it here and not in worker class because we cache rotation
    d->m_parametersMutex.lock();
    bool areParametersValid = d->m_parameters.isValid();
    d->m_parametersMutex.unlock();
    if (!areParametersValid)
        return;

    d->m_rotation = rotation;
    QMetaObject::invokeMethod(d, "updateRotation");
}

int AndroidCamera::getRotation() const
{
    Q_D(const AndroidCamera);
    return d->m_rotation;
}

QList<QSize> AndroidCamera::getSupportedPictureSizes()
{
    Q_D(AndroidCamera);
    return d->getSupportedPictureSizes();
}

QList<QSize> AndroidCamera::getSupportedVideoSizes()
{
    Q_D(AndroidCamera);
    return d->getSupportedVideoSizes();
}

void AndroidCamera::setPictureSize(const QSize &size)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setPictureSize", Q_ARG(QSize, size));
}

void AndroidCamera::setJpegQuality(int quality)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setJpegQuality", Q_ARG(int, quality));
}

void AndroidCamera::takePicture()
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "takePicture", Qt::BlockingQueuedConnection);
}

void AndroidCamera::setupPreviewFrameCallback()
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "setupPreviewFrameCallback");
}

void AndroidCamera::notifyNewFrames(bool notify)
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "notifyNewFrames", Q_ARG(bool, notify));
}

void AndroidCamera::fetchLastPreviewFrame()
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "fetchLastPreviewFrame");
}

QJniObject AndroidCamera::getCameraObject()
{
    Q_D(AndroidCamera);
    return d->m_camera;
}

int AndroidCamera::getNumberOfCameras()
{
    if (!qt_androidCheckCameraPermission())
        return 0;

    return QJniObject::callStaticMethod<jint>("android/hardware/Camera",
                                                     "getNumberOfCameras");
}

void AndroidCamera::getCameraInfo(int id, QCameraDevicePrivate *info)
{
    Q_ASSERT(info);

    QJniObject cameraInfo("android/hardware/Camera$CameraInfo");
    QJniObject::callStaticMethod<void>("android/hardware/Camera",
                                              "getCameraInfo",
                                              "(ILandroid/hardware/Camera$CameraInfo;)V",
                                              id, cameraInfo.object());

    AndroidCamera::CameraFacing facing = AndroidCamera::CameraFacing(cameraInfo.getField<jint>("facing"));
    // The orientation provided by Android is counter-clockwise, we need it clockwise
    info->orientation = (360 - cameraInfo.getField<jint>("orientation")) % 360;

    switch (facing) {
    case AndroidCamera::CameraFacingBack:
        info->id = QByteArray("back");
        info->description = QStringLiteral("Rear-facing camera");
        info->position = QCameraDevice::BackFace;
        info->isDefault = true;
        break;
    case AndroidCamera::CameraFacingFront:
        info->id = QByteArray("front");
        info->description = QStringLiteral("Front-facing camera");
        info->position = QCameraDevice::FrontFace;
        break;
    default:
        break;
    }
    // Add a number to allow correct access to cameras on systems with two
    // (and more) front/back cameras
    if (id > 1) {
        info->id.append(QByteArray::number(id));
        info->description.append(QStringLiteral(" %1").arg(id));
    }
}

QVideoFrameFormat::PixelFormat AndroidCamera::QtPixelFormatFromAndroidImageFormat(AndroidCamera::ImageFormat format)
{
    switch (format) {
    case AndroidCamera::NV21:
        return QVideoFrameFormat::Format_NV21;
    case AndroidCamera::YUY2:
        return QVideoFrameFormat::Format_YUYV;
    case AndroidCamera::JPEG:
        return QVideoFrameFormat::Format_Jpeg;
    case AndroidCamera::YV12:
        return QVideoFrameFormat::Format_YV12;
    default:
        return QVideoFrameFormat::Format_Invalid;
    }
}

AndroidCamera::ImageFormat AndroidCamera::AndroidImageFormatFromQtPixelFormat(QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
    case QVideoFrameFormat::Format_NV21:
        return AndroidCamera::NV21;
    case QVideoFrameFormat::Format_YUYV:
        return AndroidCamera::YUY2;
    case QVideoFrameFormat::Format_Jpeg:
        return AndroidCamera::JPEG;
    case QVideoFrameFormat::Format_YV12:
        return AndroidCamera::YV12;
    default:
        return AndroidCamera::UnknownImageFormat;
    }
}

QList<QCameraFormat> AndroidCamera::getSupportedFormats()
{
    QList<QCameraFormat> formats;
    AndroidCamera::FpsRange range = getPreviewFpsRange();
    for (const auto &previewSize : getSupportedVideoSizes()) {
        for (const auto &previewFormat : getSupportedPreviewFormats()) {
            QCameraFormatPrivate * format = new QCameraFormatPrivate();
            format->pixelFormat = QtPixelFormatFromAndroidImageFormat(previewFormat);
            format->resolution = previewSize;
            format->minFrameRate = range.min;
            format->maxFrameRate = range.max;
            formats.append(format->create());
        }
    }

    return formats;
}

void AndroidCamera::startPreview()
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "startPreview");
}

void AndroidCamera::stopPreview()
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "stopPreview");
}

void AndroidCamera::stopPreviewSynchronous()
{
    Q_D(AndroidCamera);
    QMetaObject::invokeMethod(d, "stopPreview", Qt::BlockingQueuedConnection);
}

QJniObject AndroidCamera::getParametersObject()
{
    Q_D(AndroidCamera);
    return d->m_parameters;
}

AndroidCameraPrivate::AndroidCameraPrivate()
    : QObject()
{
}

AndroidCameraPrivate::~AndroidCameraPrivate()
{
}

static qint32 s_activeCameras = 0;

bool AndroidCameraPrivate::init(int cameraId)
{
    m_cameraId = cameraId;
    QJniEnvironment env;

    const bool opened = s_activeCameras & (1 << cameraId);
    if (opened)
        return false;

    m_camera = QJniObject::callStaticObjectMethod("android/hardware/Camera",
                                                  "open",
                                                  "(I)Landroid/hardware/Camera;",
                                                  cameraId);
    if (!m_camera.isValid())
        return false;

    m_cameraListener = QJniObject(QtCameraListenerClassName, "(I)V", m_cameraId);
    m_info = QJniObject("android/hardware/Camera$CameraInfo");
    m_camera.callStaticMethod<void>("android/hardware/Camera",
                                    "getCameraInfo",
                                    "(ILandroid/hardware/Camera$CameraInfo;)V",
                                    cameraId,
                                    m_info.object());

    QJniObject params = m_camera.callObjectMethod("getParameters",
                                                         "()Landroid/hardware/Camera$Parameters;");
    m_parameters = QJniObject(params);
    s_activeCameras |= 1 << cameraId;

    return true;
}

void AndroidCameraPrivate::release()
{
    m_previewSize = QSize();
    m_parametersMutex.lock();
    m_parameters = QJniObject();
    m_parametersMutex.unlock();
    if (m_camera.isValid()) {
        m_camera.callMethod<void>("release");
        s_activeCameras &= ~(1 << m_cameraId);
    }
}

bool AndroidCameraPrivate::lock()
{
    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_camera.objectClass(), "lock", "()V");
    env->CallVoidMethod(m_camera.object(), methodId);

    if (env.checkAndClearExceptions())
        return false;
    return true;
}

bool AndroidCameraPrivate::unlock()
{
    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_camera.objectClass(), "unlock", "()V");
    env->CallVoidMethod(m_camera.object(), methodId);

    if (env.checkAndClearExceptions())
        return false;
    return true;
}

bool AndroidCameraPrivate::reconnect()
{
    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_camera.objectClass(), "reconnect", "()V");
    env->CallVoidMethod(m_camera.object(), methodId);

    if (env.checkAndClearExceptions())
        return false;
    return true;
}

AndroidCamera::CameraFacing AndroidCameraPrivate::getFacing()
{
    return AndroidCamera::CameraFacing(m_info.getField<jint>("facing"));
}

int AndroidCameraPrivate::getNativeOrientation()
{
    return m_info.getField<jint>("orientation");
}

QSize AndroidCameraPrivate::getPreferredPreviewSizeForVideo()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return QSize();

    QJniObject size = m_parameters.callObjectMethod("getPreferredPreviewSizeForVideo",
                                                    "()Landroid/hardware/Camera$Size;");

    if (!size.isValid())
        return QSize();

    return QSize(size.getField<jint>("width"), size.getField<jint>("height"));
}

QList<QSize> AndroidCameraPrivate::getSupportedPreviewSizes()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);
    return getSupportedPreviewSizes(m_parameters);
}

QList<QSize> AndroidCameraPrivate::getSupportedPreviewSizes(QJniObject &parameters)
{
    QList<QSize> list;

    if (parameters.isValid()) {
        QJniObject sizeList = parameters.callObjectMethod("getSupportedPreviewSizes",
                                                            "()Ljava/util/List;");
        int count = sizeList.callMethod<jint>("size");
        for (int i = 0; i < count; ++i) {
            QJniObject size = sizeList.callObjectMethod("get",
                                                        "(I)Ljava/lang/Object;",
                                                        i);
            list.append(QSize(size.getField<jint>("width"), size.getField<jint>("height")));
        }

        std::sort(list.begin(), list.end(), qt_sizeLessThan);
    }

    return list;
}

QList<AndroidCamera::FpsRange> AndroidCameraPrivate::getSupportedPreviewFpsRange()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    QJniEnvironment env;

    QList<AndroidCamera::FpsRange> rangeList;

    if (m_parameters.isValid()) {
        QJniObject rangeListNative = m_parameters.callObjectMethod("getSupportedPreviewFpsRange",
                                                                   "()Ljava/util/List;");
        int count = rangeListNative.callMethod<jint>("size");

        rangeList.reserve(count);

        for (int i = 0; i < count; ++i) {
            QJniObject range = rangeListNative.callObjectMethod("get",
                                                                "(I)Ljava/lang/Object;",
                                                                i);

            jintArray jRange = static_cast<jintArray>(range.object());
            jint* rangeArray = env->GetIntArrayElements(jRange, 0);

            AndroidCamera::FpsRange fpsRange;

            fpsRange.min = rangeArray[0];
            fpsRange.max = rangeArray[1];

            env->ReleaseIntArrayElements(jRange, rangeArray, 0);

            rangeList << fpsRange;
        }
    }

    return rangeList;
}

AndroidCamera::FpsRange AndroidCameraPrivate::getPreviewFpsRange()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);
    return getPreviewFpsRange(m_parameters);
}

AndroidCamera::FpsRange AndroidCameraPrivate::getPreviewFpsRange(QJniObject &parameters)
{
    QJniEnvironment env;

    AndroidCamera::FpsRange range;

    if (!parameters.isValid())
        return range;

    jintArray jRangeArray = env->NewIntArray(2);
    parameters.callMethod<void>("getPreviewFpsRange", "([I)V", jRangeArray);

    jint* jRangeElements = env->GetIntArrayElements(jRangeArray, 0);

    // Android Camera API returns values scaled by 1000, so divide here to report
    // normal values for Qt
    range.min = jRangeElements[0] / 1000;
    range.max = jRangeElements[1] / 1000;

    env->ReleaseIntArrayElements(jRangeArray, jRangeElements, 0);
    env->DeleteLocalRef(jRangeArray);

    return range;
}

void AndroidCameraPrivate::setPreviewFpsRange(int min, int max)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    // Android Camera API returns values scaled by 1000, so multiply here to
    // give Android API the scale is expects
    m_parameters.callMethod<void>("setPreviewFpsRange", "(II)V", min * 1000, max * 1000);
}

AndroidCamera::ImageFormat AndroidCameraPrivate::getPreviewFormat()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return AndroidCamera::UnknownImageFormat;

    return AndroidCamera::ImageFormat(m_parameters.callMethod<jint>("getPreviewFormat"));
}

void AndroidCameraPrivate::setPreviewFormat(AndroidCamera::ImageFormat fmt)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setPreviewFormat", "(I)V", jint(fmt));
    applyParameters();
}

QList<AndroidCamera::ImageFormat> AndroidCameraPrivate::getSupportedPreviewFormats()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);
    return getSupportedPreviewFormats(m_parameters);
}

QList<AndroidCamera::ImageFormat> AndroidCameraPrivate::getSupportedPreviewFormats(QJniObject &parameters)
{
    QList<AndroidCamera::ImageFormat> list;

    if (parameters.isValid()) {
        QJniObject formatList = parameters.callObjectMethod("getSupportedPreviewFormats",
                                                              "()Ljava/util/List;");
        int count = formatList.callMethod<jint>("size");
        for (int i = 0; i < count; ++i) {
            QJniObject format = formatList.callObjectMethod("get",
                                                            "(I)Ljava/lang/Object;",
                                                            i);
            list.append(AndroidCamera::ImageFormat(format.callMethod<jint>("intValue")));
        }
    }

    return list;
}

QSize AndroidCameraPrivate::getPreviewSize()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return QSize();

    QJniObject size = m_parameters.callObjectMethod("getPreviewSize",
                                                           "()Landroid/hardware/Camera$Size;");

    if (!size.isValid())
        return QSize();

    return QSize(size.getField<jint>("width"), size.getField<jint>("height"));
}

void AndroidCameraPrivate::updatePreviewSize()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (m_previewSize.isValid()) {
        m_parameters.callMethod<void>("setPreviewSize", "(II)V", m_previewSize.width(), m_previewSize.height());
        applyParameters();
    }

    emit previewSizeChanged();
}

bool AndroidCameraPrivate::setPreviewTexture(void *surfaceTexture)
{
    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_camera.objectClass(), "setPreviewTexture",
                                     "(Landroid/graphics/SurfaceTexture;)V");
    env->CallVoidMethod(m_camera.object(), methodId, static_cast<jobject>(surfaceTexture));

    if (env.checkAndClearExceptions())
        return false;
    return true;
}

bool AndroidCameraPrivate::setPreviewDisplay(void *surfaceHolder)
{
    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_camera.objectClass(), "setPreviewDisplay",
                                     "(Landroid/view/SurfaceHolder;)V");
    env->CallVoidMethod(m_camera.object(), methodId, static_cast<jobject>(surfaceHolder));

    if (env.checkAndClearExceptions())
        return false;
    return true;
}

void AndroidCameraPrivate::setDisplayOrientation(int degrees)
{
    m_camera.callMethod<void>("setDisplayOrientation", "(I)V", degrees);
    m_cameraListener.callMethod<void>("setPhotoRotation", "(I)V", degrees);
}

bool AndroidCameraPrivate::isZoomSupported()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isZoomSupported");
}

int AndroidCameraPrivate::getMaxZoom()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxZoom");
}

QList<int> AndroidCameraPrivate::getZoomRatios()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    QList<int> ratios;

    if (m_parameters.isValid()) {
        QJniObject ratioList = m_parameters.callObjectMethod("getZoomRatios",
                                                                    "()Ljava/util/List;");
        int count = ratioList.callMethod<jint>("size");
        for (int i = 0; i < count; ++i) {
            QJniObject zoomRatio = ratioList.callObjectMethod("get",
                                                                     "(I)Ljava/lang/Object;",
                                                                     i);

            ratios.append(zoomRatio.callMethod<jint>("intValue"));
        }
    }

    return ratios;
}

int AndroidCameraPrivate::getZoom()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getZoom");
}

void AndroidCameraPrivate::setZoom(int value)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setZoom", "(I)V", value);
    applyParameters();
}

QString AndroidCameraPrivate::getFlashMode()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    QString value;

    if (m_parameters.isValid()) {
        QJniObject flashMode = m_parameters.callObjectMethod("getFlashMode",
                                                             "()Ljava/lang/String;");
        if (flashMode.isValid())
            value = flashMode.toString();
    }

    return value;
}

void AndroidCameraPrivate::setFlashMode(const QString &value)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setFlashMode",
                                  "(Ljava/lang/String;)V",
                                  QJniObject::fromString(value).object());
    applyParameters();
}

QString AndroidCameraPrivate::getFocusMode()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    QString value;

    if (m_parameters.isValid()) {
        QJniObject focusMode = m_parameters.callObjectMethod("getFocusMode",
                                                             "()Ljava/lang/String;");
        if (focusMode.isValid())
            value = focusMode.toString();
    }

    return value;
}

void AndroidCameraPrivate::setFocusMode(const QString &value)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setFocusMode",
                                  "(Ljava/lang/String;)V",
                                  QJniObject::fromString(value).object());
    applyParameters();
}

int AndroidCameraPrivate::getMaxNumFocusAreas()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxNumFocusAreas");
}

QList<QRect> AndroidCameraPrivate::getFocusAreas()
{
    QList<QRect> areas;
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (m_parameters.isValid()) {
        QJniObject list = m_parameters.callObjectMethod("getFocusAreas",
                                                        "()Ljava/util/List;");

        if (list.isValid()) {
            int count = list.callMethod<jint>("size");
            for (int i = 0; i < count; ++i) {
                QJniObject area = list.callObjectMethod("get",
                                                        "(I)Ljava/lang/Object;",
                                                        i);

                areas.append(areaToRect(area.object()));
            }
        }
    }

    return areas;
}

void AndroidCameraPrivate::setFocusAreas(const QList<QRect> &areas)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid() || areas.isEmpty())
        return;

    QJniObject list;

    if (!areas.isEmpty()) {
        QJniEnvironment env;
        QJniObject arrayList("java/util/ArrayList", "(I)V", areas.size());
        for (int i = 0; i < areas.size(); ++i) {
            arrayList.callMethod<jboolean>("add",
                                           "(Ljava/lang/Object;)Z",
                                           rectToArea(areas.at(i)).object());
        }
        list = arrayList;
    }

    m_parameters.callMethod<void>("setFocusAreas", "(Ljava/util/List;)V", list.object());

    applyParameters();
}

void AndroidCameraPrivate::autoFocus()
{
    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_camera.objectClass(), "autoFocus",
                                     "(Landroid/hardware/Camera$AutoFocusCallback;)V");
    env->CallVoidMethod(m_camera.object(), methodId, m_cameraListener.object());

    if (!env.checkAndClearExceptions())
        emit autoFocusStarted();
}

void AndroidCameraPrivate::cancelAutoFocus()
{
    QJniEnvironment env;
    m_camera.callMethod<void>("cancelAutoFocus");
}

bool AndroidCameraPrivate::isAutoExposureLockSupported()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isAutoExposureLockSupported");
}

bool AndroidCameraPrivate::getAutoExposureLock()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("getAutoExposureLock");
}

void AndroidCameraPrivate::setAutoExposureLock(bool toggle)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setAutoExposureLock", "(Z)V", toggle);
    applyParameters();
}

bool AndroidCameraPrivate::isAutoWhiteBalanceLockSupported()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isAutoWhiteBalanceLockSupported");
}

bool AndroidCameraPrivate::getAutoWhiteBalanceLock()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("getAutoWhiteBalanceLock");
}

void AndroidCameraPrivate::setAutoWhiteBalanceLock(bool toggle)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setAutoWhiteBalanceLock", "(Z)V", toggle);
    applyParameters();
}

int AndroidCameraPrivate::getExposureCompensation()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getExposureCompensation");
}

void AndroidCameraPrivate::setExposureCompensation(int value)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setExposureCompensation", "(I)V", value);
    applyParameters();
}

float AndroidCameraPrivate::getExposureCompensationStep()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jfloat>("getExposureCompensationStep");
}

int AndroidCameraPrivate::getMinExposureCompensation()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMinExposureCompensation");
}

int AndroidCameraPrivate::getMaxExposureCompensation()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxExposureCompensation");
}

QString AndroidCameraPrivate::getSceneMode()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    QString value;

    if (m_parameters.isValid()) {
        QJniObject sceneMode = m_parameters.callObjectMethod("getSceneMode",
                                                             "()Ljava/lang/String;");
        if (sceneMode.isValid())
            value = sceneMode.toString();
    }

    return value;
}

void AndroidCameraPrivate::setSceneMode(const QString &value)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setSceneMode",
                                  "(Ljava/lang/String;)V",
                                  QJniObject::fromString(value).object());
    applyParameters();
}

QString AndroidCameraPrivate::getWhiteBalance()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    QString value;

    if (m_parameters.isValid()) {
        QJniObject wb = m_parameters.callObjectMethod("getWhiteBalance",
                                                      "()Ljava/lang/String;");
        if (wb.isValid())
            value = wb.toString();
    }

    return value;
}

void AndroidCameraPrivate::setWhiteBalance(const QString &value)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setWhiteBalance",
                                  "(Ljava/lang/String;)V",
                                  QJniObject::fromString(value).object());
    applyParameters();

    emit whiteBalanceChanged();
}

void AndroidCameraPrivate::updateRotation()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    m_parameters.callMethod<void>("setRotation", "(I)V", m_rotation);
    applyParameters();
}

QList<QSize> AndroidCameraPrivate::getSupportedPictureSizes()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    QList<QSize> list;

    if (m_parameters.isValid()) {
        QJniObject sizeList = m_parameters.callObjectMethod("getSupportedPictureSizes",
                                                            "()Ljava/util/List;");
        int count = sizeList.callMethod<jint>("size");
        for (int i = 0; i < count; ++i) {
            QJniObject size = sizeList.callObjectMethod("get",
                                                        "(I)Ljava/lang/Object;",
                                                        i);
            list.append(QSize(size.getField<jint>("width"), size.getField<jint>("height")));
        }

        std::sort(list.begin(), list.end(), qt_sizeLessThan);
    }

    return list;
}

QList<QSize> AndroidCameraPrivate::getSupportedVideoSizes()
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);
    QList<QSize> list;

    if (m_parameters.isValid()) {
        QJniObject sizeList = m_parameters.callObjectMethod("getSupportedVideoSizes",
                                                            "()Ljava/util/List;");
        if (!sizeList.isValid())
            return list;

        int count = sizeList.callMethod<jint>("size");
        for (int i = 0; i < count; ++i) {
            const QJniObject size = sizeList.callObjectMethod("get", "(I)Ljava/lang/Object;", i);
            if (size.isValid())
                list.append(QSize(size.getField<jint>("width"), size.getField<jint>("height")));
        }
        std::sort(list.begin(), list.end(), qt_sizeLessThan);
    }

    return list;
}

void AndroidCameraPrivate::setPictureSize(const QSize &size)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setPictureSize", "(II)V", size.width(), size.height());
    applyParameters();
}

void AndroidCameraPrivate::setJpegQuality(int quality)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setJpegQuality", "(I)V", quality);
    applyParameters();
}

void AndroidCameraPrivate::startPreview()
{
    setupPreviewFrameCallback();

    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_camera.objectClass(), "startPreview", "()V");
    env->CallVoidMethod(m_camera.object(), methodId);

    if (env.checkAndClearExceptions())
        emit previewFailedToStart();
    else
        emit previewStarted();
}

void AndroidCameraPrivate::stopPreview()
{
    // cancel any pending new frame notification
    m_cameraListener.callMethod<void>("notifyWhenFrameAvailable", "(Z)V", false);
    m_camera.callMethod<void>("stopPreview");
    emit previewStopped();
}

void AndroidCameraPrivate::takePicture()
{
    // We must clear the preview callback before calling takePicture(), otherwise the call will
    // block and the camera server will be frozen until the next device restart...
    // That problem only happens on some devices and on the emulator
    m_cameraListener.callMethod<void>("clearPreviewCallback", "(Landroid/hardware/Camera;)V", m_camera.object());

    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_camera.objectClass(), "takePicture",
                                     "(Landroid/hardware/Camera$ShutterCallback;"
                                     "Landroid/hardware/Camera$PictureCallback;"
                                     "Landroid/hardware/Camera$PictureCallback;)V");
    env->CallVoidMethod(m_camera.object(), methodId, m_cameraListener.object(),
                        jobject(0), m_cameraListener.object());

    if (env.checkAndClearExceptions())
        emit takePictureFailed();
}

void AndroidCameraPrivate::setupPreviewFrameCallback()
{
    m_cameraListener.callMethod<void>("setupPreviewCallback", "(Landroid/hardware/Camera;)V", m_camera.object());
}

void AndroidCameraPrivate::notifyNewFrames(bool notify)
{
    m_cameraListener.callMethod<void>("notifyNewFrames", "(Z)V", notify);
}

void AndroidCameraPrivate::fetchLastPreviewFrame()
{
    QJniEnvironment env;
    QJniObject data = m_cameraListener.callObjectMethod("lastPreviewBuffer", "()[B");

    if (!data.isValid()) {
        // If there's no buffer received yet, retry when the next one arrives
        m_cameraListener.callMethod<void>("notifyWhenFrameAvailable", "(Z)V", true);
        return;
    }

    const int arrayLength = env->GetArrayLength(static_cast<jbyteArray>(data.object()));
    if (arrayLength == 0)
        return;

    QByteArray bytes(arrayLength, Qt::Uninitialized);
    env->GetByteArrayRegion(static_cast<jbyteArray>(data.object()),
                            0,
                            arrayLength,
                            reinterpret_cast<jbyte *>(bytes.data()));

    const int width = m_cameraListener.callMethod<jint>("previewWidth");
    const int height = m_cameraListener.callMethod<jint>("previewHeight");
    const int format = m_cameraListener.callMethod<jint>("previewFormat");
    const int bpl = m_cameraListener.callMethod<jint>("previewBytesPerLine");

    QVideoFrameFormat frameFormat(
            QSize(width, height),
            qt_pixelFormatFromAndroidImageFormat(AndroidCamera::ImageFormat(format)));

    QVideoFrame frame = QVideoFramePrivate::createFrame(
            std::make_unique<QMemoryVideoBuffer>(std::move(bytes), bpl), std::move(frameFormat));

    emit lastPreviewFrameFetched(frame);
}

void AndroidCameraPrivate::applyParameters()
{
    QJniEnvironment env;
    m_camera.callMethod<void>("setParameters",
                              "(Landroid/hardware/Camera$Parameters;)V",
                              m_parameters.object());
}

QStringList AndroidCameraPrivate::callParametersStringListMethod(const QByteArray &methodName)
{
    const std::lock_guard<QRecursiveMutex> locker(m_parametersMutex);

    QStringList stringList;

    if (m_parameters.isValid()) {
        QJniObject list = m_parameters.callObjectMethod(methodName.constData(),
                                                        "()Ljava/util/List;");

        if (list.isValid()) {
            int count = list.callMethod<jint>("size");
            for (int i = 0; i < count; ++i) {
                QJniObject string = list.callObjectMethod("get",
                                                          "(I)Ljava/lang/Object;",
                                                          i);
                stringList.append(string.toString());
            }
        }
    }

    return stringList;
}

bool AndroidCamera::registerNativeMethods()
{
    static const JNINativeMethod methods[] = {
        {"notifyAutoFocusComplete", "(IZ)V", (void *)notifyAutoFocusComplete},
        {"notifyPictureExposed", "(I)V", (void *)notifyPictureExposed},
        {"notifyPictureCaptured", "(I[B)V", (void *)notifyPictureCaptured},
        {"notifyNewPreviewFrame", "(I[BIIII)V", (void *)notifyNewPreviewFrame},
        {"notifyFrameAvailable", "(I)V", (void *)notifyFrameAvailable}
    };

    const int size = std::size(methods);
    return QJniEnvironment().registerNativeMethods(QtCameraListenerClassName, methods, size);
}

QT_END_NAMESPACE

#include "androidcamera.moc"
#include "moc_androidcamera_p.cpp"
