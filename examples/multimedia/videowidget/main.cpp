// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "videoplayer.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QScreen>
#include <QUrl>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("Video Widget Example");
    QCoreApplication::setOrganizationName("QtProject");
    QGuiApplication::setApplicationDisplayName(QCoreApplication::applicationName());
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription("Qt Video Widget Example");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("url", "The URL to open.");
    parser.process(app);

    VideoPlayer player;
    if (!parser.positionalArguments().isEmpty()) {
        const QUrl url = QUrl::fromUserInput(parser.positionalArguments().constFirst(),
                                             QDir::currentPath(), QUrl::AssumeLocalFile);
        player.setUrl(url);
    }

    const QSize availableGeometry = player.screen()->availableSize();
    player.resize(availableGeometry.width() / 6, availableGeometry.height() / 4);
    player.show();

    return app.exec();
}
