// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmcapturablewindows_p.h"
#include "private/qcapturablewindow_p.h"
#include <QtCore/private/qstdweb_p.h>
#include <QGuiApplication>
#include <QWindow>
#include <QUuid>

#include <QDebug>

QT_BEGIN_NAMESPACE

QWasmCapturableWindows::QWasmCapturableWindows()
{
    m_capurableWindows.push_back(QCapturableWindowPrivate::create(
        static_cast<QCapturableWindowPrivate::Id>(111),
        QStringLiteral("Any Window")));

    getDisplayMedia();
}

QList<QCapturableWindow> QWasmCapturableWindows::windows() const
{
    return m_capurableWindows;
}

void QWasmCapturableWindows::getDisplayMedia()
{
// populate windows from getDisplayMedia which includes windows from host platform.

    emscripten::val navigator = emscripten::val::global("navigator");
    emscripten::val mediaDevices = navigator["mediaDevices"];

    if (mediaDevices.isNull() || mediaDevices.isUndefined()) {
        qWarning() << "No media devices found";
        return;
    }

    emscripten::val constraints = emscripten::val::object();

    constraints.set("video", true);
    constraints.set("selfBrowserSurface", std::string("include"));

    // will ask for permissions!
    qstdweb::PromiseCallbacks getDisplayMediaCallback{
        // default
        .thenFunc =
        [this](emscripten::val stream) {
            QUuid uid(QString::fromStdString(stream["id"].as<std::string>()));

            m_capurableWindows.push_back(QCapturableWindowPrivate::create(
                static_cast<QCapturableWindowPrivate::Id>(uid.toByteArray().toLong()),
                QString::fromStdString(stream["id"].as<std::string>())));
        },
        .catchFunc =
        [](emscripten::val) {
        }
    };

    qstdweb::Promise::make(mediaDevices, QStringLiteral("getDisplayMedia"),
                           std::move(getDisplayMediaCallback), constraints);
}

bool QWasmCapturableWindows::isWindowValid(const QCapturableWindowPrivate &) const
{
    return m_capurableWindows.count() > 0;
}

QT_END_NAMESPACE
