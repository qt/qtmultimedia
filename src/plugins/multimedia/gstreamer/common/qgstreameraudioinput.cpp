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
    static const auto error = qGstErrorMessageIfElementsNotAvailable("autoaudiosrc", "volume");
    if (error)
        return *error;

    return new QGstreamerAudioInput(parent);
}

QGstreamerAudioInput::QGstreamerAudioInput(QAudioInput *parent)
    : QObject(parent),
      QPlatformAudioInput(parent),
      gstAudioInput(QGstBin::create("audioInput")),
      audioSrc{
          QGstElement::createFromFactory("autoaudiosrc", "autoaudiosrc"),
      },
      audioVolume{
          QGstElement::createFromFactory("volume", "volume"),
      }
{
    gstAudioInput.add(audioSrc, audioVolume);
    qLinkGstElements(audioSrc, audioVolume);

    gstAudioInput.addGhostPad(audioVolume, "src");
}

QGstElement QGstreamerAudioInput::createGstElement()
{
    const auto *customDeviceInfo =
            dynamic_cast<const QGStreamerCustomAudioDeviceInfo *>(m_audioDevice.handle());

    if (customDeviceInfo) {
        qCDebug(qLcMediaAudioInput)
                << "requesting custom audio src element: " << customDeviceInfo->id;

        QGstElement element = QGstBin::createFromPipelineDescription(customDeviceInfo->id,
                                                                     /*name=*/nullptr,
                                                                     /*ghostUnlinkedPads=*/true);
        if (element)
            return element;

        qCWarning(qLcMediaAudioInput)
                << "Cannot create audio source element:" << customDeviceInfo->id;
    }

    const QByteArray &id = m_audioDevice.id();
    if constexpr (QT_CONFIG(pulseaudio)) {
        QGstElement newSrc = QGstElement::createFromFactory("pulsesrc", "audiosrc");
        if (newSrc) {
            newSrc.set("device", id.constData());
            return newSrc;
        } else {
            qWarning() << "Cannot create pulsesrc";
        }
    } else if constexpr (QT_CONFIG(alsa)) {
        QGstElement newSrc = QGstElement::createFromFactory("alsasrc", "audiosrc");
        if (newSrc) {
            newSrc.set("device", id.constData());
            return newSrc;
        } else {
            qWarning() << "Cannot create alsasrc";
        }
    } else {
        auto *deviceInfo = dynamic_cast<const QGStreamerAudioDeviceInfo *>(m_audioDevice.handle());
        if (deviceInfo && deviceInfo->gstDevice) {
            QGstElement element = QGstElement::createFromDevice(deviceInfo->gstDevice, "audiosrc");
            if (element)
                return element;
        }
    }
    qCWarning(qLcMediaAudioInput) << "Invalid audio device";
    qCWarning(qLcMediaAudioInput)
            << "Failed to create a gst element for the audio device, using a default audio source";
    return QGstElement::createFromFactory("autoaudiosrc", "audiosrc");
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

    QGstElement newSrc = createGstElement();

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
