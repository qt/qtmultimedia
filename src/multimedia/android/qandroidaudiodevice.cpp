// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidaudiodevice_p.h"

#include <private/qaudioformat_p.h>

#include <QtCore/qjniobject.h>

QT_BEGIN_NAMESPACE

QAndroidAudioDevice::QAndroidAudioDevice(QByteArray device, QString desc, QAudioDevice::Mode mode,
                                         bool isDefaultDevice)
    : QAudioDevicePrivate(std::move(device), mode, std::move(desc))
{
    isDefault = isDefaultDevice;

    // Report support for everything that Qt supports, as Android should be able to resample and
    // up/downmix if needed
    minimumChannelCount = 1;
    maximumChannelCount = 32;
    minimumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.front();
    maximumSampleRate = QtMultimediaPrivate::allSupportedSampleRates.back();
    supportedSampleFormats = QList<QAudioFormat::SampleFormat>{
        QtMultimediaPrivate::allSupportedSampleFormats.begin(),
        QtMultimediaPrivate::allSupportedSampleFormats.end()
    };

    QJniObject deviceInfo = QJniObject::callStaticObjectMethod(
            "org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
            mode == QAudioDevice::Input ? "getInputDeviceInfo" : "getOutputDeviceInfo",
            "(I)Landroid/media/AudioDeviceInfo;", QString::fromUtf8(id).toInt());

    // Set preferred channel count based on what device reports, with default set to stereo
    preferredFormat.setChannelCount(QJniObject::callStaticMethod<jint>(
            "org/qtproject/qt/android/multimedia/QtAudioDeviceManager", "getClampedChannelCount",
            "(Landroid/media/AudioDeviceInfo;I)I", deviceInfo, 2));

    // Get optimal sample rate from AudioManager
    preferredFormat.setSampleRate(QJniObject::callStaticMethod<jint>(
            "org/qtproject/qt/android/multimedia/QtAudioDeviceManager", "getDefaultSampleRate",
            "()I"));

    preferredFormat.setSampleFormat(QAudioFormat::Float);
}

QT_END_NAMESPACE

