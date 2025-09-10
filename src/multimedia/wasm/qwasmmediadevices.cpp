// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmmediadevices_p.h"
#include "private/qcameradevice_p.h"
#include "private/qplatformmediaintegration_p.h"
#include "qwasmaudiosource_p.h"
#include "qwasmaudiosink_p.h"
#include "qwasmaudiodevice_p.h"
#include <AL/al.h>
#include <AL/alc.h>

#include <QMap>
#include <QDebug>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qWasmMediaDevices, "qt.multimedia.wasm.mediadevices")

QWasmCameraDevices::QWasmCameraDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration),
      m_mediaDevices(static_cast<QWasmMediaDevices *>(integration->audioDevices()))
{
}

QList<QCameraDevice> QWasmCameraDevices::findVideoInputs() const
{
    return m_mediaDevices ? m_mediaDevices->videoInputs() : QList<QCameraDevice>();
}

QWasmMediaDevices::QWasmMediaDevices()
{
    initDevices();
}

void QWasmMediaDevices::initDevices()
{
    if (m_initDone)
        return;

    m_initDone = true;
    getOpenALAudioDevices();
    getMediaDevices(); // asynchronous
}

QList<QAudioDevice> QWasmMediaDevices::findAudioInputs() const
{
    return m_audioInputs.values();
}

QList<QAudioDevice> QWasmMediaDevices::findAudioOutputs() const
{
    return m_audioOutputs.values();
}

QList<QCameraDevice> QWasmMediaDevices::videoInputs() const
{
    return m_cameraDevices.values();
}

QPlatformAudioSource *QWasmMediaDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                           const QAudioFormat &fmt,
                                                           QObject *parent)
{
    return new QWasmAudioSource(deviceInfo, fmt, parent);
}

QPlatformAudioSink *QWasmMediaDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                       const QAudioFormat &fmt,
                                                       QObject *parent)
{
    return new QWasmAudioSink(deviceInfo, fmt, parent);
}

void QWasmMediaDevices::parseDevices(emscripten::val devices)
{
    if (devices.isNull() || devices.isUndefined()) {
        qWarning() << "Something went wrong enumerating devices";
        return;
    }

    QList<std::string> cameraDevicesToRemove = m_cameraDevices.keys();
    QList<std::string> audioOutputsToRemove;
    QList<std::string> audioInputsToRemove;

    if (m_firstInit) {
        m_firstInit = false;
        qWarning() << "m_audioInputs count" << m_audioInputs.count();

    } else {
        audioOutputsToRemove = m_audioOutputs.keys();
        audioInputsToRemove = m_audioInputs.keys();
        m_audioInputsAdded = false;
        m_audioOutputsAdded = false;
    }
    m_videoInputsAdded = false;

    bool m_videoInputsRemoved = false;
    bool m_audioInputsRemoved = false;
    bool m_audioOutputsRemoved = false;

    for (int i = 0; i < devices["length"].as<int>(); i++) {

        emscripten::val mediaDevice = devices[i];

        const std::string deviceKind = mediaDevice["kind"].as<std::string>();
        const std::string label = mediaDevice["label"].as<std::string>();
        const std::string deviceId = mediaDevice["deviceId"].as<std::string>();

        qCDebug(qWasmMediaDevices) << QString::fromStdString(deviceKind)
                                   << QString::fromStdString(deviceId)
                                   << QString::fromStdString(label);

        if (deviceKind.empty())
            continue;
        bool isDefault = false;

        if (deviceKind == std::string("videoinput")) {
            if (!m_cameraDevices.contains(deviceId)) {
                QCameraDevicePrivate *camera = new QCameraDevicePrivate; // QSharedData
                camera->id = QString::fromStdString(deviceId).toUtf8();
                camera->description = QString::fromUtf8(label.c_str());
                // no camera defaults, first in wins!
                camera->isDefault = !m_videoInputsAdded;
                m_cameraDevices.insert(deviceId, camera->create());
                m_videoInputsAdded = true;
            }
            cameraDevicesToRemove.removeOne(deviceId);
        } else if (deviceKind == std::string("audioinput")) {
            if (!m_audioInputs.contains(deviceId)) {
                isDefault = !m_audioInputsAdded;
                m_audioInputs.insert(
                        deviceId,
                        QAudioDevicePrivate::createQAudioDevice(std::make_unique<QWasmAudioDevice>(
                                deviceId.c_str(), label.c_str(), isDefault, QAudioDevice::Input)));

                m_audioInputsAdded = true;
            }
            audioInputsToRemove.removeOne(deviceId);
        } else if (deviceKind == std::string("audiooutput")) {
            if (!m_audioOutputs.contains(deviceId)) {
                isDefault = !m_audioOutputsAdded;
                m_audioOutputs.insert(
                        deviceId,
                        QAudioDevicePrivate::createQAudioDevice(std::make_unique<QWasmAudioDevice>(
                                deviceId.c_str(), label.c_str(), isDefault, QAudioDevice::Output)));
                ;

                m_audioOutputsAdded = true;
            }
            audioOutputsToRemove.removeOne(deviceId);
        }
        // if permissions are given label will hold the actual
        // camera name, such as "Live! Cam Sync 1080p (041e:409d)"
    }
    if (!m_firstInit)
        getOpenALAudioDevices();

           // any left here were removed
    int j = 0;
    for (; j < cameraDevicesToRemove.count(); j++) {
        m_cameraDevices.remove(cameraDevicesToRemove.at(j));
    }
    m_videoInputsRemoved = !cameraDevicesToRemove.isEmpty();

    for (j = 0; j < audioInputsToRemove.count(); j++) {
        m_audioInputs.remove(audioInputsToRemove.at(j));
    }
    m_audioInputsRemoved = !audioInputsToRemove.isEmpty();

    for (j = 0; j < audioOutputsToRemove.count(); j++) {
        m_audioOutputs.remove(audioOutputsToRemove.at(j));
    }
    m_audioOutputsRemoved = !audioOutputsToRemove.isEmpty();

    if (m_videoInputsAdded || m_videoInputsRemoved) {
        auto videoDevices = static_cast<QWasmCameraDevices*>(QPlatformMediaIntegration::instance()->videoDevices());
        videoDevices->onVideoInputsChanged();
    }
    if (m_audioInputsAdded || m_audioInputsRemoved)
        onAudioInputsChanged();
    if (m_audioOutputsAdded || m_audioOutputsRemoved)
        onAudioOutputsChanged();

    m_firstInit = false;

}

