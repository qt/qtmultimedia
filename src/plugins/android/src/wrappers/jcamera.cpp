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

#include <qstringlist.h>
#include <qdebug.h>
#include "qandroidmultimediautils.h"
#include <qmutex.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

static jclass g_qtCameraListenerClass = 0;
static QMutex g_cameraMapMutex;
typedef QMap<int, JCamera *> CameraMap;
Q_GLOBAL_STATIC(CameraMap, g_cameraMap)

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

// native method for QtCameraLisener.java
static void notifyAutoFocusComplete(JNIEnv* , jobject, int id, jboolean success)
{
    QMutexLocker locker(&g_cameraMapMutex);
    JCamera *obj = g_cameraMap->value(id, 0);
    if (obj)
        Q_EMIT obj->autoFocusComplete(success);
}

static void notifyPictureExposed(JNIEnv* , jobject, int id)
{
    QMutexLocker locker(&g_cameraMapMutex);
    JCamera *obj = g_cameraMap->value(id, 0);
    if (obj)
        Q_EMIT obj->pictureExposed();
}

static void notifyPictureCaptured(JNIEnv *env, jobject, int id, jbyteArray data)
{
    QMutexLocker locker(&g_cameraMapMutex);
    JCamera *obj = g_cameraMap->value(id, 0);
    if (obj) {
        const int arrayLength = env->GetArrayLength(data);
        QByteArray bytes(arrayLength, Qt::Uninitialized);
        env->GetByteArrayRegion(data, 0, arrayLength, (jbyte*)bytes.data());
        Q_EMIT obj->pictureCaptured(bytes);
    }
}

static void notifyFrameFetched(JNIEnv *env, jobject, int id, jbyteArray data)
{
    QMutexLocker locker(&g_cameraMapMutex);
    JCamera *obj = g_cameraMap->value(id, 0);
    if (obj) {
        const int arrayLength = env->GetArrayLength(data);
        QByteArray bytes(arrayLength, Qt::Uninitialized);
        env->GetByteArrayRegion(data, 0, arrayLength, (jbyte*)bytes.data());

        Q_EMIT obj->frameFetched(bytes);
    }
}

class JCameraPrivate : public QObject
{
    Q_OBJECT
public:
    JCameraPrivate();
    ~JCameraPrivate();

    Q_INVOKABLE bool init(int cameraId);

    Q_INVOKABLE void release();
    Q_INVOKABLE void lock();
    Q_INVOKABLE void unlock();
    Q_INVOKABLE void reconnect();

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
    Q_INVOKABLE void setPictureSize(const QSize &size);
    Q_INVOKABLE void setJpegQuality(int quality);

    Q_INVOKABLE void startPreview();
    Q_INVOKABLE void stopPreview();

    Q_INVOKABLE void takePicture();

    Q_INVOKABLE void fetchEachFrame(bool fetch);
    Q_INVOKABLE void fetchLastPreviewFrame();

    Q_INVOKABLE void applyParameters();

    Q_INVOKABLE QStringList callParametersStringListMethod(const QByteArray &methodName);

    int m_cameraId;
    QMutex m_parametersMutex;
    QSize m_previewSize;
    int m_rotation;
    QJNIObjectPrivate m_info;
    QJNIObjectPrivate m_parameters;
    QJNIObjectPrivate m_camera;
    QJNIObjectPrivate m_cameraListener;

Q_SIGNALS:
    void previewSizeChanged();
    void previewStarted();
    void previewStopped();

    void autoFocusStarted();

    void whiteBalanceChanged();

    void previewFetched(const QByteArray &preview);
};

JCamera::JCamera(JCameraPrivate *d, QThread *worker)
    : QObject(),
      d_ptr(d),
      m_worker(worker)

