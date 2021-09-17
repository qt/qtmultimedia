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

#include <qaudioinput.h>
#include <qaudiodevice.h>
#include <qmediadevices.h>
#include <private/qplatformaudioinput_p.h>
#include <private/qplatformmediaintegration_p.h>

/*!
    \class QAudioInput
    \brief Represents an input channel for audio.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio

    This class represents an input channel that can be used together with
    QMediaCaptureSession. It enables the selection of the physical input device
    to be used, muting the channel, and changing the channel's volume.
*/

/*!
    \property QAudioInput::volume
    \brief The current volume.

    The volume is scaled linearly, ranging from \c 0 (silence) to \c 1
    (full volume).
    \note values outside this range will be clamped.

    By default the volume is \c 1.

    UI volume controls should usually be scaled non-linearly. For example,
    using a logarithmic scale will produce linear changes in perceived loudness,
    which is what a user would normally expect from a volume control.
    \sa QAudio::convertVolume()
*/

/*!
    \property QAudioInput::muted
    \brief The muted state of the current media.

    The value will be \c true if the input is muted; otherwise \c false.
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

QAudioInput::QAudioInput(QObject *parent)
  : QAudioInput(QMediaDevices::defaultAudioInput(), parent)
{}

QAudioInput::QAudioInput(const QAudioDevice &device, QObject *parent)
  : QObject(parent),
    d(QPlatformMediaIntegration::instance()->createAudioInput(this))
{
    d->device = device.mode() == QAudioDevice::Input ? device : QMediaDevices::defaultAudioInput();
    d->setAudioDevice(d->device);
}

QAudioInput::~QAudioInput()
{
    delete d;
}

QAudioDevice QAudioInput::device() const
{
    return d->device;
}

/*!
    Connects the audio input to the physical audio device described by
    \a device. Using a default constructed QAudioDevice object as \a device
    will connect the audio input to the system default audio input device.
*/

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

#include "moc_qaudioinput.cpp"
