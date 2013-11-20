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

QT_BEGIN_NAMESPACE

static jclass g_qtCameraClass = 0;
static QMap<int, JCamera*> g_objectMap;

static QRect areaToRect(jobject areaObj)
{
    QJNIObjectPrivate area(areaObj);
    QJNIObjectPrivate rect = area.getObjectField("rect", "android/graphics/Rect");

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
    JCamera *obj = g_objectMap.value(id, 0);
    if (obj)
        Q_EMIT obj->autoFocusComplete(success);
}

static void notifyPictureExposed(JNIEnv* , jobject, int id)
{
    JCamera *obj = g_objectMap.value(id, 0);
    if (obj)
        Q_EMIT obj->pictureExposed();
}

static void notifyPictureCaptured(JNIEnv *env, jobject, int id, jbyteArray data)
{
    JCamera *obj = g_objectMap.value(id, 0);
    if (obj) {
        QByteArray bytes;
        int arrayLength = env->GetArrayLength(data);
        bytes.resize(arrayLength);
        env->GetByteArrayRegion(data, 0, arrayLength, (jbyte*)bytes.data());
        Q_EMIT obj->pictureCaptured(bytes);
    }
}

static void notifyPreviewFrame(JNIEnv *env, jobject, int id, jbyteArray data)
{
    JCamera *obj = g_objectMap.value(id, 0);
    if (obj) {
        QByteArray bytes;
        int arrayLength = env->GetArrayLength(data);
        bytes.resize(arrayLength);
        env->GetByteArrayRegion(data, 0, arrayLength, (jbyte*)bytes.data());
        Q_EMIT obj->previewFrameAvailable(bytes);
    }
}

