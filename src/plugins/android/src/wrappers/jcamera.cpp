/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "jcamera.h"

#include <QtCore/private/qjni_p.h>
#include <qstringlist.h>
#include <qdebug.h>
#include "qandroidmultimediautils.h"
#include <qthread.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

static jclass g_qtCameraClass = 0;
static QMap<int, JCamera*> g_objectMap;
static QMutex g_objectMapMutex;

static QRect areaToRect(jobject areaObj)
{
    QJNIObjectPrivate area(areaObj);
    QJNIObjectPrivate rect = area.getObjectField("rect", "Landroid/graphics/Rect;");

    return QRect(rect.getField<jint>("left"),
                 rect.getField<jint>("top"),
                 rect.callMethod<jint>("width"),
                 rect.callMethod<jint>("height"));
}

static QJNIObjectPrivate rectToArea(const QRect &rect)
{
    QJNIObjectPrivate jrect("android/graphics/Rect",
                     "(IIII)V",
                     rect.left(), rect.top(), rect.right(), rect.bottom());

    QJNIObjectPrivate area("android/hardware/Camera$Area",
                    "(Landroid/graphics/Rect;I)V",
                    jrect.object(), 500);

    return area;
}

// native method for QtCamera.java
static void notifyAutoFocusComplete(JNIEnv* , jobject, int id, jboolean success)
{
    g_objectMapMutex.lock();
    JCamera *obj = g_objectMap.value(id, 0);
    g_objectMapMutex.unlock();
    if (obj)
        Q_EMIT obj->autoFocusComplete(success);
}

static void notifyPictureExposed(JNIEnv* , jobject, int id)
{
    g_objectMapMutex.lock();
    JCamera *obj = g_objectMap.value(id, 0);
    g_objectMapMutex.unlock();
    if (obj)
        Q_EMIT obj->pictureExposed();
}

static void notifyPictureCaptured(JNIEnv *env, jobject, int id, jbyteArray data)
{
    g_objectMapMutex.lock();
    JCamera *obj = g_objectMap.value(id, 0);
    g_objectMapMutex.unlock();
    if (obj) {
        QByteArray bytes;
        int arrayLength = env->GetArrayLength(data);
        bytes.resize(arrayLength);
        env->GetByteArrayRegion(data, 0, arrayLength, (jbyte*)bytes.data());
        Q_EMIT obj->pictureCaptured(bytes);
    }
}

static void notifyFrameFetched(JNIEnv *env, jobject, int id, jbyteArray data)
{
    g_objectMapMutex.lock();
    JCamera *obj = g_objectMap.value(id, 0);
    g_objectMapMutex.unlock();
    if (obj) {
        QByteArray bytes;
        int arrayLength = env->GetArrayLength(data);
        bytes.resize(arrayLength);
        env->GetByteArrayRegion(data, 0, arrayLength, (jbyte*)bytes.data());

        Q_EMIT obj->frameFetched(bytes);
    }
}

class JCameraInstantiator : public QObject
{
    Q_OBJECT
public:
    JCameraInstantiator() : QObject(0) {}
    QJNIObjectPrivate result() {return lastCamera;}
public slots:
    void openCamera(int cameraId)
    {
        QJNIEnvironmentPrivate env;
        lastCamera = QJNIObjectPrivate::callStaticObjectMethod(g_qtCameraClass,
                                                               "open",
                                                               "(I)Lorg/qtproject/qt5/android/multimedia/QtCamera;",
                                                               cameraId);
    }
private:
    QJNIObjectPrivate lastCamera;
};

class JCameraWorker : public QObject, public QJNIObjectPrivate
{
    Q_OBJECT
    friend class JCamera;

    JCameraWorker(JCamera *camera, int cameraId, jobject cam, QThread *workerThread);
    ~JCameraWorker();

    Q_INVOKABLE void release();

    Q_INVOKABLE JCamera::CameraFacing getFacing();
    Q_INVOKABLE int getNativeOrientation();

    Q_INVOKABLE QSize getPreferredPreviewSizeForVideo();
    Q_INVOKABLE QList<QSize> getSupportedPreviewSizes();

