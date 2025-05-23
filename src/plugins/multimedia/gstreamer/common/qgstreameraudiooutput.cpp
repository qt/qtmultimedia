// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgstreameraudiooutput_p.h>
#include <audio/qgstreameraudiodevice_p.h>

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudiooutput.h>
#include <QtCore/qloggingcategory.h>

#include <utility>

static Q_LOGGING_CATEGORY(qLcMediaAudioOutput, "qt.multimedia.audiooutput")

QT_BEGIN_NAMESPACE

QMaybe<QPlatformAudioOutput *> QGstreamerAudioOutput::create(QAudioOutput *parent)
{
    QGstElement audioconvert = QGstElement::createFromFactory("audioconvert", "audioConvert");
    if (!audioconvert)
        return errorMessageCannotFindElement("audioconvert");

    QGstElement audioresample = QGstElement::createFromFactory("audioresample", "audioResample");
    if (!audioresample)
        return errorMessageCannotFindElement("audioresample");

    QGstElement volume = QGstElement::createFromFactory("volume", "volume");
    if (!volume)
        return errorMessageCannotFindElement("volume");

    QGstElement autoaudiosink = QGstElement::createFromFactory("autoaudiosink", "autoAudioSink");
    if (!autoaudiosink)
        return errorMessageCannotFindElement("autoaudiosink");

    return new QGstreamerAudioOutput(audioconvert, audioresample, volume, autoaudiosink, parent);
}

QGstreamerAudioOutput::QGstreamerAudioOutput(QGstElement audioconvert, QGstElement audioresample,
                                             QGstElement volume, QGstElement autoaudiosink,
                                             QAudioOutput *parent)
    : QObject(parent),
      QPlatformAudioOutput(parent),
      gstAudioOutput(QGstBin::create("audioOutput")),
      audioConvert(std::move(audioconvert)),
      audioResample(std::move(audioresample)),
      audioVolume(std::move(volume)),
      audioSink(std::move(autoaudiosink))
{
    audioQueue = QGstElement::createFromFactory("queue", "audioQueue");
    gstAudioOutput.add(audioQueue, audioConvert, audioResample, audioVolume, audioSink);
    qLinkGstElements(audioQueue, audioConvert, audioResample, audioVolume, audioSink);

    gstAudioOutput.addGhostPad(audioQueue, "sink");
}

QGstreamerAudioOutput::~QGstreamerAudioOutput()
{
    gstAudioOutput.setStateSync(GST_STATE_NULL);
}

void QGstreamerAudioOutput::setVolume(float volume)
{
    audioVolume.set("volume", volume);
}

void QGstreamerAudioOutput::setMuted(bool muted)
{
    audioVolume.set("mute", muted);
}

void QGstreamerAudioOutput::setAudioDevice(const QAudioDevice &info)
{
    if (info == m_audioOutput)
        return;
    qCDebug(qLcMediaAudioOutput) << "setAudioOutput" << info.description() << info.isNull();
    m_audioOutput = info;

    QGstElement newSink;
    if constexpr (QT_CONFIG(pulseaudio)) {
        auto id = m_audioOutput.id();
        newSink = QGstElement::createFromFactory("pulsesink", "audiosink");
        if (!newSink.isNull())
            newSink.set("device", id.constData());
        else
            qCWarning(qLcMediaAudioOutput) << "Invalid audio device";
    } else {
        auto *deviceInfo = static_cast<const QGStreamerAudioDeviceInfo *>(m_audioOutput.handle());
        if (deviceInfo && deviceInfo->gstDevice)
            newSink = QGstElement::createFromDevice(deviceInfo->gstDevice, "audiosink");
        else
            qCWarning(qLcMediaAudioOutput) << "Invalid audio device";
    }

    if (newSink.isNull()) {
        qCWarning(qLcMediaAudioOutput) << "Failed to create a gst element for the audio device, using a default audio sink";
        newSink = QGstElement::createFromFactory("autoaudiosink", "audiosink");
    }

    QGstPipeline::modifyPipelineWhileNotRunning(gstAudioOutput.getPipeline(), [&] {
        qUnlinkGstElements(audioVolume, audioSink);
        gstAudioOutput.stopAndRemoveElements(audioSink);
        audioSink = std::move(newSink);
        gstAudioOutput.add(audioSink);
        audioSink.syncStateWithParent();
        qLinkGstElements(audioVolume, audioSink);
    });
}

QT_END_NAMESPACE
