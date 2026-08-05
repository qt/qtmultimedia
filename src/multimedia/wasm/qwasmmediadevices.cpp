// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmmediadevices_p.h"
#include "private/qcameradevice_p.h"
#include "private/qplatformmediaintegration_p.h"
#include "qwasmwebaudiosource_p.h"
#include "qwasmwebaudiosink_p.h"
#include "qwasmaudiodevice_p.h"
#include <QtMultimedia/private/qmultimedia_ranges_p.h>

#include <QDebug>

#include <emscripten.h>

#include <algorithm>
#include <string_view>
#include <utility>

QT_BEGIN_NAMESPACE

namespace ranges = QtMultimediaPrivate::ranges;

Q_LOGGING_CATEGORY(qWasmMediaDevices, "qt.multimedia.wasm.mediadevices")

namespace {

// Chrome and Edge add synthetic entries to enumerateDevices() that alias a real
// device: "default" tracks the system default, "communications" the device picked
// for voice calls. Both carry the aliased device's groupId. Firefox and Safari
// have no such entries and simply enumerate the default device first.
constexpr std::string_view s_defaultDeviceId = "default";
constexpr std::string_view s_communicationsDeviceId = "communications";

bool isAliasDeviceId(std::string_view deviceId)
{
    return deviceId == s_defaultDeviceId || deviceId == s_communicationsDeviceId;
}

std::string stringProperty(const emscripten::val &mediaDevice, const char *property)
{
    const emscripten::val value = mediaDevice[property];
    if (value.isUndefined() || value.isNull())
        return std::string();
    return value.as<std::string>();
}

// enumerateDevices() reports empty ids until permission is granted, and we
// substitute a synthetic per-kind id for those. Default lookup must match.
std::string effectiveDeviceId(const std::string &deviceId, const std::string &deviceKind)
{
    return deviceId.empty() ? "System " + deviceKind : deviceId;
}

template <typename DeviceList>
bool containsDeviceId(const DeviceList &devices, const QByteArray &deviceId)
{
    return ranges::any_of(devices, [&deviceId](const auto &device) {
        return device.id() == deviceId;
    });
}

} // namespace

static QWasmMediaDevices *s_mediaDevicesInstance = nullptr;
static bool s_constructingInstance = false;

bool isFirefox() {
    return !emscripten::val::global("InstallTrigger").isUndefined();
}


// Firefox only as it limits enumerateDevices to inputs only when no permissions are given
extern "C" {
EMSCRIPTEN_KEEPALIVE void qtMediaDevicesOnAudioOutputSelected()
{
    if (QWasmMediaDevices *instance = QWasmMediaDevices::instance())
        instance->getMediaDevices();
}
} // extern "C"

EM_JS(void, setupAudioOutputSelector, (), {
    const overlay = document.createElement('div');
    overlay.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.5);z-index:9999;display:flex;align-items:center;justify-content:center;';

    const dialog = document.createElement('div');
    dialog.style.cssText = 'background:white;padding:24px;border-radius:8px;text-align:center;font-family:sans-serif;min-width:240px;';

    const message = document.createElement('p');
    message.textContent = 'Select an audio output device to continue.';
    message.style.cssText = 'margin:0 0 16px 0;font-size:14px;';

    const button = document.createElement('button');
    button.textContent = 'Select Audio Output';
    button.style.cssText = 'padding:8px 16px;font-size:14px;cursor:pointer;';

    button.addEventListener('click', async () => {
        document.body.removeChild(overlay);
        try {
            const deviceInfo = await navigator.mediaDevices.selectAudioOutput();
            console.log("Selected device: ", deviceInfo.label);
            Module._qtMediaDevicesOnAudioOutputSelected();
        } catch (err) {
            console.error(err);
        }
    }, { once: true });

    dialog.appendChild(message);
    dialog.appendChild(button);
    overlay.appendChild(dialog);
    document.body.appendChild(overlay);
});

QWasmCameraDevices::QWasmCameraDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration)
{
}

QList<QCameraDevice> QWasmCameraDevices::findVideoInputs() const
{
    return QWasmMediaDevices::instance() ? QWasmMediaDevices::instance()->videoInputs() : QList<QCameraDevice>();
}

void QWasmCameraDevices::connectNotify(const QMetaMethod &)
{
    Q_ASSERT(QThread::isMainThread());
    QWasmMediaDevices::instance();
}

QWasmAudioDevices::QWasmAudioDevices() = default;

QPlatformAudioSource *QWasmAudioDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                           const QAudioFormat &fmt,
                                                           QObject *parent)
{
    return new QWasmAudioSource(deviceInfo, fmt, parent);
}

