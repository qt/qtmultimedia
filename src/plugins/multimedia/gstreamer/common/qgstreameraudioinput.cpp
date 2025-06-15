// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgstreameraudioinput_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioinput.h>

#include <audio/qgstreameraudiodevice_p.h>
#include <common/qgstpipeline_p.h>


QT_BEGIN_NAMESPACE

namespace {

Q_STATIC_LOGGING_CATEGORY(qLcMediaAudioInput, "qt.multimedia.audioinput")

constexpr QLatin1String defaultSrcName = [] {
    using namespace Qt::Literals;

    if constexpr (QT_CONFIG(pulseaudio))
        return "pulsesrc"_L1;
    else if constexpr (QT_CONFIG(alsa))
        return "alsasrc"_L1;
    else
        return "autoaudiosrc"_L1;
}();

bool srcHasDeviceProperty(const QGstElement &element)
{
    using namespace Qt::Literals;
    QLatin1String elementType = element.typeName();

    if constexpr (QT_CONFIG(pulseaudio))
        return elementType == "GstPulseSrc"_L1;

    if constexpr (0 && QT_CONFIG(alsa)) // alsasrc has a "device" property, but it cannot be changed
                                        // during playback
        return elementType == "GstAlsaSrc"_L1;

    return false;
}

} // namespace

q23::expected<QPlatformAudioInput *, QString> QGstreamerAudioInput::create(QAudioInput *parent)
{
    static const auto error = qGstErrorMessageIfElementsNotAvailable("autoaudiosrc", "volume");
    if (error)
        return q23::unexpected{ *error };

    return new QGstreamerAudioInput(parent);
}

QGstreamerAudioInput::QGstreamerAudioInput(QAudioInput *parent)
    : QObject(parent),
      QPlatformAudioInput(parent),
      m_audioInputBin(QGstBin::create("audioInput")),
      m_audioSrc{
          QGstElement::createFromFactory(defaultSrcName.constData(), "autoaudiosrc"),
      },
      m_audioVolume{
          QGstElement::createFromFactory("volume", "volume"),
      }
{
    m_audioInputBin.add(m_audioSrc, m_audioVolume);
    qLinkGstElements(m_audioSrc, m_audioVolume);

    m_audioInputBin.addGhostPad(m_audioVolume, "src");
}

QGstElement QGstreamerAudioInput::createGstElement()
{
    const auto *customDevice =
            QAudioDevicePrivate::handle<QGStreamerCustomAudioDeviceInfo>(m_audioDevice);

    if (customDevice) {
        qCDebug(qLcMediaAudioInput) << "requesting custom audio src element: " << customDevice->id;

        QGstElement element = QGstBin::createFromPipelineDescription(customDevice->id,
                                                                     /*name=*/nullptr,
                                                                     /*ghostUnlinkedPads=*/true);
        if (element)
            return element;

        qCWarning(qLcMediaAudioInput) << "Cannot create audio source element:" << customDevice->id;
    }

    const QByteArray &id = m_audioDevice.id();
    if constexpr (QT_CONFIG(pulseaudio) || QT_CONFIG(alsa)) {
        QGstElement newSrc = QGstElement::createFromFactory(defaultSrcName.constData(), "audiosrc");
        if (newSrc) {
            newSrc.set("device", id.constData());
            return newSrc;
        }

        qWarning() << "Cannot create" << defaultSrcName;
    }
    qCWarning(qLcMediaAudioInput) << "Invalid audio device";
    qCWarning(qLcMediaAudioInput)
            << "Failed to create a gst element for the audio device, using a default audio source";
    return QGstElement::createFromFactory("autoaudiosrc", "audiosrc");
}

QGstreamerAudioInput::~QGstreamerAudioInput()
{
    m_audioInputBin.setStateSync(GST_STATE_NULL);
}

void QGstreamerAudioInput::setVolume(float volume)
{
    m_audioVolume.set("volume", volume);
}

void QGstreamerAudioInput::setMuted(bool muted)
{
    m_audioVolume.set("mute", muted);
}

void QGstreamerAudioInput::setAudioDevice(const QAudioDevice &device)
{
    if (device == m_audioDevice)
        return;
    qCDebug(qLcMediaAudioInput) << "setAudioDevice" << device.description() << device.isNull();
    m_audioDevice = device;

    if (srcHasDeviceProperty(m_audioSrc) && !isCustomAudioDevice(m_audioDevice)) {
        m_audioSrc.set("device", m_audioDevice.id().constData());
        return;
    }

    QGstElement newSrc = createGstElement();

    m_audioVolume.sink().modifyPipelineInIdleProbe([&] {
        qUnlinkGstElements(m_audioSrc, m_audioVolume);
        m_audioInputBin.stopAndRemoveElements(m_audioSrc);
        m_audioSrc = std::move(newSrc);
        m_audioInputBin.add(m_audioSrc);
        qLinkGstElements(m_audioSrc, m_audioVolume);
        m_audioSrc.syncStateWithParent();
    });
}

QT_END_NAMESPACE
