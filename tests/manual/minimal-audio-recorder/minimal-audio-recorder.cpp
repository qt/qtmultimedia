// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QTimer>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>
#include <QtCore/QtEnvironmentVariables>
#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaRecorder>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QAudioDevice>
#include <chrono>

using namespace std::chrono_literals;
using namespace Qt::Literals;

QMediaFormat::FileFormat toFileFormat(const QString &format)
{
    if (format == "mpeg4audio")
        return QMediaFormat::Mpeg4Audio;
    if (format == "aac")
        return QMediaFormat::AAC;
    if (format == "wma")
        return QMediaFormat::WMA;
    if (format == "mp3")
        return QMediaFormat::MP3;
    if (format == "flac")
        return QMediaFormat::FLAC;
    if (format == "wave")
        return QMediaFormat::Wave;

    qWarning() << "Invalid file format, using default AAC";

    return QMediaFormat::AAC;
}

int main(int argc, char **argv)
{
    if (qEnvironmentVariableIsEmpty("QT_LOGGING_RULES"))
        qputenv("QT_LOGGING_RULES", "qt.multimedia.ffmpeg.audioencoder=true");

    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Minimal Audio Recorder");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("output_file", "Output audio file");

    const QCommandLineOption durationOption{
        { "d", "duration" }, "Duration of recording in seconds", "seconds", "5"
    };
    parser.addOption(durationOption);

    const QCommandLineOption formatOption{
        { "f", "format" }, "Container format", "Mpeg4Audio, AAC, WMA, MP3, FLAC, Wave", "AAC"
    };
    parser.addOption(formatOption);

    const QCommandLineOption deviceOption{ { "i", "device_id" }, "Audio device ID", "string" };
    parser.addOption(deviceOption);

    parser.process(app);

    if (parser.positionalArguments().isEmpty())
        parser.showHelp(1); // Exits process

    const QUrl mediaUrl = QUrl::fromLocalFile(parser.positionalArguments()[0]);
    const std::chrono::seconds duration{ parser.value(durationOption).toInt() };
    const QMediaFormat::FileFormat format = toFileFormat(parser.value(formatOption).toLower());
    const QString deviceId = parser.value(deviceOption);

    QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
    QAudioDevice selectedDevice;

    qInfo() << "Available audio devices:";
    for (const QAudioDevice &device : inputs) {
        qInfo() << "ID" << device.id() << "Description" << device.description();
        if (device.id() == deviceId)
            selectedDevice = device;
    }

    QAudioInput input;
    if (selectedDevice.isNull()) {
        qInfo() << "Using default device";
    } else {
        qInfo() << "Using device" << selectedDevice.description();
        input.setDevice(selectedDevice);
    }

    QMediaRecorder recorder;
    recorder.setOutputLocation(mediaUrl);
    recorder.setMediaFormat({ format });

    QMediaCaptureSession session;
    session.setRecorder(&recorder);
    session.setAudioInput(&input);

    recorder.record();

    qInfo() << "Recording" << duration.count() << "seconds of audio to"
            << recorder.actualLocation().toLocalFile();

    QTimer::singleShot(duration, &app, [&] {
        recorder.stop();
        qDebug() << "Recording completed";
        QCoreApplication::quit();
    });

    return QCoreApplication::exec();
}
