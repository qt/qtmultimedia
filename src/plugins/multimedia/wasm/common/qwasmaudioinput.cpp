// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR
// GPL-2.0-only OR GPL-3.0-only

#include "qwasmaudioinput_p.h"

#include <qaudioinput.h>
#include <private/qstdweb_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qWasmAudioInput, "qt.multimedia.wasm.audioinput")

QWasmAudioInput::QWasmAudioInput(QAudioInput *parent)
    : QObject(parent), QPlatformAudioInput(parent)
{
    m_wasMuted = false;
    setDeviceSourceStream("");
}

QWasmAudioInput::~QWasmAudioInput()
{
}

void QWasmAudioInput::setMuted(bool muted)
{
    qCDebug(qWasmAudioInput) << Q_FUNC_INFO << muted;
    if (muted == m_wasMuted)
        return;
    if (m_mediaStream.isNull() || m_mediaStream.isUndefined())
        return;
    emscripten::val audioTracks = m_mediaStream.call<emscripten::val>("getAudioTracks");
    if (audioTracks.isNull() || audioTracks.isUndefined())
        return;
    if (audioTracks["length"].as<int>() < 1)
        return;
    audioTracks[0].set("muted", muted);

    emit mutedChanged(muted);
    m_wasMuted = muted;

}

bool QWasmAudioInput::isMuted() const
{
    return m_wasMuted;
}

void QWasmAudioInput::setAudioDevice(const QAudioDevice &audioDevice)
{
    if (device == audioDevice)
        return;

    device = audioDevice;
    setDeviceSourceStream(device.id().toStdString());
}

void QWasmAudioInput::setVolume(float volume)
{
    Q_UNUSED(volume)
  // TODO seems no easy way to set input volume
}

void QWasmAudioInput::setDeviceSourceStream(const std::string &id)
{
    qCDebug(qWasmAudioInput) << Q_FUNC_INFO << id;
    emscripten::val navigator = emscripten::val::global("navigator");
    emscripten::val mediaDevices = navigator["mediaDevices"];

    if (mediaDevices.isNull() || mediaDevices.isUndefined()) {
        qWarning() << "No media devices found";
        return;
    }

    qstdweb::PromiseCallbacks getUserMediaCallback{
        // default
        .thenFunc =
                [this](emscripten::val stream) {
                    qCDebug(qWasmAudioInput) << "getUserMediaSuccess";
                    m_mediaStream = stream;
                },
        .catchFunc =
                [](emscripten::val error) {
                    qCDebug(qWasmAudioInput)
                            << "addCameraSourceElement getUserMedia  fail"
                            << QString::fromStdString(error["name"].as<std::string>())
                            << QString::fromStdString(error["message"].as<std::string>());
                }
    };

    emscripten::val constraints = emscripten::val::object();
    constraints.set("audio", true);
    if (!id.empty())
        constraints.set("deviceId", id);

    // we do it this way as this prompts user for mic permissions
    qstdweb::Promise::make(mediaDevices, QStringLiteral("getUserMedia"),
                           std::move(getUserMediaCallback), constraints);
}

emscripten::val QWasmAudioInput::mediaStream()
{
    return m_mediaStream;
}

QT_END_NAMESPACE

#include "moc_qwasmaudioinput_p.cpp"
