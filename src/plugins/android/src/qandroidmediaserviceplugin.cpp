/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qandroidmediaserviceplugin.h"

#include "qandroidmediaservice.h"
#include "qandroidcaptureservice.h"
#include "qandroidaudioinputselectorcontrol.h"
#include "qandroidcamerainfocontrol.h"
#include "qandroidcamerasession.h"
#include "androidmediaplayer.h"
#include "androidsurfacetexture.h"
#include "androidcamera.h"
#include "androidmultimediautils.h"
#include "androidmediarecorder.h"
#include "androidsurfaceview.h"
#include "qandroidglobal.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qtAndroidMediaPlugin, "qt.multimedia.plugins.android")

QAndroidMediaServicePlugin::QAndroidMediaServicePlugin()
{
}

QAndroidMediaServicePlugin::~QAndroidMediaServicePlugin()
{
}

QMediaService *QAndroidMediaServicePlugin::create(const QString &key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER))
        return new QAndroidMediaService;

    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA)
            || key == QLatin1String(Q_MEDIASERVICE_AUDIOSOURCE)) {
        return new QAndroidCaptureService(key);
    }

    qCWarning(qtAndroidMediaPlugin) << "Android service plugin: unsupported key:" << key;
    return 0;
}

void QAndroidMediaServicePlugin::release(QMediaService *service)
{
    delete service;
}

QMediaServiceProviderHint::Features QAndroidMediaServicePlugin::supportedFeatures(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_MEDIAPLAYER)
        return QMediaServiceProviderHint::VideoSurface;

    if (service == Q_MEDIASERVICE_CAMERA)
        return QMediaServiceProviderHint::VideoSurface | QMediaServiceProviderHint::RecordingSupport;

    if (service == Q_MEDIASERVICE_AUDIOSOURCE)
        return QMediaServiceProviderHint::RecordingSupport;

    return QMediaServiceProviderHint::Features();
}

QByteArray QAndroidMediaServicePlugin::defaultDevice(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA && !QAndroidCameraSession::availableCameras().isEmpty())
        return QAndroidCameraSession::availableCameras().first().name;

    return QByteArray();
}

QList<QByteArray> QAndroidMediaServicePlugin::devices(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        QList<QByteArray> devices;
        const QList<AndroidCameraInfo> &cameras = QAndroidCameraSession::availableCameras();
        for (int i = 0; i < cameras.count(); ++i)
            devices.append(cameras.at(i).name);
        return devices;
    }

    if (service == Q_MEDIASERVICE_AUDIOSOURCE)
        return QAndroidAudioInputSelectorControl::availableDevices();

    return QList<QByteArray>();
}

QString QAndroidMediaServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        const QList<AndroidCameraInfo> &cameras = QAndroidCameraSession::availableCameras();
        for (int i = 0; i < cameras.count(); ++i) {
            const AndroidCameraInfo &info = cameras.at(i);
            if (info.name == device)
                return info.description;
        }
    }

    if (service == Q_MEDIASERVICE_AUDIOSOURCE)
        return QAndroidAudioInputSelectorControl::availableDeviceDescription(device);

    return QString();
}

QCamera::Position QAndroidMediaServicePlugin::cameraPosition(const QByteArray &device) const
{
    return QAndroidCameraInfoControl::position(device);
}

int QAndroidMediaServicePlugin::cameraOrientation(const QByteArray &device) const
{
    return QAndroidCameraInfoControl::orientation(device);
}

QT_END_NAMESPACE

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void * /*reserved*/)
{
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    QT_USE_NAMESPACE
    typedef union {
        JNIEnv *nativeEnvironment;
        void *venv;
    } UnionJNIEnvToVoid;

    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_6) != JNI_OK)
        return JNI_ERR;

    JNIEnv *jniEnv = uenv.nativeEnvironment;

    if (!AndroidMediaPlayer::initJNI(jniEnv) ||
        !AndroidCamera::initJNI(jniEnv) ||
        !AndroidMediaRecorder::initJNI(jniEnv) ||
        !AndroidSurfaceHolder::initJNI(jniEnv)) {
        return JNI_ERR;
    }

    AndroidSurfaceTexture::initJNI(jniEnv);

    return JNI_VERSION_1_6;
}