    Q_INVOKABLE JCamera::ImageFormat getPreviewFormat();
    Q_INVOKABLE void setPreviewFormat(JCamera::ImageFormat fmt);

    Q_INVOKABLE QSize previewSize() const { return m_previewSize; }
    Q_INVOKABLE void updatePreviewSize();
    Q_INVOKABLE void setPreviewTexture(void *surfaceTexture);

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
    Q_INVOKABLE void setPictureSize(const QSize &size);
    Q_INVOKABLE void setJpegQuality(int quality);

    Q_INVOKABLE void startPreview();
    Q_INVOKABLE void stopPreview();

    Q_INVOKABLE void fetchEachFrame(bool fetch);
    Q_INVOKABLE void fetchLastPreviewFrame();

    Q_INVOKABLE void applyParameters();

    Q_INVOKABLE QStringList callParametersStringListMethod(const QByteArray &methodName);
    Q_INVOKABLE void callVoidMethod(const QByteArray &methodName);

    int m_cameraId;
    QJNIObjectPrivate m_info;
    QJNIObjectPrivate m_parameters;

    QSize m_previewSize;
    int m_rotation;

    bool m_hasAPI14;

    JCamera *q;

    QThread *m_workerThread;
    QMutex m_parametersMutex;

Q_SIGNALS:
    void previewSizeChanged();
    void previewStarted();
    void previewStopped();

    void autoFocusStarted();

    void whiteBalanceChanged();

    void previewFetched(const QByteArray &preview);
};



JCamera::JCamera(int cameraId, jobject cam, QThread *workerThread)
    : QObject()
{
    qRegisterMetaType<QList<int> >();
    qRegisterMetaType<QList<QSize> >();
    qRegisterMetaType<QList<QRect> >();

    d = new JCameraWorker(this, cameraId, cam, workerThread);
    connect(d, &JCameraWorker::previewSizeChanged, this, &JCamera::previewSizeChanged);
    connect(d, &JCameraWorker::previewStarted, this, &JCamera::previewStarted);
    connect(d, &JCameraWorker::previewStopped, this, &JCamera::previewStopped);
    connect(d, &JCameraWorker::autoFocusStarted, this, &JCamera::autoFocusStarted);
    connect(d, &JCameraWorker::whiteBalanceChanged, this, &JCamera::whiteBalanceChanged);
    connect(d, &JCameraWorker::previewFetched, this, &JCamera::previewFetched);
}

JCamera::~JCamera()
{
    if (d->isValid()) {
        g_objectMapMutex.lock();
        g_objectMap.remove(d->m_cameraId);
        g_objectMapMutex.unlock();
    }
    d->deleteLater();
}

JCamera *JCamera::open(int cameraId)
{
    QThread *cameraThread = new QThread;
    connect(cameraThread, &QThread::finished, cameraThread, &QThread::deleteLater);
    cameraThread->start();
    JCameraInstantiator *instantiator = new JCameraInstantiator;
    instantiator->moveToThread(cameraThread);
    QMetaObject::invokeMethod(instantiator, "openCamera",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(int, cameraId));
    QJNIObjectPrivate camera = instantiator->result();
    delete instantiator;

    if (!camera.isValid()) {
        cameraThread->terminate();
        delete cameraThread;
        return 0;
    } else {
        return new JCamera(cameraId, camera.object(), cameraThread);
    }
}

int JCamera::cameraId() const
{
    return d->m_cameraId;
}

void JCamera::lock()
{
    QMetaObject::invokeMethod(d, "callVoidMethod",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(QByteArray, "lock"));
}

void JCamera::unlock()
{
    QMetaObject::invokeMethod(d, "callVoidMethod",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(QByteArray, "unlock"));
}

void JCamera::reconnect()
{
    QMetaObject::invokeMethod(d, "callVoidMethod", Q_ARG(QByteArray, "reconnect"));
}

void JCamera::release()
{
    QMetaObject::invokeMethod(d, "release");
}

JCamera::CameraFacing JCamera::getFacing()
{
    return d->getFacing();
}

int JCamera::getNativeOrientation()
{
    return d->getNativeOrientation();
}

QSize JCamera::getPreferredPreviewSizeForVideo()
{
    return d->getPreferredPreviewSizeForVideo();
}

