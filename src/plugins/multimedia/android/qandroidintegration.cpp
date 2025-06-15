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
#include <private/qplatformvideodevices_p.h>

#include <QCoreApplication>
#include <QtCore/qjnitypes.h>

#include <QtMultimedia/private/qplatformmediaplugin_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qtAndroidMediaPlugin, "qt.multimedia.android")

namespace {
class AndroidVideoDevices : public QPlatformVideoDevices
{
public:
    using QPlatformVideoDevices::QPlatformVideoDevices;

protected:
    QList<QCameraDevice> findVideoInputs() const override
    {
        return QAndroidCameraSession::availableCameras();
    }
};
} // namespace

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
        if (name == u"android")
            return new QAndroidIntegration;
        return nullptr;
    }
};

QAndroidIntegration::QAndroidIntegration() : QPlatformMediaIntegration(QLatin1String("android")) { }

q23::expected<QPlatformAudioDecoder *, QString> QAndroidIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new QAndroidAudioDecoder(decoder);
}

QPlatformMediaFormatInfo *QAndroidIntegration::createFormatInfo()
{
    return new QAndroidFormatInfo;
}

q23::expected<QPlatformMediaCaptureSession *, QString> QAndroidIntegration::createCaptureSession()
{
    return new QAndroidMediaCaptureSession();
}

q23::expected<QPlatformMediaPlayer *, QString> QAndroidIntegration::createPlayer(QMediaPlayer *player)
{
    return new QAndroidMediaPlayer(player);
}

q23::expected<QPlatformCamera *, QString> QAndroidIntegration::createCamera(QCamera *camera)
{
    return new QAndroidCamera(camera);
}

q23::expected<QPlatformMediaRecorder *, QString> QAndroidIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QAndroidMediaEncoder(recorder);
}

q23::expected<QPlatformImageCapture *, QString> QAndroidIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QAndroidImageCapture(imageCapture);
}

q23::expected<QPlatformAudioOutput *, QString> QAndroidIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QAndroidAudioOutput(q);
}

q23::expected<QPlatformAudioInput *, QString> QAndroidIntegration::createAudioInput(QAudioInput *audioInput)
{
    return new QAndroidAudioInput(audioInput);
}

q23::expected<QPlatformVideoSink *, QString> QAndroidIntegration::createVideoSink(QVideoSink *sink)
{
    return new QAndroidVideoSink(sink);
}

Q_DECLARE_JNI_CLASS(QtMultimediaUtils, "org/qtproject/qt/android/multimedia/QtMultimediaUtils")

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

    const auto context = QNativeInterface::QAndroidApplication::context();
    QtJniTypes::QtMultimediaUtils::callStaticMethod<void>("setContext", context);

    if (!AndroidMediaPlayer::registerNativeMethods()
            || !AndroidCamera::registerNativeMethods()
            || !AndroidMediaRecorder::registerNativeMethods()
            || !AndroidSurfaceHolder::registerNativeMethods()) {
        return JNI_ERR;
    }

    AndroidSurfaceTexture::registerNativeMethods();

    return JNI_VERSION_1_6;
}

QPlatformVideoDevices *QAndroidIntegration::createVideoDevices()
{
    return new AndroidVideoDevices(this);
}

QT_END_NAMESPACE

#include "qandroidintegration.moc"
