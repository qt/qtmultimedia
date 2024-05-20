// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/private/qmediaplayer_p.h>
#include <QtMultimedia/private/qgstreamer_platformspecificinterface_p.h>
#include <QtQGstreamerMediaPlugin/private/qgstpipeline_p.h>

#include <memory>

QT_USE_NAMESPACE

class tst_QMediaPlayerGStreamer : public QObject
{
    Q_OBJECT

public:
    tst_QMediaPlayerGStreamer();

public slots:
    void init();
    void cleanup();

private slots:
    void constructor_preparesGstPipeline();

private:
    std::unique_ptr<QMediaPlayer> player;

    GstPipeline *getGstPipeline()
    {
        auto *iface = QGStreamerPlatformSpecificInterface::instance();
        return iface ? iface->gstPipeline(player.get()) : nullptr;
    }

    void dumpGraph(const char *fileNamePrefix)
    {
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(getGstPipeline()),
                                  GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_VERBOSE),
                                  fileNamePrefix);
    }
};

tst_QMediaPlayerGStreamer::tst_QMediaPlayerGStreamer()
{
    qputenv("QT_MEDIA_BACKEND", "gstreamer");
}

void tst_QMediaPlayerGStreamer::init()
{
    player = std::make_unique<QMediaPlayer>();
}

void tst_QMediaPlayerGStreamer::cleanup()
{
    player.reset();
}

void tst_QMediaPlayerGStreamer::constructor_preparesGstPipeline()
{
    auto *rawPipeline = getGstPipeline();
    QVERIFY(rawPipeline);

    QGstPipeline pipeline{
        rawPipeline,
        QGstPipeline::NeedsRef,
    };
    QVERIFY(pipeline);

    QVERIFY(pipeline.findByName("videoInputSelector"));
    QVERIFY(pipeline.findByName("audioInputSelector"));
    QVERIFY(pipeline.findByName("subTitleInputSelector"));

    dumpGraph("constructor_preparesGstPipeline");
}

QTEST_GUILESS_MAIN(tst_QMediaPlayerGStreamer)

#include "tst_qmediaplayer_gstreamer.moc"