JCamera::JCamera(int cameraId, jobject cam)
    : QObject()
    , QJNIObjectPrivate(cam)
    , m_cameraId(cameraId)
    , m_hasAPI14(false)
{
    if (isValid()) {
        g_objectMap.insert(cameraId, this);

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

JCamera::~JCamera()
{
    if (isValid())
        g_objectMap.remove(m_cameraId);
}

JCamera *JCamera::open(int cameraId)
{
    QJNIEnvironmentPrivate env;

    QJNIObjectPrivate camera = callStaticObjectMethod(g_qtCameraClass,
                                                      "open",
                                                      "(I)Lorg/qtproject/qt5/android/multimedia/QtCamera;",
                                                      cameraId);

    if (!camera.isValid())
        return 0;
    else
        return new JCamera(cameraId, camera.object());
}

void JCamera::lock()
{
    callMethod<void>("lock");
}

void JCamera::unlock()
{
    callMethod<void>("unlock");
}

void JCamera::reconnect()
{
    callMethod<void>("reconnect");
}

void JCamera::release()
{
    m_previewSize = QSize();
    m_parameters = QJNIObjectPrivate();
    callMethod<void>("release");
}

JCamera::CameraFacing JCamera::getFacing()
{
    return CameraFacing(m_info.getField<jint>("facing"));
}

int JCamera::getNativeOrientation()
{
    return m_info.getField<jint>("orientation");
}

QSize JCamera::getPreferredPreviewSizeForVideo()
{
    if (!m_parameters.isValid())
        return QSize();

    QJNIObjectPrivate size = m_parameters.callObjectMethod("getPreferredPreviewSizeForVideo",
                                                           "()Landroid/hardware/Camera$Size;");

    return QSize(size.getField<jint>("width"), size.getField<jint>("height"));
}

QList<QSize> JCamera::getSupportedPreviewSizes()
{
    QList<QSize> list;

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

JCamera::ImageFormat JCamera::getPreviewFormat()
{
    if (!m_parameters.isValid())
        return Unknown;

    return JCamera::ImageFormat(m_parameters.callMethod<jint>("getPreviewFormat"));
}

void JCamera::setPreviewFormat(ImageFormat fmt)
{
    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setPreviewFormat", "(I)V", jint(fmt));
    applyParameters();
}

void JCamera::setPreviewSize(const QSize &size)
{
    if (!m_parameters.isValid())
        return;

    m_previewSize = size;

    if (m_previewSize.isValid()) {
        m_parameters.callMethod<void>("setPreviewSize", "(II)V", size.width(), size.height());
        applyParameters();
    }

    emit previewSizeChanged();
}

void JCamera::setPreviewTexture(jobject surfaceTexture)
{
    callMethod<void>("setPreviewTexture", "(Landroid/graphics/SurfaceTexture;)V", surfaceTexture);
}

bool JCamera::isZoomSupported()
{
    if (!m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isZoomSupported");
}

int JCamera::getMaxZoom()
{
    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxZoom");
}

QList<int> JCamera::getZoomRatios()
{
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

int JCamera::getZoom()
{
    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getZoom");
}

void JCamera::setZoom(int value)
{
    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setZoom", "(I)V", value);
    applyParameters();
}

QStringList JCamera::getSupportedFlashModes()
{
    return callStringListMethod("getSupportedFlashModes");
}

QString JCamera::getFlashMode()
{
    QString value;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate flashMode = m_parameters.callObjectMethod("getFlashMode",
                                                                    "()Ljava/lang/String;");
        if (flashMode.isValid())
            value = flashMode.toString();
    }

    return value;
}

void JCamera::setFlashMode(const QString &value)
{
    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setFlashMode",
                                  "(Ljava/lang/String;)V",
                                  QJNIObjectPrivate::fromString(value).object());
    applyParameters();
}

QStringList JCamera::getSupportedFocusModes()
{
    return callStringListMethod("getSupportedFocusModes");
}

QString JCamera::getFocusMode()
{
    QString value;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate focusMode = m_parameters.callObjectMethod("getFocusMode",
                                                                    "()Ljava/lang/String;");
        if (focusMode.isValid())
            value = focusMode.toString();
    }

    return value;
}

void JCamera::setFocusMode(const QString &value)
{
    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setFocusMode",
                                  "(Ljava/lang/String;)V",
                                  QJNIObjectPrivate::fromString(value).object());
    applyParameters();
}

int JCamera::getMaxNumFocusAreas()
{
    if (!m_hasAPI14 || !m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxNumFocusAreas");
}

QList<QRect> JCamera::getFocusAreas()
{
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

void JCamera::setFocusAreas(const QList<QRect> &areas)
{
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

void JCamera::autoFocus()
{
    callMethod<void>("autoFocus");
    emit autoFocusStarted();
}

void JCamera::cancelAutoFocus()
{
    callMethod<void>("cancelAutoFocus");
}

bool JCamera::isAutoExposureLockSupported()
{
    if (!m_hasAPI14 || !m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isAutoExposureLockSupported");
}

bool JCamera::getAutoExposureLock()
{
    if (!m_hasAPI14 || !m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("getAutoExposureLock");
}

void JCamera::setAutoExposureLock(bool toggle)
{
    if (!m_hasAPI14 || !m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setAutoExposureLock", "(Z)V", toggle);
    applyParameters();
}

bool JCamera::isAutoWhiteBalanceLockSupported()
{
    if (!m_hasAPI14 || !m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("isAutoWhiteBalanceLockSupported");
}

bool JCamera::getAutoWhiteBalanceLock()
{
    if (!m_hasAPI14 || !m_parameters.isValid())
        return false;

    return m_parameters.callMethod<jboolean>("getAutoWhiteBalanceLock");
}

void JCamera::setAutoWhiteBalanceLock(bool toggle)
{
    if (!m_hasAPI14 || !m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setAutoWhiteBalanceLock", "(Z)V", toggle);
    applyParameters();
}

int JCamera::getExposureCompensation()
{
    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getExposureCompensation");
}

void JCamera::setExposureCompensation(int value)
{
    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setExposureCompensation", "(I)V", value);
    applyParameters();
}

float JCamera::getExposureCompensationStep()
{
    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jfloat>("getExposureCompensationStep");
}

int JCamera::getMinExposureCompensation()
{
    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMinExposureCompensation");
}

int JCamera::getMaxExposureCompensation()
{
    if (!m_parameters.isValid())
        return 0;

    return m_parameters.callMethod<jint>("getMaxExposureCompensation");
}

QStringList JCamera::getSupportedSceneModes()
{
    return callStringListMethod("getSupportedSceneModes");
}

QString JCamera::getSceneMode()
{
    QString value;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate sceneMode = m_parameters.callObjectMethod("getSceneMode",
                                                                    "()Ljava/lang/String;");
        if (sceneMode.isValid())
            value = sceneMode.toString();
    }

    return value;
}

void JCamera::setSceneMode(const QString &value)
{
    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setSceneMode",
                                  "(Ljava/lang/String;)V",
                                  QJNIObjectPrivate::fromString(value).object());
    applyParameters();
}

QStringList JCamera::getSupportedWhiteBalance()
{
    return callStringListMethod("getSupportedWhiteBalance");
}

QString JCamera::getWhiteBalance()
{
    QString value;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate wb = m_parameters.callObjectMethod("getWhiteBalance",
                                                             "()Ljava/lang/String;");
        if (wb.isValid())
            value = wb.toString();
    }

    return value;
}

void JCamera::setWhiteBalance(const QString &value)
{
    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setWhiteBalance",
                                  "(Ljava/lang/String;)V",
                                  QJNIObjectPrivate::fromString(value).object());
    applyParameters();

    emit whiteBalanceChanged();
}

void JCamera::setRotation(int rotation)
{
    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setRotation", "(I)V", rotation);
    applyParameters();
}

QList<QSize> JCamera::getSupportedPictureSizes()
{
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

void JCamera::setPictureSize(const QSize &size)
{
    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setPictureSize", "(II)V", size.width(), size.height());
    applyParameters();
}

void JCamera::setJpegQuality(int quality)
{
    if (!m_parameters.isValid())
        return;

    m_parameters.callMethod<void>("setJpegQuality", "(I)V", quality);
    applyParameters();
}

void JCamera::requestPreviewFrame()
{
    callMethod<void>("requestPreviewFrame");
}

void JCamera::takePicture()
{
    callMethod<void>("takePicture");
}

void JCamera::startPreview()
{
    callMethod<void>("startPreview");
}

void JCamera::stopPreview()
{
    callMethod<void>("stopPreview");
}

void JCamera::applyParameters()
{
    callMethod<void>("setParameters",
                     "(Landroid/hardware/Camera$Parameters;)V",
                     m_parameters.object());
}

QStringList JCamera::callStringListMethod(const char *methodName)
{
    QStringList stringList;

    if (m_parameters.isValid()) {
        QJNIObjectPrivate list = m_parameters.callObjectMethod(methodName,
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
    {"notifyPreviewFrame", "(I[B)V", (void *)notifyPreviewFrame}
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
