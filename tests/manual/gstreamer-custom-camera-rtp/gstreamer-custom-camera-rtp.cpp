// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/private/qgstreamer_platformspecificinterface_p.h>
#include <QtMultimediaWidgets/QtMultimediaWidgets>
#include <QtWidgets/QApplication>
#include <QtCore/QCommandLineParser>

#include <QtGstreamerMediaPluginImpl/private/qgst_p.h>
#include <QtGstreamerMediaPluginImpl/private/qgstpipeline_p.h>

using namespace std::chrono_literals;
using namespace Qt::Literals;

struct GStreamerRtpStreamSender
{
    GStreamerRtpStreamSender()
    {
        element = QGstElement::createFromPipelineDescription(
                "videotestsrc ! jpegenc ! rtpjpegpay ! udpsink host=127.0.0.1 port=50004"_ba);

        pipeline.add(element);
        pipeline.setStateSync(GstState::GST_STATE_PLAYING);
        pipeline.dumpGraph("sender");
    }

    ~GStreamerRtpStreamSender() { pipeline.setStateSync(GstState::GST_STATE_NULL); }

    QGstPipeline pipeline = QGstPipeline::create("UdpSend");
    QGstElement element;
};

int main(int argc, char **argv)
{
    qputenv("QT_MEDIA_BACKEND", "gstreamer");

    gst_init(&argc, &argv);
    GStreamerRtpStreamSender sender;

    QApplication app(argc, argv);

    QByteArray pipelineString =
            R"(udpsrc port=50004 ! application/x-rtp,encoding=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! videoconvert)"_ba;
    QVideoWidget wid;
    wid.show();

    QMediaCaptureSession session;
    session.setVideoSink(wid.videoSink());

    QCamera *cam = QGStreamerPlatformSpecificInterface::instance()->makeCustomGStreamerCamera(
            pipelineString, &session);
    session.setCamera(cam);
    cam->start();

    return QApplication::exec();
}
