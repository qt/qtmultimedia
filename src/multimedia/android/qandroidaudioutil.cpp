// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidaudioutil_p.h"

#include "qandroidaudiodevice_p.h"

QT_BEGIN_NAMESPACE

namespace QAndroidAudioUtil {

bool supportsLowLatency()
{
    static const bool lowLatencyIsSupported = []() {
        // clang-format off
        QJniObject ctx(QNativeInterface::QAndroidApplication::context());
        if (!ctx.isValid())
            return false;

        QJniObject pm = ctx.callObjectMethod("getPackageManager",
                                             "()Landroid/content/pm/PackageManager;");
        if (!pm.isValid())
            return false;

        QJniObject audioFeatureField =
                QJniObject::getStaticObjectField("android/content/pm/PackageManager",
                                                 "FEATURE_AUDIO_LOW_LATENCY",
                                                 "Ljava/lang/String;");
        if (!audioFeatureField.isValid())
            return false;

        bool hasFeature = pm.callMethod<jboolean>("hasSystemFeature",
                                                  "(Ljava/lang/String;)Z",
                                                  audioFeatureField.object());
        return hasFeature;
        // clang-format on
    }();

    return lowLatencyIsSupported;
}

bool isDefaultBluetoothDevice(const QAudioDevice &device)
{
    return device.isDefault() && QAudioDevicePrivate::handle<QAndroidAudioDevice>(device)->isBluetoothDevice();
}

} // namespace QAndroidAudioUtil

QT_END_NAMESPACE
