// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "player.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QUrl>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName(u"Player Example"_s);
    QCoreApplication::setOrganizationName(u"QtProject"_s);
    QCoreApplication::setApplicationVersion(QLatin1StringView(QT_VERSION_STR));
    QCommandLineParser parser;
    parser.setApplicationDescription(u"Qt MultiMedia Player Example"_s);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(u"url"_s, u"The URL(s) to open."_s);
    parser.process(app);

    Player player;

    if (!parser.positionalArguments().isEmpty() && player.isPlayerAvailable()) {
        QList<QUrl> urls;
        for (const auto &a : parser.positionalArguments())
            urls.append(QUrl::fromUserInput(a, QDir::currentPath()));
        player.addToPlaylist(urls);
    }

    player.show();
    return QCoreApplication::exec();
}