QPlatformAudioSink *QWasmAudioDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                       const QAudioFormat &fmt,
                                                       QObject *parent)
{
    return new QWasmAudioSink(deviceInfo, fmt, parent);
}

QList<QAudioDevice> QWasmAudioDevices::findAudioInputs() const
{
    return QWasmMediaDevices::instance() ? QWasmMediaDevices::instance()->audioInputs() : QList<QAudioDevice>();
}

QList<QAudioDevice> QWasmAudioDevices::findAudioOutputs() const
{
    return QWasmMediaDevices::instance() ? QWasmMediaDevices::instance()->audioOutputs() : QList<QAudioDevice>();
}

void QWasmAudioDevices::connectNotify(const QMetaMethod &)
{
    Q_ASSERT(QThread::isMainThread());
    QWasmMediaDevices::instance();
}

QWasmMediaDevices::QWasmMediaDevices()
{
}

QWasmMediaDevices *QWasmMediaDevices::instance()
{
    if (s_mediaDevicesInstance)
        return s_mediaDevicesInstance;
    if (s_constructingInstance)
        return nullptr;
    s_constructingInstance = true;
    s_mediaDevicesInstance = new QWasmMediaDevices();
    s_constructingInstance = false;
    s_mediaDevicesInstance->initDevices();
    return s_mediaDevicesInstance;
}

void QWasmMediaDevices::initDevices()
{
    if (m_initDone)
        return;

    m_initDone = true;

    // Seed a synchronous fallback output device immediately: enumerateDevices() below
    // is asynchronous, so without this, any consumer that needs a device synchronously
    // right after construction (e.g. QSoundEffect) sees an empty list and fails.
    insertFallbackAudioOutput();

    if (isFirefox())
        setupAudioOutputSelector();
    else
        getMediaDevices(); // asynchronous
}

QList<QCameraDevice> QWasmMediaDevices::videoInputs() const
{
    return m_cameraDevices;
}

QList<QAudioDevice> QWasmMediaDevices::audioInputs() const
{
    return m_audioInputs;
}

QList<QAudioDevice> QWasmMediaDevices::audioOutputs() const
{
    return m_audioOutputs;
}

// Works out which enumerated device should carry the isDefault flag. When the
// browser publishes a synthetic "default" entry it is resolved back to the real
// device through its groupId, so the returned id is one that also ends up in our
// device lists. Otherwise the first enumerated device of that kind wins.
QByteArray QWasmMediaDevices::findDefaultDeviceId(emscripten::val devices,
                                                  std::string_view deviceKind) const
{
    const QString kindName = QString::fromUtf8(deviceKind.data(), qsizetype(deviceKind.size()));
    std::string defaultGroupId;
    QList<std::pair<QByteArray, std::string>> candidates;

    const int deviceCount = devices["length"].as<int>();
    for (int i = 0; i < deviceCount; ++i) {
        const emscripten::val mediaDevice = devices[i];
        const std::string kind = stringProperty(mediaDevice, "kind");
        if (kind != deviceKind)
            continue;

        const std::string deviceId = stringProperty(mediaDevice, "deviceId");
        if (deviceId == s_defaultDeviceId) {
            defaultGroupId = stringProperty(mediaDevice, "groupId");
            continue;
        }
        if (isAliasDeviceId(deviceId))
            continue;

        candidates.append({ QByteArray::fromStdString(effectiveDeviceId(deviceId, kind)),
                            stringProperty(mediaDevice, "groupId") });
    }

    if (candidates.isEmpty()) {
        qCDebug(qWasmMediaDevices) << "no" << kindName << "devices enumerated, no default";
        return {};
    }

    if (!defaultGroupId.empty()) {
        const auto it = ranges::find_if(candidates, [&defaultGroupId](const auto &candidate) {
            return candidate.second == defaultGroupId;
        });
        if (it != candidates.cend()) {
            qCDebug(qWasmMediaDevices)
                    << "default" << kindName << "resolved from the browser's default entry:"
                    << it->first << "via groupId" << QString::fromStdString(defaultGroupId);
            return it->first;
        }
        qCDebug(qWasmMediaDevices)
                << "the browser's default" << kindName << "entry has groupId"
                << QString::fromStdString(defaultGroupId)
                << "which matches no enumerated device, falling back to the first";
    }
    qCDebug(qWasmMediaDevices) << "default" << kindName
                               << "falls back to the first enumerated device:"
                               << candidates.constFirst().first;
    return candidates.constFirst().first;
}

