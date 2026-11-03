// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgstreameraudiooutput_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qversionnumber.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudiooutput.h>

#include <audio/qgstreameraudiodevice_p.h>
#include <common/qgstpipeline_p.h>

#if QT_CONFIG(pulseaudio)
#  include <pulse/version.h>
#endif

QT_BEGIN_NAMESPACE

namespace {

Q_STATIC_LOGGING_CATEGORY(qLcMediaAudioOutput, "qt.multimedia.audiooutput")

[[maybe_unused]] bool sinkHasDeviceProperty(const QGstElement &element)
{
    using namespace Qt::Literals;
    QLatin1String elementType = element.typeName();

    // GstAlsaSink has a "device" property and GstPipeWireSink has "target-object", but they cannot
    // be changed during playback
    return elementType == "GstPulseSink"_L1;
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
          QGstElement::createFromFactory(qGstAudioElements().sink, "audiosink"),
      }
{
    using namespace Qt::Literals;
    if (m_audioSink.typeName() == "GstFakeSink"_L1)
        m_audioSink.set("sync", true); // Play in real time, as there is no audio clock

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

    QGstElement newSink = QGstElement::createFromFactory(qGstAudioElements().sink, "audiosink");
    if (newSink) {
        using namespace Qt::Literals;
        qGstSetAudioDevice(newSink, m_audioDevice);
        if (newSink.typeName() == "GstFakeSink"_L1)
            newSink.set("sync", true); // Play in real time, as there is no audio clock
        if (!m_sinkIsAsync)
            newSink.set("async", false);
        return newSink;
    }

    qWarning() << "Cannot create" << qGstAudioElements().sink << "element. Using autoaudiosink";
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
            qGstSetAudioDevice(m_audioSink, m_audioDevice);
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
