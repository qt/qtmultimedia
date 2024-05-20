// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/private/qgstreamer_platformspecificinterface_p.h>
#include <QtMultimediaWidgets/QtMultimediaWidgets>
#include <QtWidgets/QApplication>
#include <QtCore/QCommandLineParser>

using namespace std::chrono_literals;
using namespace Qt::Literals;

int main(int argc, char **argv)
{
    qputenv("QT_MEDIA_BACKEND", "gstreamer");

    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("GStreamer Custom Camera");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(
            "pipeline", "Pipeline string, e.g. `videotestsrc pattern=smpte-rp-219 is-live=true`");

    parser.process(app);

    QByteArray pipelineString;

    if (parser.positionalArguments().isEmpty()) {
        // pipelineString = "videotestsrc pattern=smpte-rp-219 is-live=true";
        pipelineString = "videotestsrc is-live=true ! gamma gamma=2.0";
    } else {
        pipelineString = parser.positionalArguments()[0].toLatin1();
    }

    QVideoWidget wid;

    QMediaCaptureSession session;
    session.setVideoSink(wid.videoSink());

    QCamera *cam = QGStreamerPlatformSpecificInterface::instance()->makeCustomGStreamerCamera(
            pipelineString, &session);
    session.setCamera(cam);
    cam->start();

    wid.show();

    return QApplication::exec();
}