{
    qRegisterMetaType<QList<int> >();
    qRegisterMetaType<QList<QSize> >();
    qRegisterMetaType<QList<QRect> >();

    connect(d, &JCameraPrivate::previewSizeChanged, this, &JCamera::previewSizeChanged);
    connect(d, &JCameraPrivate::previewStarted, this, &JCamera::previewStarted);
    connect(d, &JCameraPrivate::previewStopped, this, &JCamera::previewStopped);
    connect(d, &JCameraPrivate::autoFocusStarted, this, &JCamera::autoFocusStarted);
    connect(d, &JCameraPrivate::whiteBalanceChanged, this, &JCamera::whiteBalanceChanged);
    connect(d, &JCameraPrivate::previewFetched, this, &JCamera::previewFetched);
}

JCamera::~JCamera()
{
    Q_D(JCamera);
    if (d->m_camera.isValid()) {
        g_cameraMapMutex.lock();
        g_cameraMap->remove(d->m_cameraId);
        g_cameraMapMutex.unlock();
    }

    release();
    m_worker->exit();
    m_worker->wait(5000);
}

JCamera *JCamera::open(int cameraId)
{
    JCameraPrivate *d = new JCameraPrivate();
    QThread *worker = new QThread;
    worker->start();
    d->moveToThread(worker);
    connect(worker, &QThread::finished, d, &JCameraPrivate::deleteLater);
    bool ok = false;
    QMetaObject::invokeMethod(d, "init", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, ok), Q_ARG(int, cameraId));
    if (!ok) {
        worker->quit();
        worker->wait(5000);
        delete d;
        delete worker;
        return 0;
    }

    JCamera *q = new JCamera(d, worker);
    g_cameraMapMutex.lock();
    g_cameraMap->insert(cameraId, q);
    g_cameraMapMutex.unlock();
    return q;
}

int JCamera::cameraId() const
{
    Q_D(const JCamera);
    return d->m_cameraId;
}

void JCamera::lock()
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "lock", Qt::BlockingQueuedConnection);
}

void JCamera::unlock()
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "unlock", Qt::BlockingQueuedConnection);
}

void JCamera::reconnect()
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "reconnect");
}

void JCamera::release()
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "release", Qt::BlockingQueuedConnection);
}

JCamera::CameraFacing JCamera::getFacing()
{
    Q_D(JCamera);
    return d->getFacing();
}

int JCamera::getNativeOrientation()
{
    Q_D(JCamera);
    return d->getNativeOrientation();
}

QSize JCamera::getPreferredPreviewSizeForVideo()
{
    Q_D(JCamera);
    return d->getPreferredPreviewSizeForVideo();
}

QList<QSize> JCamera::getSupportedPreviewSizes()
{
    Q_D(JCamera);
    return d->getSupportedPreviewSizes();
}

JCamera::ImageFormat JCamera::getPreviewFormat()
{
    Q_D(JCamera);
    return d->getPreviewFormat();
}

void JCamera::setPreviewFormat(ImageFormat fmt)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setPreviewFormat", Q_ARG(JCamera::ImageFormat, fmt));
}

QSize JCamera::previewSize() const
{
    Q_D(const JCamera);
    return d->m_previewSize;
}

void JCamera::setPreviewSize(const QSize &size)
{
    Q_D(JCamera);
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
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setPreviewTexture", Qt::BlockingQueuedConnection, Q_ARG(void *, surfaceTexture));
}

bool JCamera::isZoomSupported()
{
    Q_D(JCamera);
    return d->isZoomSupported();
}

int JCamera::getMaxZoom()
{
    Q_D(JCamera);
    return d->getMaxZoom();
}

QList<int> JCamera::getZoomRatios()
{
    Q_D(JCamera);
    return d->getZoomRatios();
}

int JCamera::getZoom()
{
    Q_D(JCamera);
    return d->getZoom();
}

void JCamera::setZoom(int value)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setZoom", Q_ARG(int, value));
}

QStringList JCamera::getSupportedFlashModes()
{
    Q_D(JCamera);
    return d->callParametersStringListMethod("getSupportedFlashModes");
}

QString JCamera::getFlashMode()
{
    Q_D(JCamera);
    return d->getFlashMode();
}

void JCamera::setFlashMode(const QString &value)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setFlashMode", Q_ARG(QString, value));
}

