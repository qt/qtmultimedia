// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgstreameraudiooutput_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudiooutput.h>

#include <common/qgstpipeline_p.h>
#include <audio/qgstreameraudiodevice_p.h>


QT_BEGIN_NAMESPACE

namespace {

Q_LOGGING_CATEGORY(qLcMediaAudioOutput, "qt.multimedia.audiooutput")

constexpr QLatin1String defaultSinkName = [] {
    using namespace Qt::Literals;

    if constexpr (QT_CONFIG(pulseaudio))
        return "pulsesink"_L1;
    else if constexpr (QT_CONFIG(alsa))
        return "alsasink"_L1;
    else
        return "autoaudiosink"_L1;
}();

bool sinkHasDeviceProperty(const QGstElement &element)
{
    using namespace Qt::Literals;
    QLatin1String elementType = element.typeName();

    if constexpr (QT_CONFIG(pulseaudio))
        return elementType == "GstPulseSink"_L1;
    if constexpr (0 && QT_CONFIG(alsa)) // alsasrc has a "device" property, but it cannot be changed
                                        // during playback
        return elementType == "GstAlsaSink"_L1;

    return false;
}

} // namespace

QMaybe<QPlatformAudioOutput *> QGstreamerAudioOutput::create(QAudioOutput *parent)
{
    static const auto error = qGstErrorMessageIfElementsNotAvailable(
            "audioconvert", "audioresample", "volume", "autoaudiosink");
    if (error)
        return *error;

    return new QGstreamerAudioOutput(parent);
}

QGstreamerAudioOutput::QGstreamerAudioOutput(QAudioOutput *parent)
    : QObject(parent),
      QPlatformAudioOutput(parent),
      m_audioOutputBin(QGstBin::create("audioOutput")),
      m_audioQueue{
          QGstElement::createFromFactory("queue", "audioQueue"),
      },
      m_audioConvert{
          QGstElement::createFromFactory("audioconvert", "audioConvert"),
      },
      m_audioResample{
          QGstElement::createFromFactory("audioresample", "audioResample"),
      },
      m_audioVolume{
          QGstElement::createFromFactory("volume", "volume"),
      },
      m_audioSink{
          QGstElement::createFromFactory(defaultSinkName.constData(), "audiosink"),
      }
{
    m_audioOutputBin.add(m_audioQueue, m_audioConvert, m_audioResample, m_audioVolume, m_audioSink);
    qLinkGstElements(m_audioQueue, m_audioConvert, m_audioResample, m_audioVolume, m_audioSink);

    m_audioOutputBin.addGhostPad(m_audioQueue, "sink");
}

QGstElement QGstreamerAudioOutput::createGstElement()
{
    const auto *customDeviceInfo =
            dynamic_cast<const QGStreamerCustomAudioDeviceInfo *>(m_audioDevice.handle());

    if (customDeviceInfo) {
        qCDebug(qLcMediaAudioOutput)
                << "requesting custom audio sink element: " << customDeviceInfo->id;

        QGstElement element =
                QGstBin::createFromPipelineDescription(customDeviceInfo->id, /*name=*/nullptr,
                                                       /*ghostUnlinkedPads=*/true);
        if (element)
            return element;

        qCWarning(qLcMediaAudioOutput)
                << "Cannot create audio sink element:" << customDeviceInfo->id;
    }

    const QByteArray &id = m_audioDevice.id();
    if constexpr (QT_CONFIG(pulseaudio) || QT_CONFIG(alsa)) {
        QGstElement newSink =
                QGstElement::createFromFactory(defaultSinkName.constData(), "audiosink");
        if (newSink) {
            newSink.set("device", id.constData());
            return newSink;
        }

        qWarning() << "Cannot create" << defaultSinkName;
    } else {
        auto *deviceInfo = dynamic_cast<const QGStreamerAudioDeviceInfo *>(m_audioDevice.handle());
        if (deviceInfo && deviceInfo->gstDevice) {
            QGstElement element = QGstElement::createFromDevice(deviceInfo->gstDevice, "audiosink");
            if (element)
                return element;
        }
    }
    qCWarning(qLcMediaAudioOutput) << "Invalid audio device:" << m_audioDevice.id();
    qCWarning(qLcMediaAudioOutput)
            << "Failed to create a gst element for the audio device, using a default audio sink";
    return QGstElement::createFromFactory("autoaudiosink", "audiosink");
}

QGstreamerAudioOutput::~QGstreamerAudioOutput()
{
    m_audioOutputBin.setStateSync(GST_STATE_NULL);
}

void QGstreamerAudioOutput::setVolume(float volume)
{
    m_audioVolume.set("volume", volume);
}

void QGstreamerAudioOutput::setMuted(bool muted)
{
    m_audioVolume.set("mute", muted);
}

void QGstreamerAudioOutput::setAudioDevice(const QAudioDevice &device)
{
    if (device == m_audioDevice)
        return;
    qCDebug(qLcMediaAudioOutput) << "setAudioDevice" << device.description() << device.isNull();

    m_audioDevice = device;

    if (sinkHasDeviceProperty(m_audioSink) && !isCustomAudioDevice(m_audioDevice)) {
        m_audioSink.set("device", m_audioDevice.id().constData());
        return;
    }

    QGstElement newSink = createGstElement();
    newSink.set("async", false); // no async state changes

    m_audioVolume.src().modifyPipelineInIdleProbe([&] {
        qUnlinkGstElements(m_audioVolume, m_audioSink);
        m_audioOutputBin.stopAndRemoveElements(m_audioSink);
        m_audioSink = std::move(newSink);
        m_audioOutputBin.add(m_audioSink);
        m_audioSink.syncStateWithParent();
        qLinkGstElements(m_audioVolume, m_audioSink);
    });
}

QT_END_NAMESPACE
