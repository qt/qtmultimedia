// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qdir.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qtimer.h>
#include <QtCore/qurl.h>
#include <QtCore/qdatetime.h>
#include <QtWidgets/qapplication.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qscreencapture.h>
#include <QtMultimediaWidgets/qvideowidget.h>
#include <QtMultimedia/qaudioinput.h>

class Widget : public QVideoWidget
{
    Q_OBJECT
public:
    using QVideoWidget::QVideoWidget;

    void closeEvent(QCloseEvent *event) override
    {
        emit onClose();
        QVideoWidget::closeEvent(event);
    }

signals:
    void onClose();
};

QUrl createFileName()
{
    const QDir mediaDir = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                                  .value(0, QDir::homePath());

    const QDateTime currentDateTime = QDateTime::currentDateTime();
    const QString formattedDateTime = currentDateTime.toString("yyyy-MM-dd-hh-mm-ss");
    const QString filename = mediaDir.filePath("screen-recording-" + formattedDateTime);
    return QUrl::fromLocalFile(filename);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMediaCaptureSession session;

    Widget widget;
    session.setVideoOutput(&widget);

    QScreenCapture screen;
    session.setScreenCapture(&screen);
    screen.setScreen(QApplication::screens().at(0)); // Change to select screen
    screen.start();

    QAudioInput audio;
    session.setAudioInput(&audio);

    QMediaRecorder recorder;
    session.setRecorder(&recorder);
    recorder.setOutputLocation(createFileName());

    QObject::connect(&widget, &Widget::onClose, &recorder, &QMediaRecorder::stop);

    widget.show();
    recorder.record();

    qInfo() << "Recording to" << recorder.actualLocation();
    qInfo() << "Close window to stop";

    return QApplication::exec();
}

#include "minimal-screen-recorder.moc"