QStringList JCamera::getSupportedFocusModes()
{
    Q_D(JCamera);
    return d->callParametersStringListMethod("getSupportedFocusModes");
}

QString JCamera::getFocusMode()
{
    Q_D(JCamera);
    return d->getFocusMode();
}

void JCamera::setFocusMode(const QString &value)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setFocusMode", Q_ARG(QString, value));
}

int JCamera::getMaxNumFocusAreas()
{
    Q_D(JCamera);
    return d->getMaxNumFocusAreas();
}

QList<QRect> JCamera::getFocusAreas()
{
    Q_D(JCamera);
    return d->getFocusAreas();
}

void JCamera::setFocusAreas(const QList<QRect> &areas)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setFocusAreas", Q_ARG(QList<QRect>, areas));
}

void JCamera::autoFocus()
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "autoFocus");
}

void JCamera::cancelAutoFocus()
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "cancelAutoFocus", Qt::QueuedConnection);
}

bool JCamera::isAutoExposureLockSupported()
{
    Q_D(JCamera);
    return d->isAutoExposureLockSupported();
}

bool JCamera::getAutoExposureLock()
{
    Q_D(JCamera);
    return d->getAutoExposureLock();
}

void JCamera::setAutoExposureLock(bool toggle)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setAutoExposureLock", Q_ARG(bool, toggle));
}

bool JCamera::isAutoWhiteBalanceLockSupported()
{
    Q_D(JCamera);
    return d->isAutoWhiteBalanceLockSupported();
}

bool JCamera::getAutoWhiteBalanceLock()
{
    Q_D(JCamera);
    return d->getAutoWhiteBalanceLock();
}

void JCamera::setAutoWhiteBalanceLock(bool toggle)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setAutoWhiteBalanceLock", Q_ARG(bool, toggle));
}

int JCamera::getExposureCompensation()
{
    Q_D(JCamera);
    return d->getExposureCompensation();
}

void JCamera::setExposureCompensation(int value)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setExposureCompensation", Q_ARG(int, value));
}

float JCamera::getExposureCompensationStep()
{
    Q_D(JCamera);
    return d->getExposureCompensationStep();
}

int JCamera::getMinExposureCompensation()
{
    Q_D(JCamera);
    return d->getMinExposureCompensation();
}

int JCamera::getMaxExposureCompensation()
{
    Q_D(JCamera);
    return d->getMaxExposureCompensation();
}

QStringList JCamera::getSupportedSceneModes()
{
    Q_D(JCamera);
    return d->callParametersStringListMethod("getSupportedSceneModes");
}

QString JCamera::getSceneMode()
{
    Q_D(JCamera);
    return d->getSceneMode();
}

void JCamera::setSceneMode(const QString &value)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setSceneMode", Q_ARG(QString, value));
}

QStringList JCamera::getSupportedWhiteBalance()
{
    Q_D(JCamera);
    return d->callParametersStringListMethod("getSupportedWhiteBalance");
}

QString JCamera::getWhiteBalance()
{
    Q_D(JCamera);
    return d->getWhiteBalance();
}

void JCamera::setWhiteBalance(const QString &value)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setWhiteBalance", Q_ARG(QString, value));
}

void JCamera::setRotation(int rotation)
{
    Q_D(JCamera);
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
    Q_D(const JCamera);
    return d->m_rotation;
}

QList<QSize> JCamera::getSupportedPictureSizes()
{
    Q_D(JCamera);
    return d->getSupportedPictureSizes();
}

void JCamera::setPictureSize(const QSize &size)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setPictureSize", Q_ARG(QSize, size));
}

void JCamera::setJpegQuality(int quality)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "setJpegQuality", Q_ARG(int, quality));
}

void JCamera::takePicture()
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "takePicture", Qt::BlockingQueuedConnection);
}

void JCamera::fetchEachFrame(bool fetch)
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "fetchEachFrame", Q_ARG(bool, fetch));
}

void JCamera::fetchLastPreviewFrame()
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "fetchLastPreviewFrame");
}

QJNIObjectPrivate JCamera::getCameraObject()
{
    Q_D(JCamera);
    return d->m_camera;
}

