// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/qqmlapplicationengine.h>
#include <QtGui/qguiapplication.h>
#include <QtCore/qtenvironmentvariables.h>

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
