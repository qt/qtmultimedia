// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    \property QMediaDevices::audioInputs

    Returns a list of available audio input devices on the system.

    Those devices are usually microphones. Devices can be either built-in, or
    connected through for example USB or Bluetooth.
*/
QList<QAudioDevice> QMediaDevices::audioInputs()
{
    return QPlatformMediaDevices::instance()->audioInputs();
}

/*!
    \qmlproperty list<audioDevice> QtMultimedia::MediaDevices::audioOutputs
    Contains a list of available audio output devices on the system.

    Those devices are usually loudspeakers or head sets. Devices can be either
    built-in, or connected through for example USB or Bluetooth.
*/

/*!
    \property QMediaDevices::audioOutputs

    Returns a list of available audio output devices on the system.

    Those devices are usually loudspeakers or head sets. Devices can be either
    built-in, or connected through for example USB or Bluetooth.
*/
QList<QAudioDevice> QMediaDevices::audioOutputs()
{
    return QPlatformMediaDevices::instance()->audioOutputs();
}

/*!
    \qmlproperty list<cameraDevice> QtMultimedia::MediaDevices::videoInputs
    Contains a list of cameras on the system.
*/

/*!
    \property QMediaDevices::videoInputs

    Returns a list of available cameras on the system.
*/
QList<QCameraDevice> QMediaDevices::videoInputs()
{
    QPlatformMediaDevices::instance()->initVideoDevicesConnection();
    return QPlatformMediaIntegration::instance()->videoInputs();
}

/*!
    \qmlproperty audioDevice QtMultimedia::MediaDevices::defaultAudioInput
    Returns the default audio input device.

    The default device can change during the runtime of the application. The value
    of this property will automatically adjust itself to such changes.
*/

/*!
    \property QMediaDevices::defaultAudioInput

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
    \property QMediaDevices::defaultAudioOutput

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
    \property QMediaDevices::defaultVideoInput

    Returns the default camera on the system.

    \note The returned object should be checked using isNull() before being used,
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
    auto platformDevices = QPlatformMediaDevices::instance();
    connect(platformDevices, &QPlatformMediaDevices::videoInputsChanged, this,
            &QMediaDevices::videoInputsChanged);
    connect(platformDevices, &QPlatformMediaDevices::audioInputsChanged, this,
            &QMediaDevices::audioInputsChanged);
    connect(platformDevices, &QPlatformMediaDevices::audioOutputsChanged, this,
            &QMediaDevices::audioOutputsChanged);
}

/*!
    \internal
*/
QMediaDevices::~QMediaDevices() = default;

void QMediaDevices::connectNotify(const QMetaMethod &signal)
{
    if (signal == QMetaMethod::fromSignal(&QMediaDevices::videoInputsChanged))
        QPlatformMediaDevices::instance()->initVideoDevicesConnection();

    QObject::connectNotify(signal);
}

QT_END_NAMESPACE

#include "moc_qmediadevices.cpp"
