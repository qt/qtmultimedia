// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#if QT_CONFIG(permissions)
  #include <QPermission>
#endif

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreated, &app,
            [url](QObject *obj, const QUrl &objUrl) {
                if (!obj && url == objUrl)
                    QCoreApplication::exit(-1);
            },
            Qt::QueuedConnection);

#if QT_CONFIG(permissions)
    // If the permissions are not granted, display another main window, which
    // simply contains the error message.
    const QUrl noPermissionsUrl(QStringLiteral("qrc:/main_no_permissions.qml"));
    QCameraPermission cameraPermission;
    qApp->requestPermission(cameraPermission, [&](const QPermission &permission) {
        if (permission.status() != Qt::PermissionStatus::Granted) {
            qWarning("Camera permission is not granted!");
            engine.load(noPermissionsUrl);
            return;
        }
        QMicrophonePermission micPermission;
        qApp->requestPermission(micPermission, [&](const QPermission &permission) {
            if (permission.status() != Qt::PermissionStatus::Granted) {
                qWarning("Microphone permission is not granted!");
                engine.load(noPermissionsUrl);
            } else {
                engine.load(url);
            }
        });
    });
#else
    engine.load(url);
#endif

    return app.exec();
}