QList<QSize> JCamera::getSupportedPreviewSizes()
{
    return d->getSupportedPreviewSizes();
}

JCamera::ImageFormat JCamera::getPreviewFormat()
{
    return d->getPreviewFormat();
}

void JCamera::setPreviewFormat(ImageFormat fmt)
{
    QMetaObject::invokeMethod(d, "setPreviewFormat", Q_ARG(JCamera::ImageFormat, fmt));
}

QSize JCamera::previewSize() const
{
    return d->m_previewSize;
}

void JCamera::setPreviewSize(const QSize &size)
{
    d->m_parametersMutex.lock();
    bool areParametersValid = d->m_parameters.isValid();
    d->m_parametersMutex.unlock();
    if (!areParametersValid)
        return;

    d->m_previewSize = size;
    QMetaObject::invokeMethod(d, "updatePreviewSize");
}

void JCamera::setPreviewTexture(jobject surfaceTexture)
{
    QMetaObject::invokeMethod(d, "setPreviewTexture", Q_ARG(void *, surfaceTexture));
}

bool JCamera::isZoomSupported()
{
    return d->isZoomSupported();
}

int JCamera::getMaxZoom()
{
    return d->getMaxZoom();
}

QList<int> JCamera::getZoomRatios()
{
    return d->getZoomRatios();
}

int JCamera::getZoom()
{
    return d->getZoom();
}

void JCamera::setZoom(int value)
{
    QMetaObject::invokeMethod(d, "setZoom", Q_ARG(int, value));
}

QStringList JCamera::getSupportedFlashModes()
{
    return d->callParametersStringListMethod("getSupportedFlashModes");
}

QString JCamera::getFlashMode()
{
    return d->getFlashMode();
}

void JCamera::setFlashMode(const QString &value)
{
    QMetaObject::invokeMethod(d, "setFlashMode", Q_ARG(QString, value));
}

QStringList JCamera::getSupportedFocusModes()
{
    return d->callParametersStringListMethod("getSupportedFocusModes");
}

QString JCamera::getFocusMode()
{
    return d->getFocusMode();
}

void JCamera::setFocusMode(const QString &value)
{
    QMetaObject::invokeMethod(d, "setFocusMode", Q_ARG(QString, value));
}

int JCamera::getMaxNumFocusAreas()
{
    return d->getMaxNumFocusAreas();
}

QList<QRect> JCamera::getFocusAreas()
{
    return d->getFocusAreas();
}

void JCamera::setFocusAreas(const QList<QRect> &areas)
{
    QMetaObject::invokeMethod(d, "setFocusAreas", Q_ARG(QList<QRect>, areas));
}

void JCamera::autoFocus()
{
    QMetaObject::invokeMethod(d, "autoFocus");
}

void JCamera::cancelAutoFocus()
{
    QMetaObject::invokeMethod(d, "callVoidMethod", Q_ARG(QByteArray, "cancelAutoFocus"));
}

bool JCamera::isAutoExposureLockSupported()
{
    return d->isAutoExposureLockSupported();
}

bool JCamera::getAutoExposureLock()
{
    return d->getAutoExposureLock();
}

void JCamera::setAutoExposureLock(bool toggle)
{
    QMetaObject::invokeMethod(d, "setAutoExposureLock", Q_ARG(bool, toggle));
}

bool JCamera::isAutoWhiteBalanceLockSupported()
{
    return d->isAutoWhiteBalanceLockSupported();
}

bool JCamera::getAutoWhiteBalanceLock()
{
    return d->getAutoWhiteBalanceLock();
}

void JCamera::setAutoWhiteBalanceLock(bool toggle)
{
    QMetaObject::invokeMethod(d, "setAutoWhiteBalanceLock", Q_ARG(bool, toggle));
}

int JCamera::getExposureCompensation()
{
    return d->getExposureCompensation();
}

void JCamera::setExposureCompensation(int value)
{
    QMetaObject::invokeMethod(d, "setExposureCompensation", Q_ARG(int, value));
}

float JCamera::getExposureCompensationStep()
{
    return d->getExposureCompensationStep();
}

int JCamera::getMinExposureCompensation()
{
    return d->getMinExposureCompensation();
}

