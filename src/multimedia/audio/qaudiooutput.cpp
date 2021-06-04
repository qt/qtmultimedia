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

#include <qaudiooutput.h>
#include <qaudiodevice.h>
#include <qmediadevices.h>
#include <private/qplatformaudiooutput_p.h>
#include <private/qplatformmediaintegration_p.h>

/*!
    \class QAudioOutput
    \brief The QAudioOutput class represents an output channel for audio.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio

    QAudioOutput represents an output channel that can be used together with QMediaPlayer or
    QMediaCaptureSession. It allows selecting the physical output device to be used, muting the channel
    or changing its volume.
*/

/*!
    \property QAudioOutput::volume
    \brief the current volume.

    The volume is scaled linearly, ranging from \c 0 (silence) to \c 1 (full volume).
    Values outside this range will be clamped.

    By default the volume is \c 1.

    UI volume controls should usually be scaled nonlinearly. For example, using a logarithmic scale
    will produce linear changes in perceived loudness, which is what a user would normally expect
    from a volume control. See QAudio::convertVolume() for more details.
*/

/*!
    \property QAudioOutput::muted
    \brief the muted state of the current media.

    The value will be true if the output is muted; otherwise false.
*/

/*!
    \property QAudioOutput::device
    \brief the audio device connected to this output.

    The device property represents the audio device connected to this output. A default constructed
    QAudioOutput object will be connected to the systems default audio output at construction time.

    This property can be used to select any other output device listed by QMediaDevices::audioOutputs().
*/

QAudioOutput::QAudioOutput(QObject *parent)
    : QAudioOutput(QMediaDevices::defaultAudioOutput(), parent)
{}

QAudioOutput::QAudioOutput(const QAudioDevice &device, QObject *parent)
    : QObject(parent),
    d(QPlatformMediaIntegration::instance()->createAudioOutput(this))
{
    d->device = device;
    if (!d->device.isNull() && d->device.mode() != QAudio::AudioOutput)
        d->device = QMediaDevices::defaultAudioOutput();
    d->setAudioDevice(d->device);
}

QAudioOutput::~QAudioOutput()
{
    delete d;
}

QAudioDevice QAudioOutput::device() const
{
    return d->device;
}

void QAudioOutput::setDevice(const QAudioDevice &device)
{
    if (device.mode() != QAudio::AudioOutput)
        return;
    if (d->device == device)
        return;
    d->device = device;
    d->setAudioDevice(device);
    emit deviceChanged();
}

float QAudioOutput::volume() const
{
    return d->volume;
}

void QAudioOutput::setVolume(float volume)
{
    if (d->volume == volume)
        return;
    d->volume = volume;
    d->setVolume(volume);
    emit volumeChanged(volume);
}

bool QAudioOutput::isMuted() const
{
    return d->muted;
}

void QAudioOutput::setMuted(bool muted)
{
    if (d->muted == muted)
        return;
    d->muted = muted;
    d->setMuted(muted);
    emit mutedChanged(muted);
}

#include "moc_qaudiooutput.cpp"
