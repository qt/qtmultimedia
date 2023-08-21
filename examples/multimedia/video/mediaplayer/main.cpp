// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCommandLineParser>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName("MediaPlayer Example");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription("Qt Quick MediaPlayer Example");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("url", "The URL(s) to open.");
    parser.process(app);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));

    if (!parser.positionalArguments().isEmpty()) {
        QUrl source = QUrl::fromUserInput(parser.positionalArguments().at(0), QDir::currentPath());
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                         &engine, [source](QObject *object, const QUrl &) {
                             qDebug() << "setting source";
                             object->setProperty("source", source);
                         });
    }

    engine.load(url);

    return app.exec();
}
