// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidintegration_p.h"
#include "qandroidglobal_p.h"
#include "qandroidmediacapturesession_p.h"
#include "androidmediaplayer_p.h"
#include "qandroidcamerasession_p.h"
#include "androidsurfacetexture_p.h"
#include "androidsurfaceview_p.h"
#include "androidcamera_p.h"
#include "qandroidcamera_p.h"
#include "qandroidimagecapture_p.h"
#include "qandroidmediaencoder_p.h"
#include "androidmediarecorder_p.h"
#include "qandroidformatsinfo_p.h"
#include "qandroidmediaplayer_p.h"
#include "qandroidaudiooutput_p.h"
#include "qandroidaudioinput_p.h"
#include "qandroidvideosink_p.h"
#include "qandroidaudiodecoder_p.h"
#include <QtMultimedia/private/qplatformmediaplugin_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qtAndroidMediaPlugin, "qt.multimedia.android")

class QAndroidMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "android.json")

public:
    QAndroidMediaPlugin()
      : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == QLatin1String("android"))
            return new QAndroidIntegration;
        return nullptr;
    }
};


QAndroidIntegration::QAndroidIntegration()
{

}

QMaybe<QPlatformAudioDecoder *> QAndroidIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new QAndroidAudioDecoder(decoder);
}

QPlatformMediaFormatInfo *QAndroidIntegration::createFormatInfo()
{
    return new QAndroidFormatInfo;
}

QMaybe<QPlatformMediaCaptureSession *> QAndroidIntegration::createCaptureSession()
{
    return new QAndroidMediaCaptureSession();
}

QMaybe<QPlatformMediaPlayer *> QAndroidIntegration::createPlayer(QMediaPlayer *player)
{
    return new QAndroidMediaPlayer(player);
}

QMaybe<QPlatformCamera *> QAndroidIntegration::createCamera(QCamera *camera)
{
    return new QAndroidCamera(camera);
}

QMaybe<QPlatformMediaRecorder *> QAndroidIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QAndroidMediaEncoder(recorder);
}

QMaybe<QPlatformImageCapture *> QAndroidIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QAndroidImageCapture(imageCapture);
}

QMaybe<QPlatformAudioOutput *> QAndroidIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QAndroidAudioOutput(q);
}

QMaybe<QPlatformAudioInput *> QAndroidIntegration::createAudioInput(QAudioInput *audioInput)
{
    return new QAndroidAudioInput(audioInput);
}

QMaybe<QPlatformVideoSink *> QAndroidIntegration::createVideoSink(QVideoSink *sink)
{
    return new QAndroidVideoSink(sink);
}

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

    if (!AndroidMediaPlayer::registerNativeMethods()
            || !AndroidCamera::registerNativeMethods()
            || !AndroidMediaRecorder::registerNativeMethods()
            || !AndroidSurfaceHolder::registerNativeMethods()) {
        return JNI_ERR;
    }

    AndroidSurfaceTexture::registerNativeMethods();

    return JNI_VERSION_1_6;
}

QList<QCameraDevice> QAndroidIntegration::videoInputs()
{
    return QAndroidCameraSession::availableCameras();
}

QT_END_NAMESPACE

#include "qandroidintegration.moc"
