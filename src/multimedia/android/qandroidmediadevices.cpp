// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidmediadevices_p.h"
#include "qmediadevices.h"
#include "private/qcameradevice_p.h"

#include "qandroidaudiosource_p.h"
#include "qandroidaudiosink_p.h"
#include "qandroidaudiodevice_p.h"
#include "qopenslesengine_p.h"
#include "private/qplatformmediaintegration_p.h"

#include <qjnienvironment.h>
#include <QJniObject>
#include <QCoreApplication>

QT_BEGIN_NAMESPACE

QAndroidMediaDevices::QAndroidMediaDevices() : QPlatformMediaDevices() { }

QList<QAudioDevice> QAndroidMediaDevices::audioInputs() const
{
    return QOpenSLESEngine::availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QAndroidMediaDevices::audioOutputs() const
{
    return QOpenSLESEngine::availableDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QAndroidMediaDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                              QObject *parent)
{
    return new QAndroidAudioSource(deviceInfo.id(), parent);
}

QPlatformAudioSink *QAndroidMediaDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                          QObject *parent)
{
    return new QAndroidAudioSink(deviceInfo.id(), parent);
}

void QAndroidMediaDevices::forwardAudioOutputsChanged()
{
    emit audioOutputsChanged();
}

void QAndroidMediaDevices::forwardAudioInputsChanged()
{
    emit audioInputsChanged();
}

static void onAudioInputDevicesUpdated(JNIEnv */*env*/, jobject /*thiz*/)
{
    static_cast<QAndroidMediaDevices*>(QPlatformMediaDevices::instance())->forwardAudioInputsChanged();
}

static void onAudioOutputDevicesUpdated(JNIEnv */*env*/, jobject /*thiz*/)
{
    static_cast<QAndroidMediaDevices*>(QPlatformMediaDevices::instance())->forwardAudioOutputsChanged();
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

    const JNINativeMethod methods[] = {
        { "onAudioInputDevicesUpdated", "()V", (void *)onAudioInputDevicesUpdated },
        { "onAudioOutputDevicesUpdated", "()V", (void *)onAudioOutputDevicesUpdated }
    };

    bool registered = QJniEnvironment().registerNativeMethods(
            "org/qtproject/qt/android/multimedia/QtAudioDeviceManager", methods,
            std::size(methods));

    if (!registered)
        return JNI_ERR;

    QJniObject::callStaticMethod<void>("org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
                                       "registerAudioHeadsetStateReceiver",
                                       "(Landroid/content/Context;)V",
                                       QNativeInterface::QAndroidApplication::context());

    return JNI_VERSION_1_6;
}

QT_END_NAMESPACE
