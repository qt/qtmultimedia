// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidaudiodevices_p.h"

#include "qandroidaudiodevice_p.h"
#include "qandroidaudiosink_p.h"
#include "qandroidaudiosource_p.h"

#include <private/qplatformmediaintegration_p.h>

#include <QtCore/qjniobject.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtAudioDeviceManager,
                    "org/qtproject/qt/android/multimedia/QtAudioDeviceManager");

namespace {

QList<QAudioDevice> availableDevices(QAudioDevice::Mode mode)
{
    QList<QAudioDevice> devices;
    QJniObject deviceInfos;
    if (mode == QAudioDevice::Input) {
        deviceInfos = QJniObject::callStaticObjectMethod(
                "org/qtproject/qt/android/multimedia/QtAudioDeviceManager", "getUpdatedAudioInputDevices",
                "()[Landroid/media/AudioDeviceInfo;");
    } else if (mode == QAudioDevice::Output) {
        deviceInfos = QJniObject::callStaticObjectMethod(
                "org/qtproject/qt/android/multimedia/QtAudioDeviceManager", "getUpdatedAudioOutputDevices",
                "()[Landroid/media/AudioDeviceInfo;");
    }
    if (deviceInfos.isValid()) {
        QJniEnvironment env;
        jobjectArray deviceInfosArray = static_cast<jobjectArray>(deviceInfos.object());
        const jint size = env->GetArrayLength(deviceInfosArray);
        for (int i = 0; i < size; ++i) {
            auto dev = env->GetObjectArrayElement(deviceInfosArray, i);
            QJniObject deviceInfo(dev);
            int id = deviceInfo.callMethod<jint>("getId", "()I");
            jint deviceType = deviceInfo.callMethod<jint>("getType", "()I");
            auto description =
                    QJniObject::callStaticObjectMethod(
                            "org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
                            "audioDeviceTypeToString", "(I)Ljava/lang/String;", deviceType)
                            .toString();
            env->DeleteLocalRef(dev);
            devices << QAudioDevicePrivate::createQAudioDevice(
                    std::make_unique<QAndroidAudioDevice>(QString::number(id).toUtf8(), description,
                                                          mode, i == 0));
        }
    }
    return devices;
}

} // namespace

QAndroidAudioDevices::QAndroidAudioDevices() : QPlatformAudioDevices()
{
    QtJniTypes::QtAudioDeviceManager::callStaticMethod<void>("registerAudioHeadsetStateReceiver");
}

QAndroidAudioDevices::~QAndroidAudioDevices()
{
    // Object of QAndroidAudioDevices type is static. Unregistering will happend only when closing
    // the application. In such case it is probably not needed, but let's leave it for
    // compatibility with Android documentation
    QtJniTypes::QtAudioDeviceManager::callStaticMethod<void>("unregisterAudioHeadsetStateReceiver");
}

QList<QAudioDevice> QAndroidAudioDevices::findAudioInputs() const
{
    return availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QAndroidAudioDevices::findAudioOutputs() const
{
    return availableDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QAndroidAudioDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                              const QAudioFormat &fmt,
                                                              QObject *parent)
{
    return new QtAAudio::QAndroidAudioSource(deviceInfo, fmt, parent);
}

QPlatformAudioSink *QAndroidAudioDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                          const QAudioFormat &fmt, QObject *parent)
{
    return new QtAAudio::QAndroidAudioSink(deviceInfo, fmt, parent);
}

static void onAudioInputDevicesUpdated(JNIEnv * /*env*/, jobject /*thiz*/)
{
    static_cast<QAndroidAudioDevices *>(QPlatformMediaIntegration::instance()->audioDevices())
            ->onAudioInputsChanged();
}

static void onAudioOutputDevicesUpdated(JNIEnv * /*env*/, jobject /*thiz*/)
{
    static_cast<QAndroidAudioDevices *>(QPlatformMediaIntegration::instance()->audioDevices())
            ->onAudioOutputsChanged();
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

    const auto context = QNativeInterface::QAndroidApplication::context();
    QtJniTypes::QtAudioDeviceManager::callStaticMethod<void>("setContext", context);

    const JNINativeMethod methods[] = {
        { "onAudioInputDevicesUpdated", "()V", (void *)onAudioInputDevicesUpdated },
        { "onAudioOutputDevicesUpdated", "()V", (void *)onAudioOutputDevicesUpdated }
    };

    bool registered = QJniEnvironment().registerNativeMethods(
            "org/qtproject/qt/android/multimedia/QtAudioDeviceManager", methods,
            std::size(methods));

    if (!registered)
        return JNI_ERR;

    return JNI_VERSION_1_6;
}

QT_END_NAMESPACE
