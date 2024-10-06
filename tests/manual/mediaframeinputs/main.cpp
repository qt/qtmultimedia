// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>

#include "commandlineparser.h"
#include "previewrunner.h"
#include "recordingrunner.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const CommandLineParser::Result cmdResult = CommandLineParser().process();

    std::unique_ptr<RecordingRunner> recordingRunner;
    if (cmdResult.pushModeSettings)
        recordingRunner = std::make_unique<PushModeRecordingRunner>(
                cmdResult.recorderSettings, cmdResult.audioGeneratorSettings,
                cmdResult.videoGeneratorSettings, *cmdResult.pushModeSettings);
    else
        recordingRunner = std::make_unique<PullModeRecordingRunner>(
                cmdResult.recorderSettings, cmdResult.audioGeneratorSettings,
                cmdResult.videoGeneratorSettings);

    PreviewRunner previewRunner;

    QObject::connect(recordingRunner.get(), &RecordingRunner::finished, &app, [&]() {
        if (recordingRunner->recorder().error() == QMediaRecorder::NoError)
            previewRunner.run(recordingRunner->recorder().actualLocation());
        else
            QMetaObject::invokeMethod(&app, &QApplication::quit, Qt::QueuedConnection);
    });

    QObject::connect(&previewRunner, &PreviewRunner::finished, &app, &QApplication::quit,
                     Qt::QueuedConnection);

    recordingRunner->run();

    return app.exec();
}