int JCamera::getMaxExposureCompensation()
{
    return d->getMaxExposureCompensation();
}

QStringList JCamera::getSupportedSceneModes()
{
    return d->callParametersStringListMethod("getSupportedSceneModes");
}

QString JCamera::getSceneMode()
{
    return d->getSceneMode();
}

void JCamera::setSceneMode(const QString &value)
{
    QMetaObject::invokeMethod(d, "setSceneMode", Q_ARG(QString, value));
}

QStringList JCamera::getSupportedWhiteBalance()
{
    return d->callParametersStringListMethod("getSupportedWhiteBalance");
}

QString JCamera::getWhiteBalance()
{
    return d->getWhiteBalance();
}

void JCamera::setWhiteBalance(const QString &value)
{
    QMetaObject::invokeMethod(d, "setWhiteBalance", Q_ARG(QString, value));
}

void JCamera::setRotation(int rotation)
{
    //We need to do it here and not in worker class because we cache rotation
    d->m_parametersMutex.lock();
    bool areParametersValid = d->m_parameters.isValid();
    d->m_parametersMutex.unlock();
    if (!areParametersValid)
        return;

    d->m_rotation = rotation;
    QMetaObject::invokeMethod(d, "updateRotation");
}

int JCamera::getRotation() const
{
    return d->m_rotation;
}

QList<QSize> JCamera::getSupportedPictureSizes()
{
    return d->getSupportedPictureSizes();
}

void JCamera::setPictureSize(const QSize &size)
{
    QMetaObject::invokeMethod(d, "setPictureSize", Q_ARG(QSize, size));
}

void JCamera::setJpegQuality(int quality)
{
    QMetaObject::invokeMethod(d, "setJpegQuality", Q_ARG(int, quality));
}

void JCamera::takePicture()
{
    QMetaObject::invokeMethod(d, "callVoidMethod", Q_ARG(QByteArray, "takePicture"));
}

void JCamera::fetchEachFrame(bool fetch)
{
    QMetaObject::invokeMethod(d, "fetchEachFrame", Q_ARG(bool, fetch));
}

void JCamera::fetchLastPreviewFrame()
{
    QMetaObject::invokeMethod(d, "fetchLastPreviewFrame");
}

QJNIObjectPrivate JCamera::getCameraObject()
{
    return d->getObjectField("m_camera", "Landroid/hardware/Camera;");
}

void JCamera::startPreview()
{
    QMetaObject::invokeMethod(d, "startPreview");
}

void JCamera::stopPreview()
{
    QMetaObject::invokeMethod(d, "stopPreview");
}


//JCameraWorker

JCameraWorker::JCameraWorker(JCamera *camera, int cameraId, jobject cam, QThread *workerThread)
    : QObject(0)
    , QJNIObjectPrivate(cam)
    , m_cameraId(cameraId)
    , m_rotation(0)
    , m_hasAPI14(false)
    , m_parametersMutex(QMutex::Recursive)
{
    q = camera;
    m_workerThread = workerThread;
    moveToThread(m_workerThread);

    if (isValid()) {
        g_objectMapMutex.lock();
        g_objectMap.insert(cameraId, q);
        g_objectMapMutex.unlock();

        m_info = QJNIObjectPrivate("android/hardware/Camera$CameraInfo");
        callStaticMethod<void>("android/hardware/Camera",
                               "getCameraInfo",
                               "(ILandroid/hardware/Camera$CameraInfo;)V",
                               cameraId, m_info.object());

        QJNIObjectPrivate params = callObjectMethod("getParameters",
                                                    "()Landroid/hardware/Camera$Parameters;");
        m_parameters = QJNIObjectPrivate(params);

        // Check if API 14 is available
        QJNIEnvironmentPrivate env;
        jclass clazz = env->FindClass("android/hardware/Camera");
        if (env->ExceptionCheck()) {
            clazz = 0;
            env->ExceptionClear();
        }
        if (clazz) {
            // startFaceDetection() was added in API 14
            jmethodID id = env->GetMethodID(clazz, "startFaceDetection", "()V");
            if (env->ExceptionCheck()) {
                id = 0;
                env->ExceptionClear();
            }
            m_hasAPI14 = bool(id);
        }
    }
}

