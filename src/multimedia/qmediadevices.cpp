/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmediadevices.h"
#include "private/qplatformmediaintegration_p.h"
#include "private/qplatformmediadevices_p.h"

#include <qaudiodeviceinfo.h>
#include <qcamerainfo.h>

QT_BEGIN_NAMESPACE

class QMediaDevicesPrivate
{
public:
    ~QMediaDevicesPrivate()
    {
    }
    QPlatformMediaDevices *platformDevices() {
        if (!platform)
            platform = QPlatformMediaIntegration::instance()->devices();
        return platform;
    }

private:
    QPlatformMediaDevices *platform = nullptr;

} priv;

/*!
    \class QMediaDevices
    \brief The QMediaDevices class provides information about available
    multimedia input and output devices.
    \ingroup multimedia
    \inmodule QtMultimedia

    The QMediaDevices class helps in managing the available multimedia
    input and output devices. It manages three types of devices:
    \list
    \li Audio input devices (Microphones)
    \li Audio output devices (Speakers, Headsets)
    \li Video input devices (Cameras)
    \endlist

    QMediaDevices allows listing all available devices and will emit
    signals when the list of available devices has changed.

    While using the default input and output devices is often sufficient for
    playing back or recording multimedia, there is often a need to explicitly
    select the device to be used.

    QMediaDevices is a singleton object and all getters are thread-safe.
*/

/*!
    Returns a list of available audio input devices on the system.

    Those devices are usually microphones, either built-in, or connected to
    the device through e.g. USB or Bluetooth.
*/
QList<QAudioDeviceInfo> QMediaDevices::audioInputs()
{
    return priv.platformDevices()->audioInputs();
}

/*!
    Returns a list of available audio input devices on the system.

    Those devices are usually loudspeakers or head sets, either built-in,
    or connected to the device through e.g. USB or Bluetooth.
*/
QList<QAudioDeviceInfo> QMediaDevices::audioOutputs()
{
    return priv.platformDevices()->audioOutputs();
}

/*!
    Returns a list of available cameras on the system.
*/
QList<QCameraInfo> QMediaDevices::videoInputs()
{
    return priv.platformDevices()->videoInputs();
}

/*!
    Returns the default audio input device.

    The default device can change during the runtime of the application. The audioInputsChanged()
    signal will get emitted in that case.
*/
QAudioDeviceInfo QMediaDevices::defaultAudioInput()
{
    const auto inputs = audioInputs();
    for (const auto &info : inputs)
        if (info.isDefault())
            return info;
    return inputs.value(0);
}

/*!
    Returns the default audio output device.

    The default device can change during the runtime of the application. The audioOutputsChanged()
    signal will get emitted in that case.
*/
QAudioDeviceInfo QMediaDevices::defaultAudioOutput()
{
    const auto outputs = audioOutputs();
    for (const auto &info : outputs)
        if (info.isDefault())
            return info;
    return outputs.value(0);
}

/*!
    Returns the default camera on the system.

    The returned object should be checked using isNull() before being used, in case there is no
    default camera or no cameras at all.

    The default device can change during the runtime of the application. The videoInputsChanged()
    signal will get emitted in that case.

    \sa availableCameras()
*/
QCameraInfo QMediaDevices::defaultVideoInput()
{
    const auto inputs = videoInputs();
    for (const auto &info : inputs)
        if (info.isDefault())
            return info;
    return inputs.value(0);
}

/*!
    \internal
*/
QMediaDevices::QMediaDevices(QObject *parent)
    : QObject(parent)
{
    priv.platformDevices()->addDevices(this);
}

/*!
    \internal
*/
QMediaDevices::~QMediaDevices()
{
    priv.platformDevices()->removeDevices(this);
}


QT_END_NAMESPACE

#include "moc_qmediadevices.cpp"
