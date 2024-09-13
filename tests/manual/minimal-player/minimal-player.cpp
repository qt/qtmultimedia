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

struct CLIArgs
{
    bool loop;
    bool noAudio;
    bool toggleWidgets;
    QString media;
    bool playAfterEndOfMediaOption;
};

std::optional<CLIArgs> parseArgs(QCoreApplication &app)
{
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

    QCommandLineOption disableAudioOption{
        "no-audio",
        "Disable audio output.",
    };
    parser.addOption(disableAudioOption);

    QCommandLineOption loopOption{
        "loop",
        "Loop.",
    };
    parser.addOption(loopOption);

    parser.process(app);

    if (parser.positionalArguments().isEmpty()) {
        qInfo() << "Please specify a media source";
        return std::nullopt;
    }

    QString filename = parser.positionalArguments()[0];

    return CLIArgs{
        parser.isSet(loopOption),
        parser.isSet(disableAudioOption),
        parser.isSet(toggleWidgetsOption),
        filename,
        parser.isSet(playAfterEndOfMediaOption),
    };
}

int run(const CLIArgs &args)
{
    QTimer toggleOutput;
    bool toggled = {};

    QMediaPlayer player;
    QVideoWidget widget1;
    QVideoWidget widget2;
    QAudioOutput audioOutput;
    player.setVideoOutput(&widget1);
    if (args.noAudio)
        player.setAudioOutput(nullptr);
    else
        player.setAudioOutput(&audioOutput);
    player.setSource(args.media);

    widget1.show();

    if (args.toggleWidgets) {
        toggleOutput.callOnTimeout([&] {
            toggled = !toggled;
            if (toggled)
                player.setVideoOutput(&widget2);
            else
                player.setVideoOutput(&widget1);
        });

        toggleOutput.setInterval(1s);
        toggleOutput.start();
        widget2.show();
    }

    player.play();

    if (args.playAfterEndOfMediaOption) {
        QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &player,
                         [&](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::MediaStatus::EndOfMedia)
                player.play();
        });
    }

    return QApplication::exec();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    std::optional<CLIArgs> args = parseArgs(app);
    if (!args)
        return 1;

    return run(*args);
}