void JCamera::startPreview()
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "startPreview");
}

void JCamera::stopPreview()
{
    Q_D(JCamera);
    QMetaObject::invokeMethod(d, "stopPreview");
}

JCameraPrivate::JCameraPrivate()
    : QObject(),
      m_parametersMutex(QMutex::Recursive)
{
}

JCameraPrivate::~JCameraPrivate()
{
}

bool JCameraPrivate::init(int cameraId)
{
    m_cameraId = cameraId;
    m_camera = QJNIObjectPrivate::callStaticObjectMethod("android/hardware/Camera",
                                                         "open",
                                                         "(I)Landroid/hardware/Camera;",
                                                         cameraId);

    if (!m_camera.isValid())
        return false;

    m_cameraListener = QJNIObjectPrivate(g_qtCameraListenerClass, "(I)V", m_cameraId);
    m_info = QJNIObjectPrivate("android/hardware/Camera$CameraInfo");
    m_camera.callStaticMethod<void>("android/hardware/Camera",
                                    "getCameraInfo",
                                    "(ILandroid/hardware/Camera$CameraInfo;)V",
                                    cameraId,
                                    m_info.object());

    QJNIObjectPrivate params = m_camera.callObjectMethod("getParameters",
                                                         "()Landroid/hardware/Camera$Parameters;");
    m_parameters = QJNIObjectPrivate(params);

    return true;
}

void JCameraPrivate::release()
{
    m_previewSize = QSize();
    m_parametersMutex.lock();
    m_parameters = QJNIObjectPrivate();
    m_parametersMutex.unlock();
    if (m_camera.isValid())
        m_camera.callMethod<void>("release");
}

void JCameraPrivate::lock()
{
    m_camera.callMethod<void>("lock");
}

void JCameraPrivate::unlock()
{
    m_camera.callMethod<void>("unlock");
}

void JCameraPrivate::reconnect()
{
    QJNIEnvironmentPrivate env;
    m_camera.callMethod<void>("reconnect");
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif // QT_DEBUG
        env->ExceptionDescribe();
    }
}

JCamera::CameraFacing JCameraPrivate::getFacing()
{
    return JCamera::CameraFacing(m_info.getField<jint>("facing"));
}

int JCameraPrivate::getNativeOrientation()
{
    return m_info.getField<jint>("orientation");
}

QSize JCameraPrivate::getPreferredPreviewSizeForVideo()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return QSize();

    QJNIObjectPrivate size = m_parameters.callObjectMethod("getPreferredPreviewSizeForVideo",
                                                           "()Landroid/hardware/Camera$Size;");

    return QSize(size.getField<jint>("width"), size.getField<jint>("height"));
}

QList<QSize> JCameraPrivate::getSupportedPreviewSizes()
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

JCamera::ImageFormat JCameraPrivate::getPreviewFormat()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return JCamera::Unknown;

    return JCamera::ImageFormat(m_parameters.callMethod<jint>("getPreviewFormat"));
}

void JCameraPrivate::setPreviewFormat(JCamera::ImageFormat fmt)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setPreviewFormat", "(I)V", jint(fmt));
    applyParameters();
}

void JCameraPrivate::updatePreviewSize()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (m_previewSize.isValid()) {
        m_parameters.callMethod<void>("setPreviewSize", "(II)V", m_previewSize.width(), m_previewSize.height());
        applyParameters();
    }

    emit previewSizeChanged();
}

void JCameraPrivate::setPreviewTexture(void *surfaceTexture)
{
    m_camera.callMethod<void>("setPreviewTexture",
                              "(Landroid/graphics/SurfaceTexture;)V",
                              static_cast<jobject>(surfaceTexture));
}

bool JCameraPrivate::isZoomSupported()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isZoomSupported");
}

int JCameraPrivate::getMaxZoom()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxZoom");
}

QList<int> JCameraPrivate::getZoomRatios()
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

int JCameraPrivate::getZoom()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getZoom");
}

