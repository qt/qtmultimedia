/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include <private/qgstreameraudiooutput_p.h>
#include <private/qgstreameraudiodevice_p.h>
#include <qaudiodevice.h>
#include <qaudiooutput.h>

#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

Q_LOGGING_CATEGORY(qLcMediaAudioOutput, "qt.multimedia.audiooutput")

QT_BEGIN_NAMESPACE

QGstreamerAudioOutput::QGstreamerAudioOutput(QAudioOutput *parent)
  : QObject(parent),
    QPlatformAudioOutput(parent),
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
    gstAudioOutput.setStateSync(GST_STATE_NULL);
}

void QGstreamerAudioOutput::setVolume(float vol)
{
    audioVolume.set("volume", vol);
}

void QGstreamerAudioOutput::setMuted(bool muted)
{
    audioVolume.set("mute", muted);
}

void QGstreamerAudioOutput::setPipeline(const QGstPipeline &pipeline)
{
    gstPipeline = pipeline;
}

void QGstreamerAudioOutput::setAudioDevice(const QAudioDevice &info)
{
    if (info == m_audioOutput)
        return;
    qCDebug(qLcMediaAudioOutput) << "setAudioOutput" << info.description() << info.isNull();
    m_audioOutput = info;

    QGstElement newSink;
    auto *deviceInfo = static_cast<const QGStreamerAudioDeviceInfo *>(m_audioOutput.handle());
    if (deviceInfo && deviceInfo->gstDevice)
        newSink = gst_device_create_element(deviceInfo->gstDevice , "audiosink");
    else
        qCWarning(qLcMediaAudioOutput) << "Invalid audio device";

    if (newSink.isNull()) {
        qCWarning(qLcMediaAudioOutput) << "Failed to create a gst element for the audio device, using a default audio sink";
        newSink = QGstElement("autoaudiosink", "audiosink");
    }

    audioVolume.staticPad("src").doInIdleProbe([&](){
        audioVolume.unlink(audioSink);
        gstAudioOutput.remove(audioSink);
        gstAudioOutput.add(newSink);
        newSink.syncStateWithParent();
        audioVolume.link(newSink);
    });

    audioSink.setStateSync(GST_STATE_NULL);
    audioSink = newSink;
}

QT_END_NAMESPACE
