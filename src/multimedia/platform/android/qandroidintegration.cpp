/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include "qandroidintegration_p.h"
#include "qandroidmediadevices_p.h"
#include "private/qandroidglobal_p.h"
#include "private/qandroidmediacapturesession_p.h"
#include "private/androidmediaplayer_p.h"
#include "private/qandroidcamerasession_p.h"
#include "private/androidsurfacetexture_p.h"
#include "private/androidsurfaceview_p.h"
#include "private/androidcamera_p.h"
#include "private/qandroidcamera_p.h"
#include "private/qandroidimagecapture_p.h"
#include "private/qandroidmediaencoder_p.h"
#include "private/androidmediarecorder_p.h"
#include "private/qandroidformatsinfo_p.h"
#include "private/qandroidmediaplayer_p.h"
#include "private/qandroidaudiooutput_p.h"
#include "private/qandroidaudioinput_p.h"
#include "private/qandroidvideosink_p.h"
#include "private/qandroidaudiodecoder_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qtAndroidMediaPlugin, "qt.multimedia.android")

QAndroidIntegration::QAndroidIntegration()
{

}

QAndroidIntegration::~QAndroidIntegration()
{
    delete m_devices;
    delete m_formatInfo;
}

QPlatformMediaDevices *QAndroidIntegration::devices()
{
    if (!m_devices)
        m_devices = new QAndroidMediaDevices();
    return m_devices;
}

QPlatformAudioDecoder *QAndroidIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new QAndroidAudioDecoder(decoder);
}

QPlatformMediaFormatInfo *QAndroidIntegration::formatInfo()
{
    if (!m_formatInfo)
        m_formatInfo = new QAndroidFormatInfo();
    return m_formatInfo;

}

QPlatformMediaCaptureSession *QAndroidIntegration::createCaptureSession()
{
    return new QAndroidMediaCaptureSession();
}

QPlatformMediaPlayer *QAndroidIntegration::createPlayer(QMediaPlayer *player)
{
    return new QAndroidMediaPlayer(player);
}

QPlatformCamera *QAndroidIntegration::createCamera(QCamera *camera)
{
    return new QAndroidCamera(camera);
}

QPlatformMediaRecorder *QAndroidIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QAndroidMediaEncoder(recorder);
}

QPlatformImageCapture *QAndroidIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QAndroidImageCapture(imageCapture);
}

QPlatformAudioOutput *QAndroidIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QAndroidAudioOutput(q);
}

QPlatformAudioInput *QAndroidIntegration::createAudioInput(QAudioInput *audioInput)
{
    return new QAndroidAudioInput(audioInput);
}

QPlatformVideoSink *QAndroidIntegration::createVideoSink(QVideoSink *sink)
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
            || !AndroidSurfaceHolder::registerNativeMethods()
            || !QAndroidMediaDevices::registerNativeMethods()) {
        return JNI_ERR;
    }

    AndroidSurfaceTexture::registerNativeMethods();

    return JNI_VERSION_1_6;
}

QT_END_NAMESPACE
