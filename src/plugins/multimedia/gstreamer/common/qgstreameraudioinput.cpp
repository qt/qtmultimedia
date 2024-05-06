// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioinput.h>

#include <QtCore/qloggingcategory.h>

#include <audio/qgstreameraudiodevice_p.h>
#include <common/qgstreameraudioinput_p.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <utility>

static Q_LOGGING_CATEGORY(qLcMediaAudioInput, "qt.multimedia.audioInput")

QT_BEGIN_NAMESPACE

QMaybe<QPlatformAudioInput *> QGstreamerAudioInput::create(QAudioInput *parent)
{
    QGstElement autoaudiosrc = QGstElement::createFromFactory("autoaudiosrc", "autoaudiosrc");
    if (!autoaudiosrc)
        return errorMessageCannotFindElement("autoaudiosrc");

    QGstElement volume = QGstElement::createFromFactory("volume", "volume");
    if (!volume)
        return errorMessageCannotFindElement("volume");

    return new QGstreamerAudioInput(autoaudiosrc, volume, parent);
}

QGstreamerAudioInput::QGstreamerAudioInput(QGstElement autoaudiosrc, QGstElement volume,
                                           QAudioInput *parent)
    : QObject(parent),
      QPlatformAudioInput(parent),
      gstAudioInput(QGstBin::create("audioInput")),
      audioSrc(std::move(autoaudiosrc)),
      audioVolume(std::move(volume))
{
    gstAudioInput.add(audioSrc, audioVolume);
    qLinkGstElements(audioSrc, audioVolume);

    gstAudioInput.addGhostPad(audioVolume, "src");
}

QGstreamerAudioInput::~QGstreamerAudioInput()
{
    gstAudioInput.setStateSync(GST_STATE_NULL);
}

void QGstreamerAudioInput::setVolume(float volume)
{
    audioVolume.set("volume", volume);
}

void QGstreamerAudioInput::setMuted(bool muted)
{
    audioVolume.set("mute", muted);
}

void QGstreamerAudioInput::setAudioDevice(const QAudioDevice &device)
{
    if (device == m_audioDevice)
        return;
    qCDebug(qLcMediaAudioInput) << "setAudioInput" << device.description() << device.isNull();
    m_audioDevice = device;
    const QByteArray &id = m_audioDevice.id();

    QGstElement newSrc;
    if constexpr (QT_CONFIG(pulseaudio)) {
        newSrc = QGstElement::createFromFactory("pulsesrc", "audiosrc");
        if (newSrc)
            newSrc.set("device", id.constData());
        else
            qWarning() << "Cannot create pulsesrc";
    } else if constexpr (QT_CONFIG(alsa)) {
        newSrc = QGstElement::createFromFactory("alsasrc", "audiosrc");
        if (newSrc)
            newSrc.set("device", id.constData());
        else
            qWarning() << "Cannot create alsasrc";
    } else {
        auto *gstDeviceInfo =
                dynamic_cast<const QGStreamerAudioDeviceInfo *>(m_audioDevice.handle());
        if (gstDeviceInfo && gstDeviceInfo->gstDevice) {
            newSrc = QGstElement::createFromDevice(gstDeviceInfo->gstDevice, "audiosrc");
        } else {
            qWarning() << "Invalid audio device";
        }
    }

    if (newSrc.isNull()) {
        qWarning() << "Failed to create a gst element for the audio device, using a default audio "
                      "source";
        newSrc = QGstElement::createFromFactory("autoaudiosrc", "audiosrc");
    }

    QGstPipeline::modifyPipelineWhileNotRunning(gstAudioInput.getPipeline(), [&] {
        qUnlinkGstElements(audioSrc, audioVolume);
        gstAudioInput.stopAndRemoveElements(audioSrc);
        audioSrc = std::move(newSrc);
        gstAudioInput.add(audioSrc);
        qLinkGstElements(audioSrc, audioVolume);
        audioSrc.syncStateWithParent();
    });
}

QAudioDevice QGstreamerAudioInput::audioInput() const
{
    return m_audioDevice;
}

QT_END_NAMESPACE
