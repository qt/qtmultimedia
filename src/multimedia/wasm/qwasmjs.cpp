// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmjs_p.h"

QT_BEGIN_NAMESPACE

JsMediaInputStream::JsMediaInputStream(QObject *parent)
    : QObject{parent}
{
}

void JsMediaInputStream::setStreamDevice(const std::string &id)
{
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
                m_mediaStream = std::move(stream);
                emit mediaStreamReady();
            },
        .catchFunc =
        [](emscripten::val error) {
                qWarning()
                << "setStreamDevice getUserMedia  fail"
                << error["name"].as<std::string>()
                << error["message"].as<std::string>();
            }
    };

    emscripten::val constraints = emscripten::val::object();
    if (m_needsAudio) {
        emscripten::val audioConstraints = emscripten::val::object();
        audioConstraints.set("audio", m_needsVideo); // formatting?
        emscripten::val exactDeviceId = emscripten::val::object();
        exactDeviceId.set("exact", id);
        audioConstraints.set("deviceId", exactDeviceId);
        constraints.set("audio", audioConstraints);
    }

    if (m_needsVideo) {
        emscripten::val videoContraints = emscripten::val::object();
        emscripten::val exactDeviceId = emscripten::val::object();
        exactDeviceId.set("exact", id);
        videoContraints.set("deviceId", exactDeviceId);
        videoContraints.set("resizeMode", std::string("crop-and-scale"));
        constraints.set("video", videoContraints);
    }

    // ???
    // if (!id.empty()) {
    //     emscripten::val exactDeviceId = emscripten::val::object();
    //     exactDeviceId.set("exact", id);
    //     constraints.set("deviceId", exactDeviceId);
    // }

    // we do it this way as this prompts user for permissions
    qstdweb::Promise::make(mediaDevices, QStringLiteral("getUserMedia"),
                           std::move(getUserMediaCallback), constraints);
}

void JsMediaInputStream::setupMediaStream(emscripten::val mStream)
{
    m_mediaStream = mStream;

    auto activeStreamCallback = [=](emscripten::val event) {
        m_active = true;
        emit activated(m_active);
    };
    m_activeStreamEvent.reset(new qstdweb::EventCallback(m_mediaStream, "active", activeStreamCallback));

    auto inactiveStreamCallback = [=](emscripten::val event) {
        m_active = false;
        emit activated(m_active);
    };
    m_inactiveStreamEvent.reset(new qstdweb::EventCallback(m_mediaStream, "inactive", inactiveStreamCallback));
}

QT_END_NAMESPACE
