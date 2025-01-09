// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qaudioinput.h>
#include <qaudiodevice.h>
#include <qmediadevices.h>
#include <private/qplatformaudioinput_p.h>
#include <private/qplatformmediaintegration_p.h>

#include <utility>

/*!
    \qmltype AudioInput
    \nativetype QAudioInput
    \brief An audio input to be used for capturing audio in a capture session.

    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_audio_qml

    \qml
    CaptureSession {
        id: playMusic
        audioInput: AudioInput {
            volume: slider.value
        }
        recorder: MediaRecorder { ... }
    }
    Slider {
        id: slider
        from: 0.
        to: 1.
    }
    \endqml

    You can use AudioInput together with a QtMultiMedia::CaptureSession to capture audio from an
   audio input device.

    \sa Camera, AudioOutput
*/

/*!
    \class QAudioInput
    \brief Represents an input channel for audio.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio

    This class represents an input channel that can be used together with
    QMediaCaptureSession. It enables the selection of the physical input device
    to be used, muting the channel, and changing the channel's volume.

    \note On WebAssembly platform, due to it's asynchronous nature,
    QMediaDevices::audioInputsChanged() signal is emitted when the list of
    audio inputs is ready. User permissions are required. Works only on secure https contexts.
*/

QAudioInput::QAudioInput(QObject *parent) : QAudioInput(QMediaDevices::defaultAudioInput(), parent)
{
}

QAudioInput::QAudioInput(const QAudioDevice &device, QObject *parent)
    : QObject(parent)
{
    auto maybeAudioInput = QPlatformMediaIntegration::instance()->createAudioInput(this);
    if (maybeAudioInput) {
        d = maybeAudioInput.value();
        d->device = device.mode() == QAudioDevice::Input ? device : QMediaDevices::defaultAudioInput();
        d->setAudioDevice(d->device);
    } else {
        d = new QPlatformAudioInput(nullptr);
        qWarning() << "Failed to initialize QAudioInput" << maybeAudioInput.error();
    }
}

QAudioInput::~QAudioInput()
{
    setDisconnectFunction({});
    delete d;
}

/*!
    \qmlproperty real QtMultimedia::AudioInput::volume

    The volume is scaled linearly, ranging from \c 0 (silence) to \c 1 (full volume).
    \note values outside this range will be clamped.

    By default the volume is \c 1.

    UI volume controls should usually be scaled non-linearly. For example,
    using a logarithmic scale will produce linear changes in perceived loudness,
    which is what a user would normally expect from a volume control.
    \sa QtAudio::convertVolume()
*/
/*!
    \property QAudioInput::volume

    The property returns the volume of the audio input.
*/
float QAudioInput::volume() const
{
    return d->volume;
}

void QAudioInput::setVolume(float volume)
{
    volume = qBound(0., volume, 1.);
    if (d->volume == volume)
        return;
    d->volume = volume;
    d->setVolume(volume);
    emit volumeChanged(volume);
}

/*!
    \qmlproperty bool QtMultimedia::AudioInput::muted

    This property holds whether the audio input is muted.

    Defaults to \c{false}.
*/

/*!
    \property QAudioInput::muted
    \brief The muted state of the current media.

    The value will be \c true if the input is muted; otherwise \c false.
*/
bool QAudioInput::isMuted() const
{
    return d->muted;
}

void QAudioInput::setMuted(bool muted)
{
    if (d->muted == muted)
        return;
    d->muted = muted;
    d->setMuted(muted);
    emit mutedChanged(muted);
}

/*!
    \qmlproperty AudioDevice QtMultimedia::AudioInput::device

    This property describes the audio device connected to this input.

    The device property represents the audio device this input is connected to.
    This property can be used to select an output device from the
    QtMultimedia::MediaDevices::audioInputs() list.
*/

/*!
    \property QAudioInput::device
    \brief The audio device connected to this input.

    The device property represents the audio device connected to this input.
    This property can be used to select an input device from the
    QMediaDevices::audioInputs() list.

    You can select the system default audio input by setting this property to
    a default constructed QAudioDevice object.
*/
QAudioDevice QAudioInput::device() const
{
    return d->device;
}

void QAudioInput::setDevice(const QAudioDevice &device)
{
    auto dev = device;
    if (dev.isNull())
        dev = QMediaDevices::defaultAudioInput();
    if (dev.mode() != QAudioDevice::Input)
        return;
    if (d->device == dev)
        return;
    d->device = dev;
    d->setAudioDevice(dev);
    emit deviceChanged();
}

/*!
    \internal
*/
void QAudioInput::setDisconnectFunction(std::function<void()> disconnectFunction)
{
    if (d->disconnectFunction) {
        auto df = d->disconnectFunction;
        d->disconnectFunction = {};
        df();
    }
    d->disconnectFunction = std::move(disconnectFunction);
}

#include "moc_qaudioinput.cpp"
