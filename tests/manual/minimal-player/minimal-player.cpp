// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimediaWidgets/QVideoWidget>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QMediaPlayer player;
    QVideoWidget widget;
    QAudioOutput audioOutput;
    player.setVideoOutput(&widget);
    player.setAudioOutput(&audioOutput);

    if (QApplication::arguments().size() > 1) {
        player.setSource(QApplication::arguments()[1]);
    } else {
        qInfo() << "Please specify a video source";
        return 0;
    }

    widget.show();
    player.play();
    return QApplication::exec();
}