void QWasmMediaDevices::getMediaDevices()
{
    emscripten::val navigator = emscripten::val::global("navigator");
    m_jsMediaDevicesInterface = navigator["mediaDevices"];

    if (m_jsMediaDevicesInterface.isNull() || m_jsMediaDevicesInterface.isUndefined()) {
        qWarning() << "No media devices found";
        return;
    }

    if (qstdweb::haveAsyncify()) {
#ifdef QT_HAVE_EMSCRIPTEN_ASYNCIFY
        emscripten::val devicesList = m_jsMediaDevicesInterface.call<emscripten::val>("enumerateDevices").await();
        if (devicesList.isNull() || devicesList.isUndefined()) {
            qWarning() << "devices list error";
            return;
        }

        parseDevices(devicesList);
#endif
    } else {
        qstdweb::PromiseCallbacks enumerateDevicesCallback{
            .thenFunc =
            [&](emscripten::val devices) {
                parseDevices(devices);
            },
            .catchFunc =
            [this](emscripten::val error) {
                qWarning() << "mediadevices enumerateDevices fail"
                           << QString::fromStdString(error["name"].as<std::string>())
                           << QString::fromStdString(error["message"].as<std::string>());
                m_initDone = false;
            }
        };

        qstdweb::Promise::make(m_jsMediaDevicesInterface,
                               QStringLiteral("enumerateDevices"),
                               std::move(enumerateDevicesCallback));

               // setup devicechange monitor
        m_deviceChangedCallback = std::make_unique<qstdweb::EventCallback>(
                m_jsMediaDevicesInterface, "devicechange",
                [this, enumerateDevicesCallback](emscripten::val) {
                    qstdweb::Promise::make(m_jsMediaDevicesInterface,
                                           QStringLiteral("enumerateDevices"),
                                           std::move(enumerateDevicesCallback));
                });
    }

}

void QWasmMediaDevices::getOpenALAudioDevices()
{
    // VM3959:4 The AudioContext was not allowed to start.
    // It must be resumed (or created) after a user gesture on the page. https://goo.gl/7K7WLu
    auto capture = alcGetString(nullptr, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
    // present even if there is no capture device
    if (capture && !m_audioOutputs.contains(capture)) {
        m_audioInputs.insert(
                capture,
                QAudioDevicePrivate::createQAudioDevice(std::make_unique<QWasmAudioDevice>(
                        capture, "WebAssembly audio capture device", true, QAudioDevice::Input)));
        m_audioInputsAdded = true;
        onAudioInputsChanged();
    }

    auto playback = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    // present even if there is no playback device
    if (playback && !m_audioOutputs.contains(capture)) {
        m_audioOutputs.insert(
                playback,
                QAudioDevicePrivate::createQAudioDevice(std::make_unique<QWasmAudioDevice>(
                        playback, "WebAssembly audio playback device", true,
                        QAudioDevice::Output)));
        onAudioOutputsChanged();
    }
    m_firstInit = true;
}

QT_END_NAMESPACE
