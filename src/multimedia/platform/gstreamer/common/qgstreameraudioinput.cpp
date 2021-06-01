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
#include <private/qaudiodeviceinfo_gstreamer_p.h>
#include <qaudiodeviceinfo.h>

#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

Q_LOGGING_CATEGORY(qLcMediaAudioInput, "qt.multimedia.audioInput")

QT_BEGIN_NAMESPACE

QGstreamerAudioInput::QGstreamerAudioInput(QObject *parent)
    : QObject(parent),
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
}

int QGstreamerAudioInput::volume() const
{
    return m_volume;
}

bool QGstreamerAudioInput::isMuted() const
{
    return m_muted;
}

void QGstreamerAudioInput::setVolume(int vol)
{
    if (vol == m_volume)
        return;
    m_volume = vol;
    audioVolume.set("volume", vol/100.);
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

void QGstreamerAudioInput::setPipeline(const QGstPipeline &pipeline)
{
    gstPipeline = pipeline;
}

bool QGstreamerAudioInput::setAudioInput(const QAudioDeviceInfo &info)
{
    if (info == m_audioInput)
        return true;
    qCDebug(qLcMediaAudioInput) << "setAudioInput" << info.description() << info.isNull();
    m_audioInput = info;

    auto state = gstPipeline.state();
    if (state != GST_STATE_PLAYING)
        return changeAudioInput();

    auto pad = audioVolume.staticPad("src");
    pad.addProbe<&QGstreamerAudioInput::prepareAudioInputChange>(this, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM);

    return true;
}

bool QGstreamerAudioInput::changeAudioInput()
{
    qCDebug(qLcMediaAudioInput) << "Changing audio Input";
    QGstElement newSrc;
    auto *deviceInfo = static_cast<const QGStreamerAudioDeviceInfo *>(m_audioInput.handle());
    if (deviceInfo && deviceInfo->gstDevice)
        newSrc = gst_device_create_element(deviceInfo->gstDevice , "audiosrc");

    if (newSrc.isNull())
        newSrc = QGstElement("autoaudiosrc", "audiosrc");

    audioSrc.setStateSync(GST_STATE_NULL);
    gstAudioInput.remove(audioSrc);
    audioSrc = newSrc;
    gstAudioInput.add(audioSrc);
    audioSrc.link(audioVolume);
    audioSrc.setState(GST_STATE_PAUSED);

    return true;
}

void QGstreamerAudioInput::prepareAudioInputChange(const QGstPad &/*pad*/)
{
    qCDebug(qLcMediaAudioInput) << "Reconfiguring audio Input";

    gstPipeline.setStateSync(GST_STATE_PAUSED);
    changeAudioInput();
    gstPipeline.setState(GST_STATE_PLAYING);
}

QAudioDeviceInfo QGstreamerAudioInput::audioInput() const
{
    return m_audioInput;
}

QT_END_NAMESPACE