JCameraWorker::~JCameraWorker()
{
    m_workerThread->quit();
}

void JCameraWorker::release()
{
    m_previewSize = QSize();
    m_parametersMutex.lock();
    m_parameters = QJNIObjectPrivate();
    m_parametersMutex.unlock();
    callMethod<void>("release");
}

JCamera::CameraFacing JCameraWorker::getFacing()
{
    return JCamera::CameraFacing(m_info.getField<jint>("facing"));
}

int JCameraWorker::getNativeOrientation()
{
    return m_info.getField<jint>("orientation");
}

QSize JCameraWorker::getPreferredPreviewSizeForVideo()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return QSize();

    QJNIObjectPrivate size = m_parameters.callObjectMethod("getPreferredPreviewSizeForVideo",
                                                           "()Landroid/hardware/Camera$Size;");

    return QSize(size.getField<jint>("width"), size.getField<jint>("height"));
}

QList<QSize> JCameraWorker::getSupportedPreviewSizes()
{
    QList<QSize> list;

    QMutexLocker parametersLocker(&m_parametersMutex);

    if (m_parameters.isValid()) {
        QJNIObjectPrivate sizeList = m_parameters.callObjectMethod("getSupportedPreviewSizes",
                                                                   "()Ljava/util/List;");
        int count = sizeList.callMethod<jint>("size");
        for (int i = 0; i < count; ++i) {
            QJNIObjectPrivate size = sizeList.callObjectMethod("get",
                                                               "(I)Ljava/lang/Object;",
                                                               i);
            list.append(QSize(size.getField<jint>("width"), size.getField<jint>("height")));
        }

        qSort(list.begin(), list.end(), qt_sizeLessThan);
    }

    return list;
}

JCamera::ImageFormat JCameraWorker::getPreviewFormat()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return JCamera::Unknown;

    return JCamera::ImageFormat(m_parameters.callMethod<jint>("getPreviewFormat"));
}

void JCameraWorker::setPreviewFormat(JCamera::ImageFormat fmt)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setPreviewFormat", "(I)V", jint(fmt));
    applyParameters();
}

void JCameraWorker::updatePreviewSize()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (m_previewSize.isValid()) {
        m_parameters.callMethod<void>("setPreviewSize", "(II)V", m_previewSize.width(), m_previewSize.height());
        applyParameters();
    }

    emit previewSizeChanged();
}

void JCameraWorker::setPreviewTexture(void *surfaceTexture)
{
    callMethod<void>("setPreviewTexture", "(Landroid/graphics/SurfaceTexture;)V", static_cast<jobject>(surfaceTexture));
}

bool JCameraWorker::isZoomSupported()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isZoomSupported");
}

int JCameraWorker::getMaxZoom()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxZoom");
}

QList<int> JCameraWorker::getZoomRatios()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    QList<int> ratios;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate ratioList = m_parameters.callObjectMethod("getZoomRatios",
                                                                    "()Ljava/util/List;");
        int count = ratioList.callMethod<jint>("size");
        for (int i = 0; i < count; ++i) {
            QJNIObjectPrivate zoomRatio = ratioList.callObjectMethod("get",
                                                                     "(I)Ljava/lang/Object;",
                                                                     i);

            ratios.append(zoomRatio.callMethod<jint>("intValue"));
        }
    }

    return ratios;
}

int JCameraWorker::getZoom()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getZoom");
}

void JCameraWorker::setZoom(int value)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setZoom", "(I)V", value);
    applyParameters();
}

QString JCameraWorker::getFlashMode()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    QString value;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate flashMode = m_parameters.callObjectMethod("getFlashMode",
                                                                    "()Ljava/lang/String;");
        if (flashMode.isValid())
            value = flashMode.toString();
    }

    return value;
}

void JCameraWorker::setFlashMode(const QString &value)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setFlashMode",
                                  "(Ljava/lang/String;)V",
                                  QJNIObjectPrivate::fromString(value).object());
    applyParameters();
}

QString JCameraWorker::getFocusMode()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    QString value;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate focusMode = m_parameters.callObjectMethod("getFocusMode",
                                                                    "()Ljava/lang/String;");
        if (focusMode.isValid())
            value = focusMode.toString();
    }

    return value;
}

