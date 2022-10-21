/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidmediadevices_p.h"
#include "qmediadevices.h"
#include "qcameradevice_p.h"

#include "private/qandroidaudiosource_p.h"
#include "private/qandroidaudiosink_p.h"
#include "private/qandroidaudiodevice_p.h"
#include "private/qopenslesengine_p.h"
#include "private/qplatformmediaintegration_p.h"
#include "private/qandroidcamerasession_p.h"

QT_BEGIN_NAMESPACE

QAndroidMediaDevices::QAndroidMediaDevices()
    : QPlatformMediaDevices()
{
}

QList<QAudioDevice> QAndroidMediaDevices::audioInputs() const
{
    return QOpenSLESEngine::availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QAndroidMediaDevices::audioOutputs() const
{
    return QOpenSLESEngine::availableDevices(QAudioDevice::Output);
}

QList<QCameraDevice> QAndroidMediaDevices::videoInputs() const
{
    return QAndroidCameraSession::availableCameras();
}

QPlatformAudioSource *QAndroidMediaDevices::createAudioSource(const QAudioDevice &deviceInfo)
{
    return new QAndroidAudioSource(deviceInfo.id());
}

QPlatformAudioSink *QAndroidMediaDevices::createAudioSink(const QAudioDevice &deviceInfo)
{
    return new QAndroidAudioSink(deviceInfo.id());
}

void QAndroidMediaDevices::forwardAudioOutputsChanged()
{
    audioOutputsChanged();
}

void QAndroidMediaDevices::forwardAudioInputsChanged()
{
    audioInputsChanged();
}

static void onAudioInputDevicesUpdated(JNIEnv */*env*/, jobject /*thiz*/)
{
    static_cast<QAndroidMediaDevices*>(
                QPlatformMediaIntegration::instance()->devices())->forwardAudioInputsChanged();
}

static void onAudioOutputDevicesUpdated(JNIEnv */*env*/, jobject /*thiz*/)
{
    static_cast<QAndroidMediaDevices*>(
                QPlatformMediaIntegration::instance()->devices())->forwardAudioOutputsChanged();
}

bool QAndroidMediaDevices::registerNativeMethods()
{
    static const JNINativeMethod methods[] = {
        {"onAudioInputDevicesUpdated","()V",(void*)onAudioInputDevicesUpdated},
        {"onAudioOutputDevicesUpdated", "()V",(void*)onAudioOutputDevicesUpdated}
    };
    const int size = std::size(methods);
    return QJniEnvironment().registerNativeMethods(
                "org/qtproject/qt/android/multimedia/QtAudioDeviceManager", methods, size);
}

QT_END_NAMESPACE
