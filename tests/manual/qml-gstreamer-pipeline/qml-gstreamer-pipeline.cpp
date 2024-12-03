// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtEnvironmentVariables>

int main(int argc, char *argv[])
{
    qputenv("QT_MEDIA_BACKEND", "gstreamer");

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/qml-gstreamer-pipeline.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
