// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreameraudiodevice_p.h"

#include <common/qgst_p.h>
#include <common/qgstutils_p.h>
#include <private/qplatformmediaintegration_p.h>

#if QT_CONFIG(pulseaudio)
#  include <QtMultimedia/private/qpulsehelpers_p.h>
#endif

#include <optional>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals;

QGStreamerCustomAudioDeviceInfo::QGStreamerCustomAudioDeviceInfo(const QByteArray &gstreamerPipeline,
                                                                 QAudioDevice::Mode mode)
    : QAudioDevicePrivate{
          gstreamerPipeline,
          mode,
          QString::fromUtf8(gstreamerPipeline),
          false,
          std::future<QAudioDevicePrivate::AudioDeviceFormat>{}  // invalid future - format not predetermined
      }
{
}

QAudioDevice qMakeCustomGStreamerAudioInput(const QByteArray &gstreamerPipeline)
{
    auto deviceInfo = std::make_unique<QGStreamerCustomAudioDeviceInfo>(gstreamerPipeline,
                                                                        QAudioDevice::Mode::Input);

    return QAudioDevicePrivate::createQAudioDevice(std::move(deviceInfo));
}

QAudioDevice qMakeCustomGStreamerAudioOutput(const QByteArray &gstreamerPipeline)
{
    auto deviceInfo = std::make_unique<QGStreamerCustomAudioDeviceInfo>(gstreamerPipeline,
                                                                        QAudioDevice::Mode::Output);

    return QAudioDevicePrivate::createQAudioDevice(std::move(deviceInfo));
}

bool isCustomAudioDevice(const QAudioDevicePrivate *device)
{
    return dynamic_cast<const QGStreamerCustomAudioDeviceInfo *>(device);
}

bool isCustomAudioDevice(const QAudioDevice &device)
{
    return isCustomAudioDevice(QAudioDevicePrivate::handle(device));
}

namespace {

constexpr QGstAudioElements pipeWireElements{ "pipewiresrc", "pipewiresink" };
constexpr QGstAudioElements pulseElements{ "pulsesrc", "pulsesink" };
constexpr QGstAudioElements alsaElements{ "alsasrc", "alsasink" };
constexpr QGstAudioElements autoElements{ "autoaudiosrc", "autoaudiosink" };
constexpr QGstAudioElements noElements{ nullptr, "fakesink" };

bool isPulseAudioServerReachable()
{
#if QT_CONFIG(pulseaudio)
    using namespace QPulseAudioInternal;
    return pulseaudioDetectServerType() != PulseaudioServerType::NotConnected;
#else
    return false; // Cannot check without libpulse
#endif
}

std::optional<QGstAudioElements> requestedAudioElements(const QByteArray &requested)
{
    if (requested == "pipewire"_ba)
        return pipeWireElements;
    if (requested == "pulseaudio"_ba)
        return pulseElements;
    if (requested == "alsa"_ba)
        return alsaElements;

    qWarning() << "Unknown QT_GSTREAMER_AUDIO_ELEMENTS value" << requested;
    return std::nullopt;
}

std::optional<QGstAudioElements> pipeWireAudioElements()
{
    // pipewiresrc and pipewiresink are only used if requested with
    // QT_GSTREAMER_AUDIO_ELEMENTS=pipewire, because they cannot switch devices: a running stream
    // cannot be moved to another device, and replacing a running pipewiresink can deadlock
    // (https://gitlab.freedesktop.org/pipewire/pipewire/-/issues/5171). Consider making them the
    // default once that is fixed.

    if (!isPulseAudioServerReachable()) {
        qWarning() << "No PulseAudio server is reachable for the GStreamer audio elements. Set "
                      "QT_GSTREAMER_AUDIO_ELEMENTS=pipewire to use PipeWire directly.";
        return std::nullopt;
    }
    return pulseElements;
}

std::optional<QGstAudioElements> backendAudioElements()
{
    const QLatin1StringView backend = QPlatformMediaIntegration::audioBackendName();
    if (backend == "PipeWire"_L1)
        return pipeWireAudioElements();
    if (backend == "PulseAudio"_L1)
        return pulseElements;
    if (backend == "ALSA"_L1)
        return alsaElements;
    return autoElements;
}

QGstAudioElements selectAudioElements()
{
    const QByteArray requested = qgetenv("QT_GSTREAMER_AUDIO_ELEMENTS").toLower();
    std::optional<QGstAudioElements> elements =
            requested.isEmpty() ? backendAudioElements() : requestedAudioElements(requested);

    if (elements) {
        if (auto error = qGstErrorMessageIfElementsNotAvailable(elements->source, elements->sink)) {
            qWarning().noquote() << *error;
            elements = std::nullopt;
        }
    }
    if (!elements) {
        qWarning() << "Audio output is discarded and audio input is not available";
        return noElements;
    }
    return *elements;
}

} // namespace

const QGstAudioElements &qGstAudioElements()
{
    static const QGstAudioElements elements = selectAudioElements();
    return elements;
}

void qGstSetAudioDevice(QGstElement &element, const QAudioDevice &device)
{
    if (device.isNull())
        return;

    const QByteArray id = device.id();

    const QLatin1StringView type = element.typeName();
    if (type == "GstPipeWireSrc"_L1 || type == "GstPipeWireSink"_L1) {
        // target-object is only read when the stream connects, so it selects the device of a
        // newly created element, but cannot move a running stream to another device.
        element.set("target-object", id.constData());
    } else if (type == "GstPulseSrc"_L1 || type == "GstPulseSink"_L1 || type == "GstAlsaSrc"_L1
               || type == "GstAlsaSink"_L1) {
        element.set("device", id.constData());
    }
}

QT_END_NAMESPACE
