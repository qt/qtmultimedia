// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "audiodecoder.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#ifdef Q_OS_ANDROID
#    include <QApplication>
#    include <QFileDialog>
#    include <QMessageBox>
#    include <QStandardPaths>
#endif

#include <stdio.h>

int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    QApplication app(argc, argv);
#else
    QCoreApplication app(argc, argv);
#endif

    QFileInfo sourceFile;
    QFileInfo targetFile;
    bool isPlayback = false;
    bool isDelete = false;

#ifndef Q_OS_ANDROID
    QTextStream cout(stdout, QIODevice::WriteOnly);
    if (app.arguments().size() < 2) {
        cout << "Usage: audiodecoder [-p] [-pd] SOURCEFILE [TARGETFILE]\n";
        cout << "Set -p option if you want to play output file.\n";
        cout << "Set -pd option if you want to play output file and delete it after successful "
                "playback.\n";
        cout << "Default TARGETFILE name is \"out.wav\" in the same directory as the source "
                "file.\n";
        return 0;
    }

    if (app.arguments().at(1) == "-p")
        isPlayback = true;
    else if (app.arguments().at(1) == "-pd") {
        isPlayback = true;
        isDelete = true;
    }

    int sourceFileIndex = (isPlayback || isDelete) ? 2 : 1;
    if (app.arguments().size() <= sourceFileIndex) {
        cout << "Error: source filename is not specified.\n";
        return 0;
    }

    sourceFile.setFile(app.arguments().at(sourceFileIndex));
    if (app.arguments().size() > sourceFileIndex + 1)
        targetFile.setFile(app.arguments().at(sourceFileIndex + 1));
    else
        targetFile.setFile(sourceFile.dir().absoluteFilePath("out.wav"));

#else

    const QString message = "You will be prompted to select an audio file (e.g. mp3 or ogg format) "
                            "which will be decoded and played back to you.";
    QMessageBox messageBox(QMessageBox::Information, "Audio Decoder", message, QMessageBox::Ok);
    messageBox.exec();
    sourceFile =
            QFileInfo(QFileDialog::getOpenFileName(messageBox.parentWidget(), "Select Audio File"));
    auto musicPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    targetFile = QFileInfo(musicPath.append("/out.wav"));
    isPlayback = true;
#endif
    AudioDecoder decoder(isPlayback, isDelete, targetFile.absoluteFilePath());
    QObject::connect(&decoder, &AudioDecoder::done, &app, &QCoreApplication::quit);
    decoder.setSource(sourceFile.absoluteFilePath());
    decoder.start();
    if (decoder.getError() != QAudioDecoder::NoError)
        return 0;

    return app.exec();
}
