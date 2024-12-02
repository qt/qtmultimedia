// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "trace.h"
#include "qmlvideo/videosingleton.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

#if QT_CONFIG(permissions)
  #include <QPermission>
#endif

static const QString DefaultFileName1 = "";
static const QString DefaultFileName2 = "";

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QString source1, source2;
    qreal volume = 0.5;
    QStringList args = app.arguments();
    bool sourceIsUrl = false;
    bool perfMonitorsLogging = false;
    bool perfMonitorsVisible = true;
    for (int i = 1; i < args.size(); ++i) {
        const QByteArray arg = args.at(i).toUtf8();
        if (arg.startsWith('-')) {
            if ("-volume" == arg) {
                if (i + 1 < args.count())
                    volume = 0.01 * args.at(++i).toInt();
                else
                    qtTrace() << "Option \"-volume\" takes a value";
            }
            else if (arg == "-log-perf") {
                perfMonitorsLogging = true;
            }
            else if (arg == "-no-log-perf") {
                perfMonitorsLogging = false;
            }
            else if (arg == "-show-perf") {
                perfMonitorsVisible = true;
            }
            else if (arg == "-hide-perf") {
                perfMonitorsVisible = false;
            }
            else if ("-url" == arg) {
                sourceIsUrl = true;
            } else {
                qtTrace() << "Option" << arg << "ignored";
            }
        } else {
            if (source1.isEmpty())
                source1 = arg;
            else if (source2.isEmpty())
                source2 = arg;
            else
                qtTrace() << "Argument" << arg << "ignored";
        }
    }

    QUrl url1, url2;
    if (sourceIsUrl) {
        url1 = source1;
        url2 = source2;
    } else {
        if (!source1.isEmpty())
            url1 = QUrl::fromLocalFile(source1);
        if (!source2.isEmpty())
            url2 = QUrl::fromLocalFile(source2);
    }

    const QStringList moviesLocation = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    const QUrl videoPath = QUrl::fromLocalFile(moviesLocation.isEmpty() ? app.applicationDirPath()
                                                                        : moviesLocation.front());

    QQuickView viewer;
    VideoSingleton* singleton = viewer.engine()->singletonInstance<VideoSingleton*>("qmlvideo", "VideoSingleton");
    singleton->setVideoPath(videoPath);
    singleton->setSource1(source1);
    singleton->setSource2(source2);
    singleton->setVolume(volume);
    viewer.loadFromModule("qmlvideo", "Main");
    QObject::connect(viewer.engine(), &QQmlEngine::quit, &viewer, &QQuickView::close);

    QQuickItem *rootObject = viewer.rootObject();
    rootObject->setProperty("perfMonitorsLogging", perfMonitorsLogging);
    rootObject->setProperty("perfMonitorsVisible", perfMonitorsVisible);
    QObject::connect(&viewer, SIGNAL(afterRendering()), rootObject, SLOT(qmlFramePainted()));

    QMetaObject::invokeMethod(rootObject, "init");

    auto setupView = [&viewer]() {
        viewer.setMinimumSize(QSize(640, 360));
        viewer.show();
    };

#if QT_CONFIG(permissions)
    QCameraPermission cameraPermission;
    qApp->requestPermission(cameraPermission, [&setupView](const QPermission &permission) {
        // Show UI in any case. If there is no permission, the UI will just
        // be disabled.
        if (permission.status() != Qt::PermissionStatus::Granted)
            qWarning("Camera permission is not granted! Camera will not be available.");
        setupView();
    });
#else
    setupView();
#endif

    return app.exec();
}
