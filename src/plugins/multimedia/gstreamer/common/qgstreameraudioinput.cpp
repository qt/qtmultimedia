// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qgstreameraudioinput_p.h>
#include <qgstreameraudiodevice_p.h>
#include <qaudiodevice.h>
#include <qaudioinput.h>

#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <utility>

static Q_LOGGING_CATEGORY(qLcMediaAudioInput, "qt.multimedia.audioInput")

QT_BEGIN_NAMESPACE

QMaybe<QPlatformAudioInput *> QGstreamerAudioInput::create(QAudioInput *parent)
{
    QGstElement autoaudiosrc("autoaudiosrc", "autoaudiosrc");
    if (!autoaudiosrc)
        return errorMessageCannotFindElement("autoaudiosrc");

    QGstElement volume("volume", "volume");
    if (!volume)
        return errorMessageCannotFindElement("volume");

    return new QGstreamerAudioInput(autoaudiosrc, volume, parent);
}

QGstreamerAudioInput::QGstreamerAudioInput(QGstElement autoaudiosrc, QGstElement volume,
                                           QAudioInput *parent)
    : QObject(parent),
      QPlatformAudioInput(parent),
      gstAudioInput("audioInput"),
      audioSrc(std::move(autoaudiosrc)),
      audioVolume(std::move(volume))
{
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
#if QT_CONFIG(pulseaudio)
    auto id = m_audioDevice.id();
    newSrc = QGstElement("pulsesrc", "audiosrc");
    if (!newSrc.isNull())
        newSrc.set("device", id.constData());
    else
        qCWarning(qLcMediaAudioInput) << "Invalid audio device";
#else
    auto *deviceInfo = static_cast<const QGStreamerAudioDeviceInfo *>(m_audioDevice.handle());
    if (deviceInfo && deviceInfo->gstDevice)
        newSrc = gst_device_create_element(deviceInfo->gstDevice, "audiosrc");
    else
        qCWarning(qLcMediaAudioInput) << "Invalid audio device";
#endif

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

#include "moc_qgstreameraudioinput_p.cpp"
