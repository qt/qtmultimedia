// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "performancemonitor.h"
#include "trace.h"
#ifdef PERFORMANCEMONITOR_SUPPORT
#    include "performancemonitordeclarative.h"
#endif

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

#ifdef PERFORMANCEMONITOR_SUPPORT
    PerformanceMonitor::qmlRegisterTypes();
#endif

    QString source1, source2;
    qreal volume = 0.5;
    QStringList args = app.arguments();
#ifdef PERFORMANCEMONITOR_SUPPORT
    PerformanceMonitor::State performanceMonitorState;
#endif
    bool sourceIsUrl = false;
    for (int i = 1; i < args.size(); ++i) {
        const QByteArray arg = args.at(i).toUtf8();
        if (arg.startsWith('-')) {
            if ("-volume" == arg) {
                if (i + 1 < args.count())
                    volume = 0.01 * args.at(++i).toInt();
                else
                    qtTrace() << "Option \"-volume\" takes a value";
            }
#ifdef PERFORMANCEMONITOR_SUPPORT
            else if (performanceMonitorState.parseArgument(arg)) {
                // Do nothing
            }
#endif
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

    QQuickView viewer;
    viewer.setSource(QUrl("qrc:///qml/qmlvideo/main.qml"));
    QObject::connect(viewer.engine(), &QQmlEngine::quit, &viewer, &QQuickView::close);

    QQuickItem *rootObject = viewer.rootObject();
    rootObject->setProperty("source1", url1);
    rootObject->setProperty("source2", url2);
    rootObject->setProperty("volume", volume);

#ifdef PERFORMANCEMONITOR_SUPPORT
    if (performanceMonitorState.valid) {
        rootObject->setProperty("perfMonitorsLogging", performanceMonitorState.logging);
        rootObject->setProperty("perfMonitorsVisible", performanceMonitorState.visible);
    }
    QObject::connect(&viewer, SIGNAL(afterRendering()), rootObject, SLOT(qmlFramePainted()));
#endif

    const QStringList moviesLocation =
            QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    const QUrl videoPath = QUrl::fromLocalFile(moviesLocation.isEmpty() ? app.applicationDirPath()
                                                                        : moviesLocation.front());
    viewer.rootContext()->setContextProperty("videoPath", videoPath);

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