void QWasmMediaDevices::applyDefaultDevice(QList<QAudioDevice> &audioDevices,
                                           const QByteArray &defaultDeviceId)
{
    for (QAudioDevice &audioDevice : audioDevices) {
        QAudioDevicePrivate *devicePrivate = QAudioDevicePrivate::handle(audioDevice);
        if (!devicePrivate)
            continue;
        const bool isDefault = audioDevice.id() == defaultDeviceId;
        if (devicePrivate->isDefault == isDefault)
            continue;
        qCDebug(qWasmMediaDevices) << "default" << audioDevice.mode()
                                   << (isDefault ? "set to" : "cleared from")
                                   << audioDevice.id() << audioDevice.description();
        devicePrivate->isDefault = isDefault;
    }
}

void QWasmMediaDevices::applyDefaultDevice(QList<QCameraDevice> &cameraDevices,
                                           const QByteArray &defaultDeviceId)
{
    for (QCameraDevice &cameraDevice : cameraDevices) {
        QCameraDevicePrivate *devicePrivate = QCameraDevicePrivate::handle(cameraDevice);
        if (!devicePrivate)
            continue;
        const bool isDefault = cameraDevice.id() == defaultDeviceId;
        if (devicePrivate->isDefault == isDefault)
            continue;
        qCDebug(qWasmMediaDevices) << "default camera"
                                   << (isDefault ? "set to" : "cleared from")
                                   << cameraDevice.id() << cameraDevice.description();
        devicePrivate->isDefault = isDefault;
    }
}

void QWasmMediaDevices::parseDevices(emscripten::val devices)
{
    if (devices.isNull() || devices.isUndefined()) {
        qWarning() << "Something went wrong enumerating devices";
        return;
    }

    QList<QByteArray> cameraDevicesToRemove;
    cameraDevicesToRemove.reserve(m_cameraDevices.count());
    for (const QCameraDevice &cameraDevice : m_cameraDevices)
        cameraDevicesToRemove.append(cameraDevice.id());

    QList<QByteArray> audioOutputsToRemove;
    audioOutputsToRemove.reserve(m_audioOutputs.count());
    for (const QAudioDevice &audioDevice : m_audioOutputs)
        audioOutputsToRemove.append(audioDevice.id());

    QList<QByteArray> audioInputsToRemove;
    audioInputsToRemove.reserve(m_audioInputs.count());
    for (const QAudioDevice &audioDevice : m_audioInputs)
        audioInputsToRemove.append(audioDevice.id());
    m_audioInputsAdded = false;
    m_audioOutputsAdded = false;
    m_videoInputsAdded = false;

    const QByteArray defaultCameraId = findDefaultDeviceId(devices, "videoinput");
    const QByteArray defaultAudioInputId = findDefaultDeviceId(devices, "audioinput");
    const QByteArray defaultAudioOutputId = findDefaultDeviceId(devices, "audiooutput");

    bool m_videoInputsRemoved = false;
    bool m_audioInputsRemoved = false;
    bool m_audioOutputsRemoved = false;

    for (int i = 0; i < devices["length"].as<int>(); i++) {

        emscripten::val mediaDevice = devices[i];

        const std::string deviceKind = mediaDevice["kind"].as<std::string>();
        std::string label = mediaDevice["label"].as<std::string>();
        std::string deviceId = mediaDevice["deviceId"].as<std::string>();

        if (deviceKind.empty())
            continue;

        qCDebug(qWasmMediaDevices) << QString::fromStdString(deviceKind)
                                   << QString::fromStdString(deviceId)
                                   << QString::fromStdString(label) << "groupId"
                                   << QString::fromStdString(stringProperty(mediaDevice, "groupId"));

        // the real device this aliases is enumerated separately
        if (isAliasDeviceId(deviceId))
            continue;

        if (deviceId.empty()) { // no permissions we'll use System;
            label = "System " + deviceKind;
            deviceId = label;
        }

        const QByteArray qtDeviceId = QByteArray::fromStdString(deviceId);

        if (deviceKind == std::string_view("videoinput")) {
            if (!containsDeviceId(m_cameraDevices, qtDeviceId)) {
                QCameraDevicePrivate *camera = new QCameraDevicePrivate; // QSharedData
                camera->id = qtDeviceId;
                camera->description = QString::fromUtf8(label.c_str());
                camera->isDefault = false; // assigned by applyDefaultDevice() below
                m_cameraDevices.append(camera->create());
                m_videoInputsAdded = true;
            }
            cameraDevicesToRemove.removeOne(qtDeviceId);
        } else if (deviceKind == std::string_view("audioinput")) {
            if (!containsDeviceId(m_audioInputs, qtDeviceId)) {
                m_audioInputs.append(
                        QAudioDevicePrivate::createQAudioDevice(std::make_unique<QWasmAudioDevice>(
                                deviceId.c_str(), label.c_str(), false, QAudioDevice::Input)));

                m_audioInputsAdded = true;
            }
            audioInputsToRemove.removeOne(qtDeviceId);
        } else if (deviceKind == std::string_view("audiooutput")) {
            if (!containsDeviceId(m_audioOutputs, qtDeviceId)) {
                m_audioOutputs.append(
                        QAudioDevicePrivate::createQAudioDevice(std::make_unique<QWasmAudioDevice>(
                                deviceId.c_str(), label.c_str(), false, QAudioDevice::Output)));

                m_audioOutputsAdded = true;
            }
            audioOutputsToRemove.removeOne(qtDeviceId);
        }
        // if permissions are given label will hold the actual
        // camera name, such as "Live! Cam Sync 1080p (041e:409d)"
    }
    // any left here were removed
    for (const QByteArray &cameraId : cameraDevicesToRemove) {
        m_cameraDevices.removeIf([&](const QCameraDevice &cameraDevice) {
            return cameraDevice.id() == cameraId;
        });
    }
    m_videoInputsRemoved = !cameraDevicesToRemove.isEmpty();

    for (const QByteArray &audioDeviceId : audioInputsToRemove) {
        m_audioInputs.removeIf([&audioDeviceId](const QAudioDevice &audioDevice) {
            return audioDevice.id() == audioDeviceId;
        });
    }
    m_audioInputsRemoved = !audioInputsToRemove.isEmpty();

    for (const QByteArray &audioDeviceId : audioOutputsToRemove) {
        m_audioOutputs.removeIf([&audioDeviceId](const QAudioDevice &audioDevice) {
            return audioDevice.id() == audioDeviceId;
        });
    }
    m_audioOutputsRemoved = !audioOutputsToRemove.isEmpty();

    if (!m_audioOutputsAdded) {
        // Firefox and Safari require mic or camera permissions
        // (or selectAudioOutput for Firefox)
        // to enumerate output devices, so we just fake one.
        // The device actually does not require perms to play.
        insertFallbackAudioOutput();
    }

    // re-evaluated on every enumeration: the default can move when devices are
    // plugged in or removed, or when the user changes the system default
    applyDefaultDevice(m_cameraDevices, defaultCameraId);
    applyDefaultDevice(m_audioInputs, defaultAudioInputId);
    applyDefaultDevice(m_audioOutputs, defaultAudioOutputId);

    if (m_videoInputsAdded || m_videoInputsRemoved) {
        auto videoDevices = static_cast<QWasmCameraDevices*>(QPlatformMediaIntegration::instance()->videoDevices());
        QMetaObject::invokeMethod(videoDevices, &QWasmCameraDevices::onVideoInputsChanged,
                                   Qt::QueuedConnection);
    }
    if (m_audioInputsAdded || m_audioInputsRemoved) {
        auto audioDevices = static_cast<QWasmAudioDevices*>(QPlatformMediaIntegration::instance()->audioDevices());
        QMetaObject::invokeMethod(audioDevices, &QWasmAudioDevices::onAudioInputsChanged,
                                   Qt::QueuedConnection);
    }
    if (m_audioOutputsAdded || m_audioOutputsRemoved) {
        auto audioDevices = static_cast<QWasmAudioDevices*>(QPlatformMediaIntegration::instance()->audioDevices());
        QMetaObject::invokeMethod(audioDevices, &QWasmAudioDevices::onAudioOutputsChanged,
                                   Qt::QueuedConnection);
    }

}

