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
Q_DECLARE_JNI_CLASS(AudioDeviceInfo, "android/media/AudioDeviceInfo");

using namespace QtJniTypes;

namespace {

QAudioFormat preferredFormatForDevice(const QtJniTypes::AudioDeviceInfo &deviceInfo)
{
    QAudioFormat preferredFormat;

    // Set preferred channel count based on what device reports, with default set to stereo (2)
    QJniArray<jint> channelCounts = deviceInfo.callMethod<QJniArray<jint>>("getChannelCounts");
    if (channelCounts.isEmpty()) {
        preferredFormat.setChannelCount(2);
    } else {
        const auto [minIt, maxIt] = std::minmax_element(channelCounts.begin(), channelCounts.end());
        preferredFormat.setChannelCount(std::clamp(2, *minIt, *maxIt));
    }

    // Get optimal sample rate from AudioManager
    preferredFormat.setSampleRate(
            QtAudioDeviceManager::callStaticMethod<jint>("getDefaultSampleRate"));

    // Using Float avoids conversions for processing, so we should prefer that instead of whatever
    // the device uses natively
    preferredFormat.setSampleFormat(QAudioFormat::Float);

    return preferredFormat;
};

QList<QAudioDevice> availableDevices(QAudioDevice::Mode mode)
{
    if (mode == QAudioDevice::Null)
        return {};

    QList<QAudioDevice> devices;
    const char *getMethod =
            mode == QAudioDevice::Input ? "getAudioInputDevices" : "getAudioOutputDevices";
    auto deviceInfos =
            QtAudioDeviceManager::callStaticMethod<QJniArray<AudioDeviceInfo>>(getMethod);

    if (!deviceInfos.isValid())
        return {};

    for (int i = 0; i < deviceInfos.size(); ++i) {
        AudioDeviceInfo deviceInfo = deviceInfos.at(i);
        int id = deviceInfo.callMethod<jint>("getId");
        jint deviceType = deviceInfo.callMethod<jint>("getType");
        auto description = QtAudioDeviceManager::callStaticMethod<QString>(
                "audioDeviceTypeToString", deviceType);
        bool isBluetoothDevice =
                QtAudioDeviceManager::callStaticMethod<jboolean>("isBluetoothDevice", deviceInfo);
        devices << QAudioDevicePrivate::createQAudioDevice(std::make_unique<QAndroidAudioDevice>(
                QString::number(id).toUtf8(), description, mode,
                preferredFormatForDevice(deviceInfo), isBluetoothDevice, i == 0));
    }

    return devices;
}

} // namespace

QAndroidAudioDevices::QAndroidAudioDevices() : QPlatformAudioDevices()
{
    QtAudioDeviceManager::callStaticMethod<void>("registerAudioHeadsetStateReceiver");
}

QAndroidAudioDevices::~QAndroidAudioDevices()
{
    // Object of QAndroidAudioDevices type is static. Unregistering will happend only when closing
    // the application. In such case it is probably not needed, but let's leave it for
    // compatibility with Android documentation
    QtAudioDeviceManager::callStaticMethod<void>("unregisterAudioHeadsetStateReceiver");
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
    QtAudioDeviceManager::callStaticMethod<void>("setContext", context);

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
