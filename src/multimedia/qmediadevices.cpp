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

#include <qaudiodevice.h>
#include <qcameradevice.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMediaDevices
    \brief The QMediaDevices class provides information about available
    multimedia input and output devices.
    \ingroup multimedia
    \inmodule QtMultimedia

    The QMediaDevices class provides information about the available multimedia
    devices and the system defaults. It monitors the following three groups:
    \list
    \li Audio input devices (Microphones)
    \li Audio output devices (Speakers, Headsets)
    \li Video input devices (Cameras)
    \endlist

    QMediaDevices provides a separate list for each device group. If it detects that a
    new device has been connected to the system or an attached device has been disconnected
    from the system, it will update the corresponding device list and emit a signal
    notifying about the change.

    QMediaDevices monitors the system defaults for each device group. It will notify about
    any changes done through the system settings. For example, if the user selects a new
    default audio output in the system settings, QMediaDevices will update the default audio
    output accordingly and emit a signal. If the system does not provide a default for a
    camera or an audio input, QMediaDevices will select the first device from the list as
    the default device.

    While using the default input and output devices is often sufficient for
    playing back or recording multimedia, there is often a need to explicitly
    select the device to be used.

    QMediaDevices is a singleton object and all getters are thread-safe.
*/

/*!
    \qmltype MediaDevices
    \since 6.2
    \instantiates QMediaDevices
    \brief MediaDevices provides information about available
    multimedia input and output devices.
    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml

    The MediaDevices type provides information about the available multimedia
    devices and the system defaults. It monitors the following three groups:
    \list
    \li Audio input devices (Microphones)
    \li Audio output devices (Speakers, Headsets)
    \li Video input devices (Cameras)
    \endlist

    MediaDevices provides a separate list for each device group. If it detects that a
    new device has been connected to the system or an attached device has been disconnected
    from the system, it will update the corresponding device list and emit a signal
    notifying about the change.

    MediaDevices monitors the system defaults for each device group. It will notify about
    any changes done through the system settings. For example, if the user selects a new
    default audio output in the system settings, MediaDevices will update the default audio
    output accordingly and emit a signal. If the system does not provide a default for a
    camera or an audio input, MediaDevices will select the first device from the list as
    the default device.

    While using the default input and output devices is often sufficient for
    playing back or recording multimedia, there is often a need to explicitly
    select the device to be used.

    For example, the snippet below will ensure that the media player always uses
    the systems default audio output device for playback:

    \qml
    MediaDevices {
        id: devices
    }
    MediaPlayer {
        ...
        audioOutput: AudioOutput {
            device: devices.defaultAudioOutput
        }
    }
    \endqml

    \sa Camera, AudioInput, VideoOutput
*/

/*!
    \qmlproperty list<audioDevice> QtMultimedia::MediaDevices::audioInputs
    Contains a list of available audio input devices on the system.

    Those devices are usually microphones. Devices can be either built-in, or
    connected through for example USB or Bluetooth.
*/

/*!
    Returns a list of available audio input devices on the system.

    Those devices are usually microphones. Devices can be either built-in, or
    connected through for example USB or Bluetooth.
*/
QList<QAudioDevice> QMediaDevices::audioInputs()
{
    return QPlatformMediaIntegration::instance()->devices()->audioInputs();
}

/*!
    \qmlproperty list<audioDevice> QtMultimedia::MediaDevices::audioOutputs
    Contains a list of available audio output devices on the system.

    Those devices are usually loudspeakers or head sets. Devices can be either
    built-in, or connected through for example USB or Bluetooth.
*/

/*!
    Returns a list of available audio output devices on the system.

    Those devices are usually loudspeakers or head sets. Devices can be either
    built-in, or connected through for example USB or Bluetooth.
*/
QList<QAudioDevice> QMediaDevices::audioOutputs()
{
    return QPlatformMediaIntegration::instance()->devices()->audioOutputs();
}

/*!
    \qmlproperty list<cameraDevice> QtMultimedia::MediaDevices::videoInputs
    Contains a list of cameras on the system.
*/

/*!
    Returns a list of available cameras on the system.
*/
QList<QCameraDevice> QMediaDevices::videoInputs()
{
    return QPlatformMediaIntegration::instance()->devices()->videoInputs();
}

/*!
    \qmlproperty audioDevice QtMultimedia::MediaDevices::defaultAudioInput
    Returns the default audio input device.

    The default device can change during the runtime of the application. The value
    of this property will automatically adjust itself to such changes.
*/

/*!
    Returns the default audio input device.

    The default device can change during the runtime of the application.
    The audioInputsChanged() signal is emitted in this case.
*/
QAudioDevice QMediaDevices::defaultAudioInput()
{
    const auto inputs = audioInputs();
    if (inputs.isEmpty())
        return {};
    for (const auto &info : inputs)
        if (info.isDefault())
            return info;
    return inputs.value(0);
}

/*!
    \qmlproperty audioDevice QtMultimedia::MediaDevices::defaultAudioOutput
    Returns the default audio output device.

    The default device can change during the runtime of the application. The value
    of this property will automatically adjust itself to such changes.
*/

/*!
    Returns the default audio output device.

    The default device can change during the runtime of the application. The
    audioOutputsChanged() signal is emitted in this case.
*/
QAudioDevice QMediaDevices::defaultAudioOutput()
{
    const auto outputs = audioOutputs();
    if (outputs.isEmpty())
        return {};
    for (const auto &info : outputs)
        if (info.isDefault())
            return info;
    return outputs.value(0);
}

/*!
    \qmlproperty cameraDevice QtMultimedia::MediaDevices::defaultVideoInput
    Returns the default camera on the system.

    \note The returned object should be checked using isNull() before being used,
    in case there is no camera available.

    The default device can change during the runtime of the application. The value
    of this property will automatically adjust itself to such changes.
*/

/*!
    Returns the default camera on the system.

    /note The returned object should be checked using isNull() before being used,
    in case there is no default camera or no cameras at all.

    The default device can change during the runtime of the application. The
    videoInputsChanged() signal is emitted in that case.

    \sa videoInputs()
*/
QCameraDevice QMediaDevices::defaultVideoInput()
{
    const auto inputs = videoInputs();
    if (inputs.isEmpty())
        return {};
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
    QPlatformMediaIntegration::instance()->devices()->addDevices(this);
}

/*!
    \internal
*/
QMediaDevices::~QMediaDevices()
{
    QPlatformMediaIntegration::instance()->devices()->removeDevices(this);
}


QT_END_NAMESPACE

#include "moc_qmediadevices.cpp"