void JCameraWorker::setFocusMode(const QString &value)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setFocusMode",
                                  "(Ljava/lang/String;)V",
                                  QJNIObjectPrivate::fromString(value).object());
    applyParameters();
}

int JCameraWorker::getMaxNumFocusAreas()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_hasAPI14 || !m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxNumFocusAreas");
}

QList<QRect> JCameraWorker::getFocusAreas()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    QList<QRect> areas;

    if (m_hasAPI14 && m_parameters.isValid()) {
        QJNIObjectPrivate list = m_parameters.callObjectMethod("getFocusAreas",
                                                               "()Ljava/util/List;");

        if (list.isValid()) {
            int count = list.callMethod<jint>("size");
            for (int i = 0; i < count; ++i) {
                QJNIObjectPrivate area = list.callObjectMethod("get",
                                                               "(I)Ljava/lang/Object;",
                                                               i);

                areas.append(areaToRect(area.object()));
            }
        }
    }

    return areas;
}

void JCameraWorker::setFocusAreas(const QList<QRect> &areas)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_hasAPI14 || !m_parameters.isValid())
        return;

    QJNIObjectPrivate list;

    if (!areas.isEmpty()) {
        QJNIEnvironmentPrivate env;
        QJNIObjectPrivate arrayList("java/util/ArrayList", "(I)V", areas.size());
        for (int i = 0; i < areas.size(); ++i) {
            arrayList.callMethod<jboolean>("add",
                                           "(Ljava/lang/Object;)Z",
                                           rectToArea(areas.at(i)).object());
            if (env->ExceptionCheck())
                env->ExceptionClear();
        }
        list = arrayList;
    }

    m_parameters.callMethod<void>("setFocusAreas", "(Ljava/util/List;)V", list.object());

    applyParameters();
}

void JCameraWorker::autoFocus()
{
    callMethod<void>("autoFocus");
    emit autoFocusStarted();
}

bool JCameraWorker::isAutoExposureLockSupported()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_hasAPI14 || !m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isAutoExposureLockSupported");
}

bool JCameraWorker::getAutoExposureLock()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_hasAPI14 || !m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("getAutoExposureLock");
}

void JCameraWorker::setAutoExposureLock(bool toggle)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_hasAPI14 || !m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setAutoExposureLock", "(Z)V", toggle);
    applyParameters();
}

bool JCameraWorker::isAutoWhiteBalanceLockSupported()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_hasAPI14 || !m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isAutoWhiteBalanceLockSupported");
}

bool JCameraWorker::getAutoWhiteBalanceLock()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_hasAPI14 || !m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("getAutoWhiteBalanceLock");
}

void JCameraWorker::setAutoWhiteBalanceLock(bool toggle)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_hasAPI14 || !m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setAutoWhiteBalanceLock", "(Z)V", toggle);
    applyParameters();
}

int JCameraWorker::getExposureCompensation()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getExposureCompensation");
}

void JCameraWorker::setExposureCompensation(int value)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setExposureCompensation", "(I)V", value);
    applyParameters();
}

float JCameraWorker::getExposureCompensationStep()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jfloat>("getExposureCompensationStep");
}

int JCameraWorker::getMinExposureCompensation()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMinExposureCompensation");
}

int JCameraWorker::getMaxExposureCompensation()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxExposureCompensation");
}

QString JCameraWorker::getSceneMode()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    QString value;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate sceneMode = m_parameters.callObjectMethod("getSceneMode",
                                                                    "()Ljava/lang/String;");
        if (sceneMode.isValid())
            value = sceneMode.toString();
    }

    return value;
}

void JCameraWorker::setSceneMode(const QString &value)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setSceneMode",
                                  "(Ljava/lang/String;)V",
                                  QJNIObjectPrivate::fromString(value).object());
    applyParameters();
}

QString JCameraWorker::getWhiteBalance()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    QString value;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate wb = m_parameters.callObjectMethod("getWhiteBalance",
                                                             "()Ljava/lang/String;");
        if (wb.isValid())
            value = wb.toString();
    }

    return value;
}

