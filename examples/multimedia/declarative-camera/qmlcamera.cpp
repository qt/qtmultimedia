// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickView>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);

    QObject::connect(view.engine(), &QQmlEngine::quit, qApp, &QGuiApplication::quit);
    view.setSource(QUrl("qrc:///declarative-camera.qml"));
    view.show();

    return app.exec();
}
