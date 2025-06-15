// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgstreameraudiooutput_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qversionnumber.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudiooutput.h>

#include <common/qgstpipeline_p.h>
#include <audio/qgstreameraudiodevice_p.h>

#if QT_CONFIG(pulseaudio)
#  include <pulse/version.h>
#endif

QT_BEGIN_NAMESPACE

namespace {

Q_STATIC_LOGGING_CATEGORY(qLcMediaAudioOutput, "qt.multimedia.audiooutput")

constexpr QLatin1String defaultSinkName = [] {
    using namespace Qt::Literals;

    if constexpr (QT_CONFIG(pulseaudio))
        return "pulsesink"_L1;
    else if constexpr (QT_CONFIG(alsa))
        return "alsasink"_L1;
    else
        return "autoaudiosink"_L1;
}();

[[maybe_unused]] bool sinkHasDeviceProperty(const QGstElement &element)
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

void pulseVersionSanityCheck()
{
#if QT_CONFIG(pulseaudio)
    static std::once_flag versionCheckGuard;

    std::call_once(versionCheckGuard, [] {
        QVersionNumber paVersion = QVersionNumber::fromString(pa_get_library_version());
        QVersionNumber firstBadVersion(15, 99);
        QVersionNumber firstGoodVersion(16, 2);
        if (paVersion >= firstBadVersion && paVersion < firstGoodVersion) {
            qWarning() << "Pulseaudio v16 detected. It has known issues, that can cause GStreamer "
                          "to freeze.";
            // Note: gstreamer requires these two patches to work correctly:
            // https://gitlab.freedesktop.org/pulseaudio/pulseaudio/-/merge_requests/745
            // https://gitlab.freedesktop.org/pulseaudio/pulseaudio/-/merge_requests/764
        }
    });
#endif
}

} // namespace

q23::expected<QPlatformAudioOutput *, QString> QGstreamerAudioOutput::create(QAudioOutput *parent)
{
    static const auto error = qGstErrorMessageIfElementsNotAvailable(
            "audioconvert", "audioresample", "volume", "autoaudiosink");
    if (error)
        return q23::unexpected{ *error };

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

    pulseVersionSanityCheck();
}

QGstElement QGstreamerAudioOutput::createGstElement()
{
    const auto *customDevice =
            QAudioDevicePrivate::handle<QGStreamerCustomAudioDeviceInfo>(m_audioDevice);

    if (customDevice) {
        qCDebug(qLcMediaAudioOutput)
                << "requesting custom audio sink element: " << customDevice->id;

        QGstElement element =
                QGstBin::createFromPipelineDescription(customDevice->id, /*name=*/nullptr,
                                                       /*ghostUnlinkedPads=*/true);
        if (element)
            return element;

        qCWarning(qLcMediaAudioOutput) << "Cannot create audio sink element:" << customDevice->id;
    }

    const QByteArray &id = m_audioDevice.id();
    if constexpr (QT_CONFIG(pulseaudio) || QT_CONFIG(alsa)) {
        QGstElement newSink =
                QGstElement::createFromFactory(defaultSinkName.constData(), "audiosink");
        if (newSink) {
            newSink.set("device", id.constData());
            if (!m_sinkIsAsync)
                newSink.set("async", false);
            return newSink;
        }

        qWarning() << "Cannot create" << defaultSinkName;
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

void QGstreamerAudioOutput::setAsync(bool isAsync)
{
    m_sinkIsAsync = isAsync;
    if (m_audioSink)
        m_audioSink.set("async", m_sinkIsAsync);
}

void QGstreamerAudioOutput::setAudioDevice(const QAudioDevice &device)
{
    if (device == m_audioDevice)
        return;
    qCDebug(qLcMediaAudioOutput) << "setAudioDevice" << device.description() << device.isNull();

    m_audioDevice = device;

    // NOTE: ideally we could set the `device` property on the pulsesink. however that seems to
    // cause the pipeline to stall in rare occassions. so we need to force the creation of a new
    // sink
    constexpr bool forceNewSinkCreation = true;
    if constexpr (!forceNewSinkCreation) {
        if (sinkHasDeviceProperty(m_audioSink) && !isCustomAudioDevice(m_audioDevice)) {
            m_audioSink.set("device", m_audioDevice.id().constData());
            return;
        }
    }

    QGstElement newSink = createGstElement();

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
