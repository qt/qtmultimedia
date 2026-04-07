// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QTimer>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>
#include <QtCore/QDateTime>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QAudioDevice>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaRecorder>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QAudioInput>
#include <chrono>
#include <signal.h>
#include <filesystem>
#include <optional>

using namespace std::chrono_literals;
using namespace Qt::Literals;

namespace {

struct CommandLineArgs
{
    std::optional<std::chrono::seconds> recordingDuration;
    std::optional<std::filesystem::path> outputPath;
};

QString generateOutputPath()
{
    QDateTime now = QDateTime::currentDateTime();
    QString filename = u"recording_%1.mp4"_s.arg(now.toString(u"yyyyMMdd_HHmmss"_s));
    QString moviesPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    return QDir(moviesPath).filePath(filename);
}

} // namespace

std::optional<CommandLineArgs> parseCommandLine(QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(u"Minimal Camera Recorder"_s);
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption durationOption{
        QList{ u"d"_s, u"duration"_s },
        u"Duration of recording in seconds"_s,
        u"seconds"_s,
        u"5"_s,
    };
    parser.addOption(durationOption);

    const QCommandLineOption outputOption{
        QList{ u"o"_s, u"output"_s },
        u"Output video file path"_s,
        u"path"_s,
    };
    parser.addOption(outputOption);

    parser.process(app);

    CommandLineArgs args;

    // Parse duration
    bool ok = false;
    int durationSeconds = parser.value(durationOption).toInt(&ok);
    if (ok && durationSeconds > 0) {
        args.recordingDuration = std::chrono::seconds(durationSeconds);
    } else {
        qWarning() << "Invalid duration value, using default 5 seconds";
        args.recordingDuration = std::chrono::seconds(5);
    }

    // Parse output path
    if (parser.isSet(outputOption))
        args.outputPath = std::filesystem::path(parser.value(outputOption).toStdString());
    else
        args.outputPath = std::filesystem::path(generateOutputPath().toStdString());

    return args;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    std::optional<CommandLineArgs> args = parseCommandLine(app);
    if (!args)
        return 1;

    std::chrono::seconds duration = args->recordingDuration.value_or(std::chrono::seconds(5));
    std::filesystem::path outputPath =
            args->outputPath.value_or(std::filesystem::path(generateOutputPath().toStdString()));

    QString outputPathStr = QString::fromStdString(outputPath.string());
    QUrl mediaUrl = QUrl::fromLocalFile(outputPathStr);

    auto camera = QCamera(QMediaDevices::defaultVideoInput());
    auto audioInput = QAudioInput(QMediaDevices::defaultAudioInput());

    qInfo() << "recording from camera:" << camera.cameraDevice().description();
    qInfo() << "recording from audio device:" << audioInput.device().description();

    // Create capture session and recorder
    QMediaCaptureSession session;
    session.setCamera(&camera);
    session.setAudioInput(&audioInput);

    camera.start();

    QMediaRecorder recorder;
    session.setRecorder(&recorder);
    recorder.setOutputLocation(mediaUrl);

    static QMediaRecorder &recorderRef = recorder; // for signal handler

    signal(SIGINT, [](int signal) {
        if (signal == SIGINT) {
            qInfo() << "Received SIGINT, stopping recording...";
            recorderRef.stop();
            if (qApp)
                qApp->quit();
        }
    });

    // Start recording
    recorder.record();

    qInfo() << "Recording" << duration.count() << "seconds of video to" << outputPathStr;
    qInfo() << "Actual location:" << recorder.actualLocation().toLocalFile();

    QTimer::singleShot(duration, &app, [&] {
        qInfo() << "Duration reached, stopping recording...";
        recorder.stop();
        QCoreApplication::quit();
    });

    QObject::connect(&recorder, &QMediaRecorder::recorderStateChanged, &app,
                     [&](QMediaRecorder::RecorderState state) {
        if (state == QMediaRecorder::StoppedState) {
            qInfo() << "Recording stopped";
            QCoreApplication::quit();
        }
    });

    return QCoreApplication::exec();
}
