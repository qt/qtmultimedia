// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmmediadevices_p.h"
#include "private/qcameradevice_p.h"

#include "qwasmaudiosource_p.h"
#include "qwasmaudiosink_p.h"
#include "qwasmaudiodevice_p.h"
#include <AL/al.h>
#include <AL/alc.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

QWasmMediaDevices::QWasmMediaDevices()
    : QPlatformMediaDevices()
{
    getMediaDevices(); // asynchronous
}

QList<QAudioDevice> QWasmMediaDevices::audioInputs() const
{
    return m_ins;
}

QList<QAudioDevice> QWasmMediaDevices::audioOutputs() const
{
    return m_outs;
}

QPlatformAudioSource *QWasmMediaDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                           QObject *parent)
{
    return new QWasmAudioSource(deviceInfo.id(), parent);
}

QPlatformAudioSink *QWasmMediaDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                       QObject *parent)
{
    return new QWasmAudioSink(deviceInfo.id(), parent);
}

void QWasmMediaDevices::getMediaDevices()
{
    emscripten::val navigator = emscripten::val::global("navigator");
    emscripten::val mediaDevices = navigator["mediaDevices"];

    if (mediaDevices.isNull() || mediaDevices.isUndefined()) {
        qWarning() << "No media devices found";
        return;
    }

    qstdweb::PromiseCallbacks getUserMediaCallback{
        .thenFunc =
                [this, navigator, mediaDevices](emscripten::val video) {
            Q_UNUSED(video);
            qstdweb::PromiseCallbacks enumerateDevicesCallback{
                .thenFunc =
                        [&](emscripten::val devices) {

                    std::string defaultDeviceLabel;
                    for (int i = 0; i < devices["length"].as<int>(); i++) {
                        const std::string deviceKind =
                                devices[i]["kind"].as<std::string>();
                        const std::string label =
                                devices[i]["label"].as<std::string>();
                        const std::string deviceId =
                                devices[i]["deviceId"].as<std::string>();

                        if (deviceId == std::string("default")) {
                            defaultDeviceLabel = label.substr(10);
                            continue;
                        }

                        const bool isDefault =
                                (defaultDeviceLabel.compare(label) == 0);

                        if (deviceKind == std::string("videoinput")) {
                            QScopedPointer<QCameraDevicePrivate> camera(
                                        new QCameraDevicePrivate);
                            camera->id = QString::fromStdString(deviceId).toUtf8();
                            camera->description = QString::fromUtf8(label.c_str());
                            camera->isDefault = isDefault;
                            m_cameraDevices.append(camera->create());
                            emit videoInputsChanged();

                        } else if (deviceKind == std::string("audioinput")) {

                            m_ins.append((new QWasmAudioDevice(
                                              deviceId.c_str(), label.c_str(),
                                              isDefault, QAudioDevice::Input))
                                         ->create());
                            emit audioInputsChanged();
                        }
                        // if permissions are given label will hold the actual
                        // camera name, such as "Live! Cam Sync 1080p (041e:409d)"
                    }
                    getAlAudioDevices();
                },
                .catchFunc =
                        [](emscripten::val error) {
                    qWarning() << "enumerateDevices fail"
                               << QString::fromStdString(
                                      error["name"].as<std::string>())
                            << QString::fromStdString(
                                   error["message"].as<std::string>());
                }

            };

            qstdweb::Promise::make(mediaDevices, QStringLiteral("enumerateDevices"),
                                   std::move(enumerateDevicesCallback));

        },
                .catchFunc =
                [](emscripten::val error) {
            qWarning() << "getUserMedia fail"
                       << QString::fromStdString(error["name"].as<std::string>())
                    << QString::fromStdString(error["message"].as<std::string>());
        }
    };

    emscripten::val constraints = emscripten::val::object();
    constraints.set("video", true);
    constraints.set("audio", true);

    // getUserMedia will permissions user permissions
    qstdweb::Promise::make(mediaDevices, QStringLiteral("getUserMedia"),
                           std::move(getUserMediaCallback), constraints);
}

void QWasmMediaDevices::getAlAudioDevices()
{
    // VM3959:4 The AudioContext was not allowed to start.
    // It must be resumed (or created) after a user gesture on the page. https://goo.gl/7K7WLu
    auto capture = alcGetString(nullptr, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
    // present even if there is no capture device
    if (capture) {
        m_ins.append((new QWasmAudioDevice(capture, "WebAssembly audio capture device", true,
                                           QAudioDevice::Input))->create());
    }

    auto playback = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    // present even if there is no playback device
    if (playback)
        m_outs.append((new QWasmAudioDevice(playback, "WebAssembly audio playback device", true,
                                            QAudioDevice::Output))->create());
}

QT_END_NAMESPACE
