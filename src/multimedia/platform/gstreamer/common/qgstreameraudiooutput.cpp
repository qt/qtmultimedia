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

#include <private/qgstreameraudiooutput_p.h>
#include <private/qgstreameraudiodevice_p.h>
#include <qaudiodevice.h>

#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

Q_LOGGING_CATEGORY(qLcMediaAudioOutput, "qt.multimedia.audiooutput")

QT_BEGIN_NAMESPACE

QGstreamerAudioOutput::QGstreamerAudioOutput(QObject *parent)
    : QObject(parent),
      gstAudioOutput("audioOutput")
{
    audioQueue = QGstElement("queue", "audioQueue");
    audioConvert = QGstElement("audioconvert", "audioConvert");
    audioResample = QGstElement("audioresample", "audioResample");
    audioVolume = QGstElement("volume", "volume");
    audioSink = QGstElement("autoaudiosink", "autoAudioSink");
    gstAudioOutput.add(audioQueue, audioConvert, audioResample, audioVolume, audioSink);
    audioQueue.link(audioConvert, audioResample, audioVolume, audioSink);

    gstAudioOutput.addGhostPad(audioQueue, "sink");
}

QGstreamerAudioOutput::~QGstreamerAudioOutput()
{
}

int QGstreamerAudioOutput::volume() const
{
    return m_volume;
}

bool QGstreamerAudioOutput::isMuted() const
{
    return m_muted;
}

void QGstreamerAudioOutput::setVolume(int vol)
{
    if (vol == m_volume)
        return;
    m_volume = vol;
    audioVolume.set("volume", vol/100.);
    emit volumeChanged(m_volume);
}

void QGstreamerAudioOutput::setMuted(bool muted)
{
    if (muted == m_muted)
        return;
    m_muted = muted;
    audioVolume.set("mute", muted);
    emit mutedChanged(muted);
}

void QGstreamerAudioOutput::setPipeline(const QGstPipeline &pipeline)
{
    gstPipeline = pipeline;
}

bool QGstreamerAudioOutput::setAudioOutput(const QAudioDevice &info)
{
    if (info == m_audioOutput)
        return true;
    qCDebug(qLcMediaAudioOutput) << "setAudioOutput" << info.description() << info.isNull();
    m_audioOutput = info;

    auto state = gstPipeline.state();
    if (state != GST_STATE_PLAYING)
        return changeAudioOutput();

    auto pad = audioVolume.staticPad("src");
    pad.addProbe<&QGstreamerAudioOutput::prepareAudioOutputChange>(this, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM);

    return true;
}

bool QGstreamerAudioOutput::changeAudioOutput()
{
    qCDebug(qLcMediaAudioOutput) << "Changing audio output";
    QGstElement newSink;
    auto *deviceInfo = static_cast<const QGStreamerAudioDeviceInfo *>(m_audioOutput.handle());
    if (!deviceInfo)
        newSink = QGstElement("fakesink", "fakeaudiosink");
    else if (deviceInfo->gstDevice)
        newSink = gst_device_create_element(deviceInfo->gstDevice , "audiosink");

    if (newSink.isNull())
        newSink = QGstElement("autoaudiosink", "audiosink");

    gstAudioOutput.remove(audioSink);
    audioSink = newSink;
    gstAudioOutput.add(audioSink);
    audioVolume.link(audioSink);

    return true;
}

void QGstreamerAudioOutput::prepareAudioOutputChange(const QGstPad &/*pad*/)
{
    qCDebug(qLcMediaAudioOutput) << "Reconfiguring audio output";

    auto state = gstPipeline.state();
    if (state == GST_STATE_PLAYING)
        gstPipeline.setStateSync(GST_STATE_PAUSED);
    audioSink.setStateSync(GST_STATE_NULL);
    changeAudioOutput();
    audioSink.setStateSync(GST_STATE_PAUSED);
    if (state == GST_STATE_PLAYING)
        gstPipeline.setStateSync(state);
}

QAudioDevice QGstreamerAudioOutput::audioOutput() const
{
    return m_audioOutput;
}

QT_END_NAMESPACE
