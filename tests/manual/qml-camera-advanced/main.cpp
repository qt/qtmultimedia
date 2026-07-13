// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/QGuiApplication>

#include <QtQml/QQmlApplicationEngine>

#include "qtmultimediaprivateqmlhelper.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // QSettings needs these to resolve a storage location.
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationName("qml-camera-advanced");

    // Apply any backend the user picked in a previous session before the media integration is
    // initialized. Must happen before any QtMultimedia usage.
    QtMultimediaPrivateQmlHelper::applyPreferredBackendToEnvironment();

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