void JCameraPrivate::setZoom(int value)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setZoom", "(I)V", value);
    applyParameters();
}

QString JCameraPrivate::getFlashMode()
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

void JCameraPrivate::setFlashMode(const QString &value)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setFlashMode",
                                  "(Ljava/lang/String;)V",
                                  QJNIObjectPrivate::fromString(value).object());
    applyParameters();
}

QString JCameraPrivate::getFocusMode()
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

void JCameraPrivate::setFocusMode(const QString &value)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setFocusMode",
                                  "(Ljava/lang/String;)V",
                                  QJNIObjectPrivate::fromString(value).object());
    applyParameters();
}

int JCameraPrivate::getMaxNumFocusAreas()
{
    if (QtAndroidPrivate::androidSdkVersion() < 14)
        return 0;

    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxNumFocusAreas");
}

QList<QRect> JCameraPrivate::getFocusAreas()
{
    QList<QRect> areas;

    if (QtAndroidPrivate::androidSdkVersion() < 14)
        return areas;

    QMutexLocker parametersLocker(&m_parametersMutex);

    if (m_parameters.isValid()) {
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

void JCameraPrivate::setFocusAreas(const QList<QRect> &areas)
{
    if (QtAndroidPrivate::androidSdkVersion() < 14)
        return;

    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
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

void JCameraPrivate::autoFocus()
{
    m_camera.callMethod<void>("autoFocus",
                              "(Landroid/hardware/Camera$AutoFocusCallback;)V",
                              m_cameraListener.object());
    emit autoFocusStarted();
}

void JCameraPrivate::cancelAutoFocus()
{
    m_camera.callMethod<void>("cancelAutoFocus");
}

bool JCameraPrivate::isAutoExposureLockSupported()
{
    if (QtAndroidPrivate::androidSdkVersion() < 14)
        return false;

    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isAutoExposureLockSupported");
}

bool JCameraPrivate::getAutoExposureLock()
{
    if (QtAndroidPrivate::androidSdkVersion() < 14)
        return false;

    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("getAutoExposureLock");
}

void JCameraPrivate::setAutoExposureLock(bool toggle)
{
    if (QtAndroidPrivate::androidSdkVersion() < 14)
        return;

    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setAutoExposureLock", "(Z)V", toggle);
    applyParameters();
}

bool JCameraPrivate::isAutoWhiteBalanceLockSupported()
{
    if (QtAndroidPrivate::androidSdkVersion() < 14)
        return false;

    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isAutoWhiteBalanceLockSupported");
}

bool JCameraPrivate::getAutoWhiteBalanceLock()
{
    if (QtAndroidPrivate::androidSdkVersion() < 14)
        return false;

    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("getAutoWhiteBalanceLock");
}

void JCameraPrivate::setAutoWhiteBalanceLock(bool toggle)
{
    if (QtAndroidPrivate::androidSdkVersion() < 14)
        return;

    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setAutoWhiteBalanceLock", "(Z)V", toggle);
    applyParameters();
}

int JCameraPrivate::getExposureCompensation()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getExposureCompensation");
}

void JCameraPrivate::setExposureCompensation(int value)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setExposureCompensation", "(I)V", value);
    applyParameters();
}

float JCameraPrivate::getExposureCompensationStep()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jfloat>("getExposureCompensationStep");
}

int JCameraPrivate::getMinExposureCompensation()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMinExposureCompensation");
}

int JCameraPrivate::getMaxExposureCompensation()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxExposureCompensation");
}

QString JCameraPrivate::getSceneMode()
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

void JCameraPrivate::setSceneMode(const QString &value)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setSceneMode",
                                  "(Ljava/lang/String;)V",
                                  QJNIObjectPrivate::fromString(value).object());
    applyParameters();
}

QString JCameraPrivate::getWhiteBalance()
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

void JCameraPrivate::setWhiteBalance(const QString &value)
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

void JCameraPrivate::updateRotation()
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    m_parameters.callMethod<void>("setRotation", "(I)V", m_rotation);
    applyParameters();
}

QList<QSize> JCameraPrivate::getSupportedPictureSizes()
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

