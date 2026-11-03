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

bool srcHasDeviceProperty(const QGstElement &element)
{
    using namespace Qt::Literals;
    QLatin1String elementType = element.typeName();

    // GstAlsaSrc has a "device" property and GstPipeWireSrc has "target-object", but they cannot
    // be changed during playback
    return elementType == "GstPulseSrc"_L1;
}

} // namespace

q23::expected<QPlatformAudioInput *, QString> QGstreamerAudioInput::create(QAudioInput *parent)
{
    static const auto error = qGstErrorMessageIfElementsNotAvailable("autoaudiosrc", "volume");
    if (error)
        return q23::unexpected{ *error };

    using namespace Qt::Literals;
    if (!qGstAudioElements().source)
        return q23::unexpected{ u"No GStreamer audio source is available"_s };

    return new QGstreamerAudioInput(parent);
}

QGstreamerAudioInput::QGstreamerAudioInput(QAudioInput *parent)
    : QObject(parent),
      QPlatformAudioInput(parent),
      m_audioInputBin(QGstBin::create("audioInput")),
      m_audioSrc{
          QGstElement::createFromFactory(qGstAudioElements().source, "audiosrc"),
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

    QGstElement newSrc = QGstElement::createFromFactory(qGstAudioElements().source, "audiosrc");
    if (newSrc) {
        qGstSetAudioDevice(newSrc, m_audioDevice);
        return newSrc;
    }

    qWarning() << "Cannot create" << qGstAudioElements().source << "element. Using autoaudiosrc";
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
        qGstSetAudioDevice(m_audioSrc, m_audioDevice);
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
