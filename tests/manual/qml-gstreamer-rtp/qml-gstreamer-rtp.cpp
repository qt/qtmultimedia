// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QtEnvironmentVariables>

int main(int argc, char *argv[])
{
    qputenv("QT_MEDIA_BACKEND", "gstreamer");

    QProcess process;
    process.startCommand(
            "gst-launch-1.0 videotestsrc ! x264enc ! udpsink host=127.0.0.1 port=50004");

    auto scopeGuard = qScopeGuard([&] {
        process.terminate();
        process.waitForFinished();
    });

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    process.waitForStarted();

    engine.load(QUrl("qrc:/qml-gstreamer-rtp.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
