/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <QtDeclarative/QDeclarativeContext>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickItem>
#include "filereader.h"
#include "qmlapplicationviewer.h"
#include "trace.h"

#ifdef SMALL_SCREEN_LAYOUT
    static const QLatin1String MainQmlFile("main-smallscreen.qml");
#else
    static const QLatin1String MainQmlFile("main-largescreen.qml");
#endif

#ifdef PERFORMANCEMONITOR_SUPPORT
#include "performancemonitordeclarative.h"
#endif

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

#ifdef PERFORMANCEMONITOR_SUPPORT
    PerformanceMonitor::qmlRegisterTypes();
#endif

    QUrl fileName;
    qreal volume = 0.5;
    QStringList args = app.arguments();
#ifdef PERFORMANCEMONITOR_SUPPORT
    PerformanceMonitor::State performanceMonitorState;
#endif
    for (int i=1; i<args.count(); ++i) {
        const QString &arg = args.at(i);
        if (arg.startsWith('-')) {
            if ("-volume" == arg) {
                if (i+1 < args.count())
                    volume = 0.01 * args.at(++i).toInt();
                else
                    qtTrace() << "Option \"-volume\" takes a value";
            }
#ifdef PERFORMANCEMONITOR_SUPPORT
            else if (PerformanceMonitor::parseArgument(arg, performanceMonitorState)) {
                // Do nothing
            }
#endif
            else {
                qtTrace() << "Option" << arg << "ignored";
            }
        } else {
            if (fileName.isEmpty())
                fileName = QUrl::fromLocalFile(arg);
            else
                qtTrace() << "Argument" << arg << "ignored";
        }
    }

    QmlApplicationViewer viewer;

    viewer.requestWindowOrientation(Qt::LandscapeOrientation);
    viewer.setMainQmlFile(QLatin1String("qml/qmlvideofx/") + MainQmlFile);
    QQuickItem *rootObject = viewer.rootObject();
    rootObject->setProperty("fileName", fileName);
    viewer.rootObject()->setProperty("volume", volume);

#ifdef PERFORMANCEMONITOR_SUPPORT
    if (performanceMonitorState.valid) {
        rootObject->setProperty("perfMonitorsLogging", performanceMonitorState.logging);
        rootObject->setProperty("perfMonitorsVisible", performanceMonitorState.visible);
    }
    QObject::connect(&viewer, SIGNAL(afterRendering()),
                     rootObject, SLOT(qmlFramePainted()));
#endif

    FileReader fileReader;
    viewer.rootContext()->setContextProperty("fileReader", &fileReader);

    QString imagePath = "../../images";
    const QStringList picturesLocation = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    if (!picturesLocation.isEmpty())
        imagePath = picturesLocation.first();
    viewer.rootContext()->setContextProperty("imagePath", imagePath);

    QString videoPath;
    const QStringList moviesLocation = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    if (!moviesLocation.isEmpty())
        videoPath = moviesLocation.first();
    viewer.rootContext()->setContextProperty("videoPath", videoPath);

    viewer.setWindowTitle("qmlvideofx");
    viewer.setWindowFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint |
                          Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    viewer.setMinimumSize(QSize(600, 400));

#ifdef SMALL_SCREEN_PHYSICAL
    viewer.showFullScreen();
#else
    viewer.showExpanded();
#endif

    // Delay invocation of init until the event loop has started, to work around
    // a GL context issue on Harmattan: without this, we get the following error
    // when the first ShaderEffectItem is created:
    // "QGLShaderProgram::addShader: Program and shader are not associated with same context"
    QMetaObject::invokeMethod(viewer.rootObject(), "init", Qt::QueuedConnection);

    return app.exec();
}