void JCameraPrivate::setPictureSize(const QSize &size)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setPictureSize", "(II)V", size.width(), size.height());
    applyParameters();
}

void JCameraPrivate::setJpegQuality(int quality)
{
    QMutexLocker parametersLocker(&m_parametersMutex);

    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setJpegQuality", "(I)V", quality);
    applyParameters();
}

void JCameraPrivate::startPreview()
{
    //We need to clear preview buffers queue here, but there is no method to do it
    //Though just resetting preview callback do the trick
    m_camera.callMethod<void>("setPreviewCallbackWithBuffer",
                              "(Landroid/hardware/Camera$PreviewCallback;)V",
                              jobject(0));
    m_cameraListener.callMethod<void>("preparePreviewBuffer", "(Landroid/hardware/Camera;)V", m_camera.object());
    QJNIObjectPrivate buffer = m_cameraListener.callObjectMethod<jbyteArray>("callbackBuffer");
    m_camera.callMethod<void>("addCallbackBuffer", "([B)V", buffer.object());
    m_camera.callMethod<void>("setPreviewCallbackWithBuffer",
                              "(Landroid/hardware/Camera$PreviewCallback;)V",
                              m_cameraListener.object());
    m_camera.callMethod<void>("startPreview");
    emit previewStarted();
}

void JCameraPrivate::stopPreview()
{
    m_camera.callMethod<void>("stopPreview");
    emit previewStopped();
}

void JCameraPrivate::takePicture()
{
    m_camera.callMethod<void>("takePicture", "(Landroid/hardware/Camera$ShutterCallback;"
                                             "Landroid/hardware/Camera$PictureCallback;"
                                             "Landroid/hardware/Camera$PictureCallback;)V",
                                              m_cameraListener.object(),
                                              jobject(0),
                                              m_cameraListener.object());
}

void JCameraPrivate::fetchEachFrame(bool fetch)
{
    m_cameraListener.callMethod<void>("fetchEachFrame", "(Z)V", fetch);
}

void JCameraPrivate::fetchLastPreviewFrame()
{
    QJNIEnvironmentPrivate env;
    QJNIObjectPrivate data = m_cameraListener.callObjectMethod("lockAndFetchPreviewBuffer", "()[B");
    if (!data.isValid()) {
        m_cameraListener.callMethod<void>("unlockPreviewBuffer");
        return;
    }
    const int arrayLength = env->GetArrayLength(static_cast<jbyteArray>(data.object()));
    QByteArray bytes(arrayLength, Qt::Uninitialized);
    env->GetByteArrayRegion(static_cast<jbyteArray>(data.object()),
                            0,
                            arrayLength,
                            reinterpret_cast<jbyte *>(bytes.data()));
    m_cameraListener.callMethod<void>("unlockPreviewBuffer");

    emit previewFetched(bytes);
}

void JCameraPrivate::applyParameters()
{
    m_camera.callMethod<void>("setParameters",
                              "(Landroid/hardware/Camera$Parameters;)V",
                              m_parameters.object());
}

QStringList JCameraPrivate::callParametersStringListMethod(const QByteArray &methodName)
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

static JNINativeMethod methods[] = {
    {"notifyAutoFocusComplete", "(IZ)V", (void *)notifyAutoFocusComplete},
    {"notifyPictureExposed", "(I)V", (void *)notifyPictureExposed},
    {"notifyPictureCaptured", "(I[B)V", (void *)notifyPictureCaptured},
    {"notifyFrameFetched", "(I[B)V", (void *)notifyFrameFetched}
};

bool JCamera::initJNI(JNIEnv *env)
{
    jclass clazz = env->FindClass("org/qtproject/qt5/android/multimedia/QtCameraListener");
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (clazz) {
        g_qtCameraListenerClass = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (env->RegisterNatives(g_qtCameraListenerClass,
                                 methods,
                                 sizeof(methods) / sizeof(methods[0])) < 0) {
            return false;
        }
    }

    return true;
}

QT_END_NAMESPACE

#include "jcamera.moc"
