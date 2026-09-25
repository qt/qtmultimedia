// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmlscreencaptureadvanced.h"

#include <QtQml/qqmlapplicationengine.h>
#include <QtGui/qguiapplication.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // QSettings needs these to resolve a storage location.
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationName("qml-screencapture-advanced");

    // Applies the backends picked in a previous session. Must happen before QtMultimedia
    // is used and before the first window is created.
    QmlScreencaptureAdvanced::applyPreferredBackends();

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            QGuiApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.loadFromModule("QmlScreenCaptureAdvanced", "Main");

    return app.exec();
}
