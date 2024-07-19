// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QTimer>
#include <QtCore/QCommandLineParser>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

using namespace std::chrono_literals;
using namespace Qt::Literals;

int mainToggleWidgets(QString filename)
{
    QMediaPlayer player;
    QVideoWidget widget1;
    QVideoWidget widget2;
    QAudioOutput audioOutput;
    player.setVideoOutput(&widget1);
    player.setAudioOutput(&audioOutput);
    player.setSource(filename);

    QTimer toggleOutput;
    bool toggled = {};

    toggleOutput.callOnTimeout([&] {
        toggled = !toggled;
        if (toggled)
            player.setVideoOutput(&widget2);
        else
            player.setVideoOutput(&widget1);
    });

    toggleOutput.setInterval(1s);
    toggleOutput.start();

    widget1.show();
    widget2.show();
    player.play();
    return QApplication::exec();
}

int mainSimple(const QString &filename, bool loop)
{
    QMediaPlayer player;
    QVideoWidget widget1;
    QAudioOutput audioOutput;
    player.setVideoOutput(&widget1);
    player.setAudioOutput(&audioOutput);
    player.setSource(filename);

    widget1.show();

    if (loop)
        player.setLoops(QMediaPlayer::Infinite);

    player.play();
    return QApplication::exec();
}

int mainPlayAfterEndOfMedia(const QString &filename)
{
    QMediaPlayer player;
    QVideoWidget widget1;
    QAudioOutput audioOutput;
    player.setVideoOutput(&widget1);
    player.setAudioOutput(&audioOutput);
    player.setSource(filename);

    widget1.show();

    player.play();

    QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &player,
                     [&](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::MediaStatus::EndOfMedia)
            player.play();
    });

    return QApplication::exec();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Minimal Player");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("media", "File to play");

    QCommandLineOption toggleWidgetsOption{
        "toggle-widgets",
        "Toggle between widgets.",
    };
    parser.addOption(toggleWidgetsOption);
    QCommandLineOption playAfterEndOfMediaOption{
        "play-after-end-of-media",
        "Play after end of media.",
    };
    parser.addOption(playAfterEndOfMediaOption);

    QCommandLineOption loopOption{ "loop", "Loop." };
    parser.addOption(loopOption);

    parser.process(app);

    if (parser.positionalArguments().isEmpty()) {
        qInfo() << "Please specify a video source";
        return 0;
    }

    QString filename = parser.positionalArguments()[0];

    if (parser.isSet(toggleWidgetsOption))
        return mainToggleWidgets(filename);

    if (parser.isSet(playAfterEndOfMediaOption))
        return mainPlayAfterEndOfMedia(filename);

    bool loop = parser.isSet(loopOption);

    return mainSimple(filename, loop);
}
