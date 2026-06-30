// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QTimer>
#include <QtCore/QCommandLineParser>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include <optional>

using namespace std::chrono_literals;
using namespace Qt::Literals;

struct CLIArgs
{
    std::optional<int> loop;
    bool noAudio;
    bool toggleWidgets;
    QString media;
    bool playAfterEndOfMediaOption;
};

std::optional<CLIArgs> parseArgs(QCoreApplication &app)
{
    using namespace Qt::Literals;
    QCommandLineParser parser;
    parser.setApplicationDescription(u"Minimal Player"_s);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(u"media"_s, u"File to play"_s);

    QCommandLineOption toggleWidgetsOption{
        u"toggle-widgets"_s,
        u"Toggle between widgets."_s,
    };
    parser.addOption(toggleWidgetsOption);

    QCommandLineOption playAfterEndOfMediaOption{
        u"play-after-end-of-media"_s,
        u"Play after end of media."_s,
    };
    parser.addOption(playAfterEndOfMediaOption);

    QCommandLineOption disableAudioOption{
        u"no-audio"_s,
        u"Disable audio output."_s,
    };
    parser.addOption(disableAudioOption);

    QCommandLineOption loopOption{
        u"loop"_s,
        u"Loop."_s,
        u"loop"_s,
        u"0"_s,
    };
    parser.addOption(loopOption);

    parser.process(app);

    if (parser.positionalArguments().isEmpty()) {
        qInfo() << "Please specify a media source";
        return std::nullopt;
    }

    QString filename = parser.positionalArguments()[0];

    std::optional<int> loops;
    bool ok{};
    int loopValue = parser.value(loopOption).toInt(&ok);
    if (ok)
        loops = loopValue;

    return CLIArgs{
        loops,    parser.isSet(disableAudioOption),        parser.isSet(toggleWidgetsOption),
        filename, parser.isSet(playAfterEndOfMediaOption),
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

    if (args.loop)
        player.setLoops(*args.loop);

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

    QObject::connect(&player, &QMediaPlayer::errorOccurred, &player,
                     [](QMediaPlayer::Error error, const QString &errorString) {
        qCritical() << "Error:" << error << "-" << errorString;
        QCoreApplication::exit(1);
    });

    player.play();

    QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &player,
                     [&](QMediaPlayer::MediaStatus status) {
        if (status != QMediaPlayer::MediaStatus::EndOfMedia)
            return;
        if (args.playAfterEndOfMediaOption)
            player.play();
        else
            QCoreApplication::exit(0);
    });

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
