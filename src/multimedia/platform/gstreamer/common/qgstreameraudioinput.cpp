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

#include <private/qgstreameraudioinput_p.h>
#include <private/qgstreameraudiodevice_p.h>
#include <qaudiodevice.h>
#include <qaudioinput.h>

#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

Q_LOGGING_CATEGORY(qLcMediaAudioInput, "qt.multimedia.audioInput")

QT_BEGIN_NAMESPACE

QGstreamerAudioInput::QGstreamerAudioInput(QAudioInput *parent)
  : QObject(parent),
    QPlatformAudioInput(parent),
    gstAudioInput("audioInput")
{
    audioSrc = QGstElement("autoaudiosrc", "autoaudiosrc");
    audioVolume = QGstElement("volume", "volume");
    gstAudioInput.add(audioSrc, audioVolume);
    audioSrc.link(audioVolume);

    gstAudioInput.addGhostPad(audioVolume, "src");
}

QGstreamerAudioInput::~QGstreamerAudioInput()
{
    gstAudioInput.setStateSync(GST_STATE_NULL);
}

int QGstreamerAudioInput::volume() const
{
    return m_volume;
}

bool QGstreamerAudioInput::isMuted() const
{
    return m_muted;
}

void QGstreamerAudioInput::setVolume(float vol)
{
    if (vol == m_volume)
        return;
    m_volume = vol;
    audioVolume.set("volume", vol);
    emit volumeChanged(m_volume);
}

void QGstreamerAudioInput::setMuted(bool muted)
{
    if (muted == m_muted)
        return;
    m_muted = muted;
    audioVolume.set("mute", muted);
    emit mutedChanged(muted);
}

void QGstreamerAudioInput::setAudioDevice(const QAudioDevice &device)
{
    if (device == m_audioDevice)
        return;
    qCDebug(qLcMediaAudioInput) << "setAudioInput" << device.description() << device.isNull();
    m_audioDevice = device;

    QGstElement newSrc;
    auto *deviceInfo = static_cast<const QGStreamerAudioDeviceInfo *>(m_audioDevice.handle());
    if (deviceInfo && deviceInfo->gstDevice)
        newSrc = gst_device_create_element(deviceInfo->gstDevice, "audiosrc");
    else
        qCWarning(qLcMediaAudioInput) << "Invalid audio device";

    if (newSrc.isNull()) {
        qCWarning(qLcMediaAudioInput) << "Failed to create a gst element for the audio device, using a default audio source";
        newSrc = QGstElement("autoaudiosrc", "audiosrc");
    }

    // FIXME: most probably source can be disconnected outside of idle probe
    audioSrc.staticPad("src").doInIdleProbe([&](){
        audioSrc.unlink(audioVolume);
    });
    audioSrc.setStateSync(GST_STATE_NULL);
    gstAudioInput.remove(audioSrc);
    audioSrc = newSrc;
    gstAudioInput.add(audioSrc);
    audioSrc.link(audioVolume);
    audioSrc.syncStateWithParent();
}

QAudioDevice QGstreamerAudioInput::audioInput() const
{
    return m_audioDevice;
}

QT_END_NAMESPACE
