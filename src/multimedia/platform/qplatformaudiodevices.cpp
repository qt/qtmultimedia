// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformaudiodevices_p.h"

#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtCore/private/qminimalflatset_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvarlengtharray.h>

#include <optional>

#if defined(Q_OS_ANDROID)
#  include <QtMultimedia/private/qandroidaudiodevices_p.h>
#endif
#if QT_CONFIG(ohaudio)
#  include <QtMultimedia/private/qohosaudiodevices_p.h>
#endif
#if defined(Q_OS_DARWIN)
#  include <QtMultimedia/private/qcoreaudiodevices_p.h>
#endif
#if defined(Q_OS_WINDOWS)
#  include <QtMultimedia/private/qwindowsaudiodevices_p.h>
#endif
#if QT_CONFIG(alsa)
#  include <QtMultimedia/private/qalsaaudiodevices_p.h>
#endif
#if QT_CONFIG(pulseaudio)
#  include <QtMultimedia/private/qpulseaudiodevices_p.h>
#  include <QtMultimedia/private/qpulsehelpers_p.h>
#endif
#if QT_CONFIG(pipewire)
#  include <QtMultimedia/private/qpipewire_audiodevices_p.h>
#endif
#if defined(Q_OS_QNX) && defined(QT_MM_QNX_IO_SND)
#  include <QtMultimedia/private/qqnxsndaudiodevices_p.h>
#endif
#if defined(Q_OS_QNX) && defined(QT_MM_QNX_IO_AUDIO)
#  include <QtMultimedia/private/qqnxaudiodevices_p.h>
#endif
#if defined(Q_OS_WASM)
#  include <QtMultimedia/private/qwasmmediadevices_p.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::Literals;

#if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(Q_OS_HARMONY))

namespace {

enum class AudioBackend : uint8_t {
    PipeWire,
    PulseAudio,
    ALSA,
    Unavailable,
};

std::optional<AudioBackend> requestedAudioBackend()
{
    const QByteArray requested = qgetenv("QT_AUDIO_BACKEND").toLower();
    if (requested.isEmpty())
        return std::nullopt;
#  if QT_CONFIG(pipewire)
    if (requested == "pipewire"_ba) {
        if (QtPipeWire::QAudioDevices::isSupported())
            return AudioBackend::PipeWire;
        qWarning() << "Audio backend pipewire requested, but not supported on this system."
                   << "Using default backend";
        return std::nullopt;
    }
#  endif
#  if QT_CONFIG(pulseaudio)
    if (requested == "pulseaudio"_ba)
        return AudioBackend::PulseAudio;
#  endif
#  if QT_CONFIG(alsa)
    if (requested == "alsa"_ba)
        return AudioBackend::ALSA;
#  endif
    qWarning() << "Audio backend" << requested << "requested, but not available."
               << "Using default backend";
    return std::nullopt;
}

// Some distros still ship PipeWire and the vanilla PulseAudio server (not PipeWire's PulseAudio
// server) side by side. In that case, PipeWire might not report any audio devices.
[[maybe_unused]] bool isVanillaPulseAudioServerRunning()
{
#  if QT_CONFIG(pulseaudio)
    using namespace QPulseAudioInternal;
    return pulseaudioDetectServerType() == PulseaudioServerType::Pulseaudio;
#  else
    return false;
#  endif
}

AudioBackend selectedAudioBackend()
{
    if (const std::optional<AudioBackend> requested = requestedAudioBackend())
        return *requested;

#  if QT_CONFIG(pipewire)
    if (QtPipeWire::QAudioDevices::isSupported() && !isVanillaPulseAudioServerRunning())
        return AudioBackend::PipeWire;
#  endif
#  if QT_CONFIG(pulseaudio)
    return AudioBackend::PulseAudio;
#  endif
#  if QT_CONFIG(alsa)
    return AudioBackend::ALSA;
#  endif
    return AudioBackend::Unavailable;
}

} // namespace

#endif

