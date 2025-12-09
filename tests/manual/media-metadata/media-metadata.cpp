// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QMediaMetaData>

#include <cstdio>
#include <optional>

using namespace std::chrono_literals;
using namespace Qt::Literals;

struct CLIArgs
{
    QString media;
};

std::optional<CLIArgs> parseArgs(QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Read metadata from media");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("media", "File to open");

    parser.process(app);

    if (parser.positionalArguments().isEmpty()) {
        qInfo() << "Please specify a media file";
        return std::nullopt;
    }

    return CLIArgs{
        parser.positionalArguments().front(),
    };
}

auto asString = [](const auto &arg) {
    QString str;
    QDebug dbg(&str);

    dbg << arg;
    return str;
};

void printMetadata(QTextStream &stream, const QMediaMetaData &metadata)
{
    for (auto entry : metadata.asKeyValueRange())
        stream << "    " << asString(entry.first) << ": " << asString(entry.second) << "\n";
    stream << "\n";
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    std::optional<CLIArgs> args = parseArgs(app);
    if (!args)
        return 1;

    QMediaPlayer player;

    QObject::connect(&player, &QMediaPlayer::errorOccurred, &app, [&] {
        QTextStream out(stdout);
        out << "Error occurred: " << player.errorString();
        QCoreApplication::exit(1);
    });

    QObject::connect(&player, &QMediaPlayer::metaDataChanged, &app, [&] {
        QTextStream out(stdout);
        out << "Metadata:\n";
        printMetadata(out, player.metaData());
        out << "\n\n";
    });

    QObject::connect(&player, &QMediaPlayer::tracksChanged, &app, [&] {
        QTextStream out(stdout);

        auto listOfVideoTracks = player.videoTracks();
        out << "Video tracks:\n";
        for (const QMediaMetaData &metadata : listOfVideoTracks) {
            out << "  Track no " << listOfVideoTracks.indexOf(metadata) << ":\n";
            printMetadata(out, metadata);
        }
        out << "\n\n";

        auto listOfAudioTracks = player.audioTracks();
        out << "Audio tracks:\n";
        for (const QMediaMetaData &metadata : listOfAudioTracks) {
            out << "  Track no " << listOfAudioTracks.indexOf(metadata) << ":\n";
            printMetadata(out, metadata);
        }
        out << "\n\n";

        auto listOfSubtitleTracks = player.subtitleTracks();
        out << "Subtitle tracks:\n";
        for (const QMediaMetaData &metadata : listOfSubtitleTracks) {
            out << "  Track no " << listOfSubtitleTracks.indexOf(metadata) << ":\n";
            printMetadata(out, metadata);
        }
        out << "\n\n";
        QCoreApplication::exit(0);
    });

    player.setSource(args->media);

    return app.exec();
}
