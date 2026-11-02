// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qtmultimediaprivateqmlhelper.h"

#include <QtQml/qqmlapplicationengine.h>
#include <QtGui/qguiapplication.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // QSettings needs these to resolve a storage location.
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationName("qml-camera-advanced");

    // Must happen before any QtMultimedia usage and before the first window is created.
    QtMultimediaPrivateQmlHelper::applyPreferredBackends();

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.loadFromModule("QmlCameraAdvanced", "Main");

    return app.exec();
}