std::unique_ptr<QPlatformAudioDevices> QPlatformAudioDevices::create()
{
#ifdef Q_OS_DARWIN
    return std::make_unique<QCoreAudioDevices>();
#endif
#if defined(Q_OS_WINDOWS)
    return std::make_unique<QWindowsAudioDevices>();
#endif
#if defined(Q_OS_ANDROID)
    return std::make_unique<QAndroidAudioDevices>();
#endif
#if QT_CONFIG(ohaudio)
    return std::make_unique<QOhosAudioDevices>();
#endif
#if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(Q_OS_HARMONY))
    switch (selectedAudioBackend()) {
#  if QT_CONFIG(pipewire)
    case AudioBackend::PipeWire:
        return std::make_unique<QtPipeWire::QAudioDevices>();
#  endif
#  if QT_CONFIG(pulseaudio)
    case AudioBackend::PulseAudio:
        return std::make_unique<QPulseAudioDevices>();
#  endif
#  if QT_CONFIG(alsa)
    case AudioBackend::ALSA:
        return std::make_unique<QAlsaAudioDevices>();
#  endif
    default:
        break;
    }
#endif
#if defined(Q_OS_QNX) && defined(QT_MM_QNX_IO_SND)
    return std::make_unique<QQnxSndAudioDevices>();
#endif
#if defined(Q_OS_QNX) && defined(QT_MM_QNX_IO_AUDIO)
    return std::make_unique<QQnxAudioDevices>();
#endif
#if defined(Q_OS_WASM)
    return std::make_unique<QWasmAudioDevices>();
#endif
    qWarning() << "No audio backend available";
    return std::make_unique<QPlatformAudioDevices>();
}

QPlatformAudioDevices::QPlatformAudioDevices()
{
    qRegisterMetaType<PrivateTag>(); // for the case of queued connections
}

QPlatformAudioDevices::~QPlatformAudioDevices() = default;

QList<QAudioDevice> QPlatformAudioDevices::audioInputs() const
{
    return m_audioInputs.ensure([this]() {
        return findAudioInputs();
    });
}

QList<QAudioDevice> QPlatformAudioDevices::audioOutputs() const
{
    return m_audioOutputs.ensure([this]() {
        return findAudioOutputs();
    });
}

void QPlatformAudioDevices::onAudioInputsChanged()
{
    m_audioInputs.reset();
    Q_EMIT audioInputsChanged(PrivateTag{});
}

void QPlatformAudioDevices::onAudioOutputsChanged()
{
    m_audioOutputs.reset();
    Q_EMIT audioOutputsChanged(PrivateTag{});
}

void QPlatformAudioDevices::updateAudioInputsCache()
{
    if (m_audioInputs.update(findAudioInputs()))
        Q_EMIT audioInputsChanged(PrivateTag{});
}

void QPlatformAudioDevices::updateAudioOutputsCache()
{
    if (m_audioOutputs.update(findAudioOutputs()))
        Q_EMIT audioOutputsChanged(PrivateTag{});
}

QPlatformAudioSource *QPlatformAudioDevices::createAudioSource(const QAudioDevice &,
                                                               const QAudioFormat &, QObject *)
{
    return nullptr;
}
QPlatformAudioSink *QPlatformAudioDevices::createAudioSink(const QAudioDevice &,
                                                           const QAudioFormat &, QObject *)
{
    return nullptr;
}

QPlatformAudioSource *QPlatformAudioDevices::audioInputDevice(QAudioFormat format,
                                                              const QAudioDevice &deviceInfo,
                                                              QObject *parent)
{
    QAudioDevice device = deviceInfo;
    if (device.isNull())
        device = QMediaDevices::defaultAudioInput();

    if (device.isNull())
        return nullptr;

    if (format == QAudioFormat{})
        format = device.preferredFormat();

    return createAudioSource(device, format, parent);
}

QPlatformAudioSink *QPlatformAudioDevices::audioOutputDevice(QAudioFormat format,
                                                             const QAudioDevice &deviceInfo,
                                                             QObject *parent)
{
    QAudioDevice device = deviceInfo;
    if (device.isNull())
        device = QMediaDevices::defaultAudioOutput();

    if (device.isNull())
        return nullptr;

    if (format == QAudioFormat{})
        format = device.preferredFormat();

    return createAudioSink(device, format, parent);
}

bool QPlatformAudioDevices::hasCallbackApi() const
{
    return false;
}

QT_END_NAMESPACE

#include "moc_qplatformaudiodevices_p.cpp"
