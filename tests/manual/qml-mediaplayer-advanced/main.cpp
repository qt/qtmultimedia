// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmlmediaplayeradvanced.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSettings>

#include <QLibraryInfo>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationName("qml-mediaplayer-advanced");
    // Store settings in an INI file on every platform, rather than the registry on
    // Windows or a plist on macOS, so that they are easy to inspect, edit and reset.
    // Must be set before any settings are read.
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QmlMediaPlayerAdvanced::applyPreferredBackends();

    qDebug() << QLibraryInfo::paths(QLibraryInfo::QmlImportsPath);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("QmlMediaPlayerAdvanced", "Main");

    return app.exec();
}