void JCameraWorker::setWhiteBalance(const QString &value)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setWhiteBalance",
                                  "(Ljava/lang/String;)V",
                                  QJNIObjectPrivate::fromString(value).object());
    applyParameters();

    emit whiteBalanceChanged();
}

void JCameraWorker::updateRotation()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    m_parameters.callMethod<void>("setRotation", "(I)V", m_rotation);
    applyParameters();
}

QList<QSize> JCameraWorker::getSupportedPictureSizes()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    QList<QSize> list;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate sizeList = m_parameters.callObjectMethod("getSupportedPictureSizes",
                                                                   "()Ljava/util/List;");
        int count = sizeList.callMethod<jint>("size");
        for (int i = 0; i < count; ++i) {
            QJNIObjectPrivate size = sizeList.callObjectMethod("get",
                                                               "(I)Ljava/lang/Object;",
                                                               i);
            list.append(QSize(size.getField<jint>("width"), size.getField<jint>("height")));
        }

        qSort(list.begin(), list.end(), qt_sizeLessThan);
    }

    return list;
}

void JCameraWorker::setPictureSize(const QSize &size)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setPictureSize", "(II)V", size.width(), size.height());
    applyParameters();
}

void JCameraWorker::setJpegQuality(int quality)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setJpegQuality", "(I)V", quality);
    applyParameters();
}

void JCameraWorker::startPreview()
{
    callVoidMethod("startPreview");
    emit previewStarted();
}

void JCameraWorker::stopPreview()
{
    callVoidMethod("stopPreview");
    emit previewStopped();
}

void JCameraWorker::fetchEachFrame(bool fetch)
{
    callMethod<void>("fetchEachFrame", "(Z)V", fetch);
}

void JCameraWorker::fetchLastPreviewFrame()
{
    QJNIEnvironmentPrivate env;
    QJNIObjectPrivate dataObj = callObjectMethod("lockAndFetchPreviewBuffer", "()[B");
    if (!dataObj.object()) {
        callMethod<void>("unlockPreviewBuffer");
        return;
    }
    jbyteArray data = static_cast<jbyteArray>(dataObj.object());
    QByteArray bytes;
    int arrayLength = env->GetArrayLength(data);
    bytes.resize(arrayLength);
    env->GetByteArrayRegion(data, 0, arrayLength, (jbyte*)bytes.data());
    callMethod<void>("unlockPreviewBuffer");

    emit previewFetched(bytes);
}

void JCameraWorker::applyParameters()
{
    callMethod<void>("setParameters",
                     "(Landroid/hardware/Camera$Parameters;)V",
                     m_parameters.object());
}

QStringList JCameraWorker::callParametersStringListMethod(const QByteArray &methodName)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    QStringList stringList;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate list = m_parameters.callObjectMethod(methodName.constData(),
                                                               "()Ljava/util/List;");

        if (list.isValid()) {
            int count = list.callMethod<jint>("size");
            for (int i = 0; i < count; ++i) {
                QJNIObjectPrivate string = list.callObjectMethod("get",
                                                                 "(I)Ljava/lang/Object;",
                                                                 i);

                stringList.append(string.toString());
            }
        }
    }

    return stringList;
}

void JCameraWorker::callVoidMethod(const QByteArray &methodName)
{
    callMethod<void>(methodName.constData());
}


static JNINativeMethod methods[] = {
    {"notifyAutoFocusComplete", "(IZ)V", (void *)notifyAutoFocusComplete},
    {"notifyPictureExposed", "(I)V", (void *)notifyPictureExposed},
    {"notifyPictureCaptured", "(I[B)V", (void *)notifyPictureCaptured},
    {"notifyFrameFetched", "(I[B)V", (void *)notifyFrameFetched}
};

bool JCamera::initJNI(JNIEnv *env)
{
    jclass clazz = env->FindClass("org/qtproject/qt5/android/multimedia/QtCamera");
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (clazz) {
        g_qtCameraClass = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (env->RegisterNatives(g_qtCameraClass,
                                 methods,
                                 sizeof(methods) / sizeof(methods[0])) < 0) {
            return false;
        }
    }

    return true;
}

QT_END_NAMESPACE

#include "jcamera.moc"