void QWasmMediaDevices::insertFallbackAudioOutput()
{
    const bool hasFallback = std::any_of(m_audioOutputs.cbegin(), m_audioOutputs.cend(),
                                         [](const QAudioDevice &audioDevice) {
                                             return audioDevice.id().isEmpty();
                                         });
    if (!hasFallback) {
        m_audioOutputs.append(
                QAudioDevicePrivate::createQAudioDevice(std::make_unique<QWasmAudioDevice>(
                        "", "System output", true, QAudioDevice::Output)));
    }
    m_audioOutputsAdded = true;
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
        auto asyncEnumerate = [](void *arg){
            QWasmMediaDevices *mediaDevices = static_cast<QWasmMediaDevices *>(arg);
            mediaDevices->devicesList = mediaDevices->m_jsMediaDevicesInterface.call<emscripten::val>("enumerateDevices").await();
            if (mediaDevices->devicesList.isNull() || mediaDevices->devicesList.isUndefined()) {
                qWarning() << "devices list error";
                return;
            }
            mediaDevices->parseDevices(mediaDevices->devicesList);
        };

        asyncEnumerate(this);

        m_deviceChangedCallback = std::make_unique<qstdweb::EventCallback>(
                m_jsMediaDevicesInterface, "devicechange",
                [this, asyncEnumerate](emscripten::val) {
                    asyncEnumerate(this);
                });
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
            },
            .finallyFunc = {}
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

QT_END_NAMESPACE
